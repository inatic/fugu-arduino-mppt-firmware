/*  PROJECT FUGU FIRMWARE  (DIY 1kW Open Source MPPT Solar Charge Controller)
 *  Originally by: TechBuilder (Angelo Casimiro)
 *  GitHub - www.github.com/AngeloCasi
 *  Email - casithebuilder@gmail.com
 *  YouTube - www.youtube.com/TechBuilder
 *  Facebook - www.facebook.com/AngeloCasii
 *  YOUTUBE TUTORIAL LINK: www.youtube.com/watch?v=ShXNJM6uHLM
 *  GITHUB UPDATED FUGU FIRMWARE LINK: github.com/AngeloCasi/FUGU-ARDUINO-MPPT-FIRMWARE
 *  -------------------------------------------------------------------------------------------------
 *  Modified by: https://github.com/inatic
 *  -------------------------------------------------------------------------------------------------
 */      
 
//===================================== ARDUINO LIBRARIES ==========================================//
#include <Arduino.h>
#include <Wire.h>                       // For I2C communication with the ADS1115.
#include <Adafruit_ADS1X15.h>           // For communicating with the ADC.

TaskHandle_t Core2;                     // Task handle for second core of ESP32.
//Adafruit_ADS1015 ads;                 // ADS1015 ADC Library (By: Adafruit)
Adafruit_ADS1115 ads;                   // ADS1115 ADC Library (By: Adafruit)

//===================================== PIN DEFINITIONS ===========================================//
const int BACKFLOW_MOSFET_PIN = 27;
const int BUCK_IN_PIN         = 33;     // MOSFET Driver Input Pin
const int BUCK_EN_PIN         = 32;     // MOSFET Driver Enable Pin
const int LED_PIN             = 2;
const int FAN_PIN             = 16;
const int ADC_ALERT_PIN       = 34;
const int TEMP_SENSOR_PIN     = 35;

//===================================== USER PARAMETERS ===========================================//                              
enum InputMode {
  MPPT,                    // Solar panel input (needs MPPT hunting)
  PSU                      // Power supply/battery input (constant voltage source)
};

enum TelemetryMode {
  TELEM_OFF,
  TELEM_ALL,
  TELEM_NUMBERS_ONLY,
  TELEM_ESSENTIAL
};

enum ResetMode {           // Reset Telemetry Data
  RESET_NEVER,
  RESET_DAILY,
  RESET_WEEKLY,
  RESET_MONTHLY,
  RESET_YEARLY
};

const InputMode     INPUT_MODE         = MPPT;
const TelemetryMode SERIAL_TELEM_MODE  = TELEM_ALL;
const ResetMode     RESET_MODE         = RESET_NEVER;

const float OUTPUT_VOLTAGE             = 3.65;
const float MAX_OUTPUT_CURRENT         = 5.00;
const bool  FAN_ENABLED                = 0;
const bool  FAN_ALWAYS_ON              = 0;
const int   FAN_ON_TEMPERATURE         = 60;
const int   FAN_OFF_TEMPERATURE        = 55;
const int   MAX_SYSTEM_TEMPERATURE     = 80;
const int   PWM_RESOLUTION_BITS        = 11;
const int   PWM_FREQUENCY_HZ           = 39000;
const int   USB_BAUD_RATE              = 500000;
const int   WIFI_ENABLED               = 0;
const int   BLUETOOTH_ENABLED          = 0;
const int   ROUTINE_INTERVAL_MS        = 250;          // Refresh Rate For System Processes (ms)
const int   SERIAL_INTERVAL_MS         = 1;            // Refresh Rate For USB Serial Datafeed (ms)

//===================================== DATA STRUCTURES ===========================================//
struct SensorData {
  uint32_t timestamp;
  float voltageInput;
  float voltageOutput;
  float currentInput;
  float currentOutput;
  float powerInput;
  float powerOutput;
  float temperature;
  float currentSensorVoltage;
};

struct SensorCalibration {
  float currentSensorMidpointVoltage;
};

struct SystemState {
  bool isFatallyLowVoltage;
  bool isInputUnderVoltage;
  bool isInputOverCurrent;
  bool isOutputOverVoltage;
  bool isOutputOverCurrent;
  bool isInputBelowOutputVoltage;
  bool isOverTemperature;
  bool isBatteryNotConnected;
  int  activeConditions;
  int  powerSource; // 0: None, 1: Solar, 2: Battery
};

struct ControlState {
  // Control states
  bool isInputEnabled;
  bool isBuckEnabled;
  bool isFanEnabled;
};

struct PwmState {
  int dutyCycle;
  int idealDutyCycle;
  int minDutyCycle;
  int maxDutyCycle;
  int fullScaleDutyCycle;
};

struct TelemetryData {
  float         wattHours;
  float         loopTimeMs;
  unsigned long timeOn;
};

//======================================== STRUCTS ================================================//
SensorData         sensorData = {};
SensorCalibration  sensorCalibration = {};
SystemState        systemState = {};
ControlState       controlState = {};
PwmState           pwmState = {};
TelemetryData      telemetryData = {};

//===================================== MAIN PROGRAM ==============================================//
/**
 * Core0: Setup
 * WiFi functions are offloaded to their own core as they can be slow and take an unpredictable 
 * amount of time.
 */
void coreTwo(void * pvParameters){                                                             
  setupWiFi();
  // Core0: Loop
  while(1){
}}

/**
 * Core1: Setup
 */
void setup() {
  const float PWM_MAX_DUTY_CYCLE_PERCENT = 97;
  
  // Initializes serial communication for debugging and telemetry.
  Serial.begin(USB_BAUD_RATE);  
  Serial.println("> Serial Initialized");
  
  // Configures GPIO pins as either inputs or outputs.    
  pinMode(BACKFLOW_MOSFET_PIN,OUTPUT);                          
  pinMode(BUCK_EN_PIN,OUTPUT);
  pinMode(LED_PIN,OUTPUT); 
  pinMode(FAN_PIN,OUTPUT);
  pinMode(ADC_ALERT_PIN,INPUT);
  pinMode(TEMP_SENSOR_PIN,INPUT);
  
  // This is an ESP32-specific function that configures a PWM channel.
  ledcAttach(BUCK_IN_PIN, PWM_FREQUENCY_HZ, PWM_RESOLUTION_BITS);                              
  // Sets the initial PWM duty cycle to 0.
  pwmState.dutyCycle = 0;
  ledcWrite(BUCK_IN_PIN,pwmState.dutyCycle);                                                   
  // Get maximum PWM Duty Cycle value, which depends on PWM reslution (for 11 bit: 2^11-1=2047
  pwmState.fullScaleDutyCycle = pow(2,PWM_RESOLUTION_BITS)-1;                                   
  // Set an upper limit to the PWM Duty Cycle
  pwmState.maxDutyCycle = (PWM_MAX_DUTY_CYCLE_PERCENT*pwmState.fullScaleDutyCycle)/100.0;      

  // Set initial current sensor midpoint voltage
  sensorCalibration.currentSensorMidpointVoltage = 2.5250;

  // Sets ADC Gain so the measuring range is +/- 2.048V
  ads.setGain(GAIN_TWO); 
  ads.setDataRate(RATE_ADS1115_860SPS);
  ads.begin();

  // Ensure that the buck converter is disabled at startup.
  buckDisable();                                                                               
  // Enable dual core multitasking
  xTaskCreatePinnedToCore(coreTwo,"coreTwo",10000,NULL,0,&Core2,0);

  Serial.println("> MPPT has initialized");

  // Print header for CSV output
  if(SERIAL_TELEM_MODE==TELEM_NUMBERS_ONLY){
      Serial.println("sep=,");
      Serial.println(" ST, VI, VO, CI, CO, PI, PO, T, CSV, CSMV, FLV, IUV, IOC, OOV, OOC, IBOV, OT, BNC, PS, IE, BE, FE, DC, IDC, MiDC, MaDC, FDC, WH, LT, TO ");
  }
}

/**
 * CORE1: LOOP
 */
void loop() {
  // Read the latest values from the sensors.
  readSensors(sensorData, systemState, sensorCalibration);

  // Check the system state
  determineSystemState(sensorData, systemState);       

  // Calibrate sensors
  calibrateSensors(sensorData, sensorCalibration, systemState, controlState);

  // Implement the MPPT and PSU algorithms.
  buckAlgorithms(sensorData, systemState, controlState, pwmState);

  // Handle various system-level tasks.
  systemProcesses(sensorData, sensorCalibration, systemState, controlState, telemetryData);

  // Send data over the serial ports.
  onboardTelemetry(sensorData, sensorCalibration, systemState, controlState, pwmState, telemetryData);
}
