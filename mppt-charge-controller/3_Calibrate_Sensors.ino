
/**
 * Calibrate the current sensor.
 */
static float getSensorCalibration(const SensorData &sensors, const SystemState &system, const SensorCalibration &calibration, const ControlState &control){
  // Use offset to calibrate the current sensor against a multimeter
  const float CURRENT_SENSOR_CALIBRATION_OFFSET = 0.003;
  float currentSensorMidpointVoltage = calibration.currentSensorMidpointVoltage;

  if(!control.isBuckEnabled && !system.isFatallyLowVoltage && !system.isOutputOverVoltage && !(system.powerSource==0)){
    currentSensorMidpointVoltage = sensors.currentSensorVoltage - CURRENT_SENSOR_CALIBRATION_OFFSET;
  }

  return currentSensorMidpointVoltage;
}

/**
 * This is the main function in this file, called on every `loop()` cycle.
 */
void calibrateSensors(SensorData &sensors, SensorCalibration &calibration, SystemState &system, ControlState &control){
  calibration.currentSensorMidpointVoltage = getSensorCalibration(sensors, system, calibration, control);
}
