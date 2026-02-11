
/**
 * This is the main function in this file, called on every `loop()` cycle.
 */
void onboardTelemetry(const SensorData &sensors, const SensorCalibration &calibration, const SystemState &system, const ControlState &control, const PwmState &pwm, const TelemetryData &telemetry){    

  static unsigned long previousSerialMillis = 0.0;
  unsigned long currentSerialMillis = millis();

  /////////////////////// USB SERIAL DATA TELEMETRY ////////////////////////   
  // 0 - Disable Serial
  // 1 - Display All
  // 2 - Display Essential Data
  // 3 - Display Numbers Only 

  if(currentSerialMillis-previousSerialMillis>=SERIAL_INTERVAL_MS){
    previousSerialMillis = currentSerialMillis;                         //Store previous time

    if(SERIAL_TELEM_MODE==TELEM_OFF){}
    else if(SERIAL_TELEM_MODE==TELEM_ALL){

      // Move to new line
      Serial.println();

      // Sensor Data
      //Serial.print(" ST:");    Serial.print(sensors.timestamp); 
      Serial.print(" VI:");    Serial.print(sensors.voltageInput, 2);
      Serial.print(" VO:");    Serial.print(sensors.voltageOutput, 2);  
      Serial.print(" CI:");    Serial.print(sensors.currentInput, 2);  
      Serial.print(" CO:");    Serial.print(sensors.currentOutput, 2); 
      Serial.print(" PI:");    Serial.print(sensors.powerInput, 2); 
      Serial.print(" PO:");    Serial.print(sensors.powerOutput, 2); 
      Serial.print(" T:");     Serial.print(sensors.temperature, 1);
      Serial.print(" CSV:");   Serial.print(sensors.currentSensorVoltage, 3); 

      // Sensor Calibration
      Serial.print(" CSMV:");  Serial.print(calibration.currentSensorMidpointVoltage, 3);
      
      // System State
      Serial.print(" FLV:");   Serial.print(system.isFatallyLowVoltage);
      Serial.print(" IUV:");   Serial.print(system.isInputUnderVoltage);
      Serial.print(" IOC:");   Serial.print(system.isInputOverCurrent);    
      Serial.print(" OOV:");   Serial.print(system.isOutputOverVoltage);      
      Serial.print(" OOC:");   Serial.print(system.isOutputOverCurrent);
      Serial.print(" IBOV:");  Serial.print(system.isInputBelowOutputVoltage); 
      Serial.print(" OT:");    Serial.print(system.isOverTemperature); 
      Serial.print(" BNC:");   Serial.print(system.isBatteryNotConnected); 
      Serial.print(" PS:");    Serial.print(system.powerSource); 

      // Control State
      Serial.print(" IE:");    Serial.print(control.isInputEnabled); 
      Serial.print(" BE:");    Serial.print(control.isBuckEnabled); 
      Serial.print(" FE:");    Serial.print(control.isFanEnabled); 

      // PWM State
      Serial.print(" DC:");    Serial.print(pwm.dutyCycle); 
      Serial.print(" IDC:");   Serial.print(pwm.idealDutyCycle); 
      Serial.print(" MiDC:");  Serial.print(pwm.minDutyCycle); 
      Serial.print(" MaDC:");  Serial.print(pwm.maxDutyCycle); 
      Serial.print(" FDC:");   Serial.print(pwm.fullScaleDutyCycle); 

      // Telemetry Data
      Serial.print(" WH:");    Serial.print(telemetry.wattHours, 0); 
      Serial.print(" LT:");    Serial.print(telemetry.loopTimeMs, 0);
      Serial.print(" TO:");    Serial.print(telemetry.timeOn);  
    }
    else if(SERIAL_TELEM_MODE==TELEM_NUMBERS_ONLY){

      // Move to new line
      Serial.println();

      // Sensor Data
      Serial.print(sensors.timestamp);                              Serial.print(",");
      Serial.print(sensors.voltageInput, 2);                        Serial.print(",");
      Serial.print(sensors.voltageOutput, 2);                       Serial.print(",");
      Serial.print(sensors.currentInput, 2);                        Serial.print(",");
      Serial.print(sensors.currentOutput, 2);                       Serial.print(",");
      Serial.print(sensors.powerInput, 2);                          Serial.print(",");
      Serial.print(sensors.powerOutput, 2);                         Serial.print(",");
      Serial.print(sensors.temperature, 1);                         Serial.print(",");
      Serial.print(sensors.currentSensorVoltage, 3);                Serial.print(",");

      // Sensor Calibration
      Serial.print(calibration.currentSensorMidpointVoltage, 3);    Serial.print(",");
      
      // System State
      Serial.print(system.isFatallyLowVoltage);                     Serial.print(",");
      Serial.print(system.isInputUnderVoltage);                     Serial.print(",");
      Serial.print(system.isInputOverCurrent);                      Serial.print(",");
      Serial.print(system.isOutputOverVoltage);                     Serial.print(",");
      Serial.print(system.isOutputOverCurrent);                     Serial.print(",");
      Serial.print(system.isInputBelowOutputVoltage);               Serial.print(",");
      Serial.print(system.isOverTemperature);                       Serial.print(",");
      Serial.print(system.isBatteryNotConnected);                   Serial.print(",");
      Serial.print(system.powerSource);                             Serial.print(",");

      // Control State
      Serial.print(control.isInputEnabled);                         Serial.print(",");
      Serial.print(control.isBuckEnabled);                          Serial.print(",");
      Serial.print(control.isFanEnabled);                           Serial.print(",");

      // PWM State
      Serial.print(pwm.dutyCycle);                                  Serial.print(",");
      Serial.print(pwm.idealDutyCycle);                             Serial.print(",");
      Serial.print(pwm.minDutyCycle);                               Serial.print(",");
      Serial.print(pwm.maxDutyCycle);                               Serial.print(",");
      Serial.print(pwm.fullScaleDutyCycle);                         Serial.print(",");

      // Telemetry Data
      Serial.print(telemetry.wattHours, 0);                         Serial.print(",");
      Serial.print(telemetry.loopTimeMs, 0);                        Serial.print(",");
      Serial.print(telemetry.timeOn);                               Serial.print(",");
    }  
    else if(SERIAL_TELEM_MODE==TELEM_ESSENTIAL){
    }  
  } 
}
