
/**
 * Calculates temperature from an NTC thermistor using the Steinhart-Hart equation.
*/
static float calculateTemperature(float rawAdcTemperature) {
  const float NTC_FIXED_RESISTOR_OHMS = 10000.0;
  // The maximum value the ESP32's ADC will measure
  const float MAX_ADC_VALUE = 4095.0;
  // The Steinhart-Hart coefficients, they are specific to the thermistor being used. 
  const float A = 0.001009249522;
  const float B = 0.0002378405444;     
  const float C = 0.0000002019202697; 
  
  // This is derived from the voltage divider formula: R_ntc = R_fixed * (V_in / V_out - 1)
  float ntcResistance = NTC_FIXED_RESISTOR_OHMS * ( (MAX_ADC_VALUE / rawAdcTemperature) - 1.0 );

  // Calculate the natural logarithm of the resistance.
  float logNtcRes = log(ntcResistance);

  // Apply the Steinhart-Hart equation to get temperature in Kelvin.
  // Formula: 1 / (A + B*ln(R) + C*(ln(R))^3)
  float temperatureKelvin = 1.0 / (A + (B * logNtcRes) + (C * logNtcRes * logNtcRes * logNtcRes));

  // Convert temperature from Kelvin to Celsius.
  float temperatureCelsius = temperatureKelvin - 273.15;

  return temperatureCelsius;
}

/**
 * Read and calculate all power related values at the same time
 */
static SensorData getSensorData(const float currentSensorMidpointVoltage, const SystemState &system){
  const int SAMPLE_COUNT = 4;
  const float INPUT_VOLTAGE_DIVIDER_RATIO = 40.2156;           // Change to calibrate sensor.
  const float OUTPUT_VOLTAGE_DIVIDER_RATIO = 24.5;          // Change to calibrate sensor.
  const float CURRENT_SENSOR_VOLTAGE_DIVIDER_RATIO = 1.33;   // Change to calibrate sensor.
  const float CURRENT_SENSOR_SENSITIVITY = 0.066;             // V/A for the ACS712 sensor

  SensorData sensors = {};
  float rawAdcVoltageInput = 0.0;
  float rawAdcVoltageOutput = 0.0;
  float rawAdcCurrentInput = 0.0;
  float rawAdcTemperature = 0.0;
  float voltageInput = 0.0;
  float voltageOutput = 0.0;
  float currentInput = 0.0;
  float currentOutput = 0.0;
  float powerInput = 0.0;
  float powerOutput = 0.0;
  float temperature = 0.0;
  float currentSensorVoltage = 0.0;
  
  // Measure voltages and input current
  for(int i = 0; i<SAMPLE_COUNT; i++){
    rawAdcVoltageInput = rawAdcVoltageInput + ads.computeVolts(ads.readADC_SingleEnded(3));
    rawAdcVoltageOutput = rawAdcVoltageOutput + ads.computeVolts(ads.readADC_SingleEnded(1));
    rawAdcCurrentInput = rawAdcCurrentInput + ads.computeVolts(ads.readADC_SingleEnded(2));
  }

  voltageInput = (rawAdcVoltageInput/SAMPLE_COUNT)*INPUT_VOLTAGE_DIVIDER_RATIO;
  voltageOutput = (rawAdcVoltageOutput/SAMPLE_COUNT)*OUTPUT_VOLTAGE_DIVIDER_RATIO;
  currentSensorVoltage = (rawAdcCurrentInput/SAMPLE_COUNT)*CURRENT_SENSOR_VOLTAGE_DIVIDER_RATIO;
  // The current sensor is mounted in reverse so its measurement goes from 2.5V (0A) down to 0V (±37A)
  // The midpoint voltage is subtracted to flip this around: 0V (0A) to 2.5V (±37A)
  currentInput  = ((currentSensorVoltage-currentSensorMidpointVoltage)*-1)/CURRENT_SENSOR_SENSITIVITY;  
  if(currentInput<0){
    currentInput=0.0000;
  }

  // Measure temperature
  rawAdcTemperature = analogRead(TEMP_SENSOR_PIN);
  temperature = calculateTemperature(rawAdcTemperature);

  // Calculate power and output current
  if (voltageOutput <= 0.1) {
    currentOutput = 0.0; // Avoid dividing by zero
  } else {
    powerInput = voltageInput * currentInput;
    powerOutput = powerInput; // Ignore losses
    if (powerOutput < 0.0) {
      powerOutput = 0.0; // Output power cannot be negative
    }
    currentOutput = powerOutput / voltageOutput;
  }

  // If the board does not get sufficient power the current sensor cannot work and values will be wrong
  if (system.isFatallyLowVoltage) {
    currentInput=0.0;
    currentOutput=0.0;
    powerInput = 0.0;
    powerOutput = 0.0;
  }

  sensors.timestamp = millis();
  sensors.voltageInput = voltageInput;
  sensors.voltageOutput = voltageOutput;
  sensors.currentInput = currentInput;
  sensors.currentOutput = currentOutput;
  sensors.powerInput = powerInput;
  sensors.powerOutput = powerOutput;
  sensors.temperature = temperature;
  sensors.currentSensorVoltage = currentSensorVoltage;

  return sensors;
}

/**
 * This is the main function in this file, called on every `loop()` cycle.
 */
void readSensors(SensorData &sensors, SystemState &system, SensorCalibration &calibration){
  sensors = getSensorData(calibration.currentSensorMidpointVoltage, system);
}
