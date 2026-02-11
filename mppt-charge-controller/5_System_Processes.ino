
/**
 * Manages the cooling fan based on temperature and user settings.
 */
static bool handleFanControl(bool isFanEnabled, float temperature) {
  if (!FAN_ENABLED) {
    isFanEnabled = false;
  } else if (FAN_ALWAYS_ON) {
    isFanEnabled = true;
  } else {
    // If the fan is currently on, only turn it off if the temperature
    // drops all the way to the lower OFF threshold.
    if (isFanEnabled && (temperature <= FAN_OFF_TEMPERATURE)) {
      isFanEnabled = false;
    }
    // If the fan is currently off, only turn it on if the temperature
    // rises all the way to the upper ON threshold.
    else if (!isFanEnabled && (temperature >= FAN_ON_TEMPERATURE)) {
      isFanEnabled = true;
    }
    // If the temperature is between the two thresholds, do nothing.
    // The fan's state remains unchanged.
  }
  digitalWrite(FAN_PIN, isFanEnabled);

  return isFanEnabled;
}

/**  
 * Calculate the duration of each main loop cycle.
 */
static long getLoopTime() {
  long loopTimeMs = 0.0;
  static unsigned long previousLoopStartTime = 0.0; // Static local, holds value between calls.
  unsigned long currentLoopStartTime = micros();

  // Calculate duration in milliseconds since the start of the last loop.
  // On the first run, this value will be incorrect, but it will be correct for all subsequent runs.
  loopTimeMs = (currentLoopStartTime - previousLoopStartTime) / 1000.0;

  // Save the current start time to be used in the next loop's calculation.
  previousLoopStartTime = currentLoopStartTime;
 
  return loopTimeMs;
}

/**
 * Calculate the new uptime based on the current uptime in second increments.
 */
static unsigned long calculateUptime(unsigned long currentUptime) {
  static unsigned long previousMillis = 0.0;
  unsigned long newUptime = currentUptime;

  if (millis() - previousMillis >= 1000.0) {
    previousMillis = millis();
    newUptime++;
  }

  return newUptime;
}

/**
 * Check if telemetry data needs to be reset.
 */
static bool shouldResetData(long timeOn) {
  const float SECONDS_PER_DAY = 86400.0;
  float daysRunning = timeOn / SECONDS_PER_DAY;
  bool shouldReset = false;

  // A switch statement when to reset variables.
  switch (RESET_MODE) {
    case RESET_DAILY:
      if (daysRunning > 1) shouldReset = true;
      break;
    case RESET_WEEKLY:
      if (daysRunning > 7) shouldReset = true;
      break;
    case RESET_MONTHLY:
      if (daysRunning > 30) shouldReset = true;
      break;
    case RESET_YEARLY:
      if (daysRunning > 365) shouldReset = true;
      break;
    case RESET_NEVER:
    default:
      // Do nothing.
      break;
  }
  return shouldReset;
}

/**
 * Calculates energy harvested in this interval
 */
static float calculateEnergyForInterval(const float powerInput){
  const float SECONDS_PER_HOUR = 3600.0;
  static unsigned long previousRoutineMillis = 0;
  unsigned long currentRoutineMillis = millis();
  float intervalInHours = 0.0;
  float energyThisInterval = 0.0;
  
  if(currentRoutineMillis-previousRoutineMillis>=ROUTINE_INTERVAL_MS){
    previousRoutineMillis = currentRoutineMillis;

    // Calculate energy (Watt-Hours) for this interval and add to total
    intervalInHours = (float)ROUTINE_INTERVAL_MS / (1000.0 * SECONDS_PER_HOUR);
    energyThisInterval = powerInput * intervalInHours;
  }

  return energyThisInterval;
}

/**
 * This is the main function in this file, called on every `loop()` cycle.
 */
void systemProcesses(SensorData &sensors, SensorCalibration &calibration, SystemState &system, ControlState &control, TelemetryData &telemetry) {

  // Control fan
  control.isFanEnabled = handleFanControl(control.isFanEnabled, sensors.temperature);  

  telemetry.loopTimeMs = getLoopTime();

  // Update telemetry data
  telemetry.timeOn = calculateUptime(telemetry.timeOn);
  telemetry.wattHours += calculateEnergyForInterval(sensors.powerInput);

  // Reset data
  if (shouldResetData(telemetry.timeOn)) {
    telemetry.wattHours = 0.0;
    telemetry.timeOn = 0.0;
  };

}
