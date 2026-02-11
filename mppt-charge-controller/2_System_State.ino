
/**
 * Set system state based on sensor values.
 */
static SystemState getSystemState(const SensorData &sensors){
  // Define hardware protection limits.
  const float MIN_SYSTEM_VOLTAGE = 10.0;
  const float ABSOLUTE_MAX_INPUT_CURRENT = 31.0;
  const float OUTPUT_OVERVOLTAGE_MARGIN = 0.6;
  const float ABSOLUTE_MAX_OUTPUT_CURRENT = 50.0;
  const float BUCK_DROPOUT_VOLTAGE = 1.0000;       // Min. difference input and output voltage

  SystemState system = {};

  if (sensors.voltageInput < MIN_SYSTEM_VOLTAGE && sensors.voltageOutput < MIN_SYSTEM_VOLTAGE) {
    system.isFatallyLowVoltage = true;
    system.activeConditions++;
  }
  if (sensors.voltageInput < OUTPUT_VOLTAGE + BUCK_DROPOUT_VOLTAGE) {
    system.isInputUnderVoltage = true;
    system.activeConditions++;
  }
  if (sensors.currentInput > ABSOLUTE_MAX_INPUT_CURRENT) {
    system.isInputOverCurrent = true;
    system.activeConditions++;
  }
  if (sensors.voltageOutput > OUTPUT_VOLTAGE + OUTPUT_OVERVOLTAGE_MARGIN) {
    system.isOutputOverVoltage = true;
    system.activeConditions++;
  }
  if (sensors.currentOutput > ABSOLUTE_MAX_OUTPUT_CURRENT) {
    system.isOutputOverCurrent = true;
    system.activeConditions++;
  }    
  if (sensors.voltageInput < sensors.voltageOutput + BUCK_DROPOUT_VOLTAGE) {
    system.isInputBelowOutputVoltage = true;
    system.activeConditions++;
  }  
  if (sensors.temperature > MAX_SYSTEM_TEMPERATURE) {
    system.isOverTemperature = true;
    system.activeConditions++;
  }
  if (sensors.voltageOutput < MIN_SYSTEM_VOLTAGE) {
    system.isBatteryNotConnected = true;
    system.activeConditions++;
  }

  // Check input source
  if(sensors.voltageInput<=3 && sensors.voltageOutput<=3){
    // System is only powered by USB port
    system.powerSource=0;                          
  }             
  else if(sensors.voltageInput>sensors.voltageOutput)    {
    // System is running on solar as power source
    system.powerSource=1;
  }             
  else if(sensors.voltageInput<sensors.voltageOutput)    {
    // System is running on batteries as power source
    system.powerSource=2;
  }             

  return system;
}

/**
 * This is the main function in this file, called on every `loop()` cycle.
 */
void determineSystemState(SensorData &sensors, SystemState &system){
  system = getSystemState(sensors);
}
