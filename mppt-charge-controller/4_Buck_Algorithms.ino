
/*
 * Enables the input path, used for backflow control.
 */
static bool inputEnable(){
  bool isInputEnabled = true;
  digitalWrite(BACKFLOW_MOSFET_PIN,HIGH);
  return isInputEnabled;
}

/*
 * Disables the input path, used for backflow control.
 */
static bool inputDisable(){
  bool isInputEnabled = false;
  digitalWrite(BACKFLOW_MOSFET_PIN,LOW);
  return isInputEnabled;
}

/*
 * Enables the buck converter and turns on the indicator LED.
 */
bool buckEnable(){
  bool isBuckEnabled = true;
  digitalWrite(BUCK_EN_PIN,HIGH);
  digitalWrite(LED_PIN,HIGH);
  return isBuckEnabled;
}

/*
 * Disables the buck converter, turns off the indicator LED, and resets the PWM duty cycle.
 */
bool buckDisable(){
  bool isBuckEnabled = false; 
  digitalWrite(BUCK_EN_PIN,LOW);
  digitalWrite(LED_PIN,LOW);
  return isBuckEnabled;
}

/*
 * Calculate ideal duty cycle
 */
static int getIdealPwmDutyCycle(const SensorData &sensors, const PwmState &pwm){
  float idealPwmDutyCycle = 0.0;

  if (sensors.voltageInput <= 0.0) {
    // Avoid division by zero if there is no input voltage.
    idealPwmDutyCycle = 0.0;
    return idealPwmDutyCycle;
  }

  // Calculate the ideal duty cycle from measured input and output voltage.
  idealPwmDutyCycle = (sensors.voltageOutput / sensors.voltageInput) * pwm.fullScaleDutyCycle;

  return idealPwmDutyCycle;
}

/*
 * Set a lower limit to the duty cycle to avoid reverse current through the low-side MOSFET.
 */
static int getMinPwmDutyCycle(const SensorData &sensors, const PwmState &pwm){
  // Steps by which the minimum duty cycle is set lower than the ideal value.
  const int MIN_PWM_STEPS_DOWN = 10;  
  int idealDutyCycle = 0;
  int minDutyCycle = 0;

  idealDutyCycle = getIdealPwmDutyCycle(sensors, pwm);

  // Create a floor for the duty cycle that is a number of steps below the ideal value.
  minDutyCycle = idealDutyCycle - MIN_PWM_STEPS_DOWN;

  // Ensure the calculated floor is within the absolute valid range.
  minDutyCycle = constrain(minDutyCycle, 0, pwm.maxDutyCycle);
  
  return minDutyCycle;
}   

/*
 * Applies the calculated PWM duty cycle to the buck converter.
 */
static void applyPwmDutyCycle(const int pwmDutyCycle){
  ledcWrite(BUCK_IN_PIN, pwmDutyCycle);
}

/**
 * Implements the Constant Current-Constant Voltage (CC-CV) algorithm for PSU input.
 */
static int runCcCvAlgorithm(const SensorData &sensors, const PwmState &pwm) {
  int pwmDutyCycle = pwm.dutyCycle;
  if (sensors.currentOutput > MAX_OUTPUT_CURRENT) {
    // Output current is too high, decrease duty cycle to reduce current.
    pwmDutyCycle--;
  } else if (sensors.voltageOutput > OUTPUT_VOLTAGE) {
    // Output voltage is too high, decrease duty cycle to reduce voltage.
    pwmDutyCycle--;
  } else if (sensors.voltageOutput < OUTPUT_VOLTAGE) {
    // Output voltage is below target, increase duty cycle to increase voltage.
    pwmDutyCycle++;
  }
  // If voltage and current are within limits, do nothing to the duty cycle.

  return pwmDutyCycle;
}

/**
 * Implements the MPPT algorithm for solar panel input.
 */
static int runMpptAlgorithm(const SensorData &sensors, const PwmState &pwm) {
  int pwmDutyCycle = pwm.dutyCycle;
  static float previousPowerInput = 0.0;
  static float previousVoltageInput = 0.0;
  
  // First, check for Constant Current (CC) and Constant Voltage (CV) limits.
  if (sensors.currentOutput > MAX_OUTPUT_CURRENT) {
    // Output current is too high (CC limit), decrease duty cycle to reduce current.
    pwmDutyCycle--;
  } else if (sensors.voltageOutput > OUTPUT_VOLTAGE) {
    // Output voltage is too high (CV limit), decrease duty cycle to reduce voltage.
    pwmDutyCycle--;
  } else {
    // --- Perturb and Observe (P&O) MPPT Algorithm ---
    // If not limited by CC or CV, track the maximum power point of the solar panel.
    // We compare the current power and voltage with the previous values to decide
    // how to adjust the PWM duty cycle. A change in duty cycle is a "perturbation".

    if (sensors.powerInput > previousPowerInput) {
      // Power is increasing. We are moving towards the MPP. Keep perturbing in the same direction.
      if (sensors.voltageInput > previousVoltageInput) {
        // ↑Power, ↑Voltage: We moved right on the P-V curve. Keep moving right (increase voltage).
        // Decreasing duty cycle increases input voltage.
        pwmDutyCycle--;
      } else {
        // ↑Power, ↓Voltage: We moved left on the P-V curve. Keep moving left (decrease voltage).
        // Increasing duty cycle decreases input voltage.
        pwmDutyCycle++;
      }
    } else if (sensors.powerInput < previousPowerInput) {
      // Power is decreasing. We have moved away from the MPP. Reverse the perturbation direction.
      if (sensors.voltageInput > previousVoltageInput) {
        // ↓Power, ↑Voltage: We moved too far right. Reverse: move left (decrease voltage).
        // Increasing duty cycle decreases input voltage.
        pwmDutyCycle++;
      } else {
        // ↓Power, ↓Voltage: We moved too far left. Reverse: move right (increase voltage).
        // Decreasing duty cycle increases input voltage.
        pwmDutyCycle--;
      }
    } else {
      // Power is constant. We might be at the MPP or stalled.
      // If output voltage is still below target, we can try to perturb the system to 
      // re-validate the MPP.
      if (sensors.voltageOutput < OUTPUT_VOLTAGE) {
        pwmDutyCycle++;
      }
    }
  }

  // Store current values for the next P&O iteration
  previousPowerInput = sensors.powerInput;
  previousVoltageInput = sensors.voltageInput;

  return pwmDutyCycle;
}

/**
 * This is the main function in this file, called on every `loop()` cycle.
 */
void buckAlgorithms(SensorData &sensors, SystemState &system, ControlState &control, PwmState &pwm){

  // Fetch ideal duty cycle
  pwm.idealDutyCycle = getIdealPwmDutyCycle(sensors, pwm);
 
  // Disable input path if input voltage is lower than output voltage to avoid backflow current.
  if (system.isInputBelowOutputVoltage) {
    control.isInputEnabled = inputDisable();    
    control.isBuckEnabled = buckDisable();
    pwm.dutyCycle = 0;  // Start again from zero
    return;
  }

  // Disable buck if fatally low voltage or over temperature
  if (system.isFatallyLowVoltage || system.isOverTemperature) {
    control.isBuckEnabled = buckDisable();
    pwm.dutyCycle = 0;  // Start again from zero
    return;
  }

  // Disable buck if input is under voltage
  if (system.isInputUnderVoltage) {
    control.isBuckEnabled = buckDisable();
    pwm.dutyCycle = pwm.idealDutyCycle;
    return;
  }

  if ( INPUT_MODE == PSU ){
    pwm.dutyCycle = runCcCvAlgorithm(sensors, pwm);
  }
  if ( INPUT_MODE == MPPT ) {
    pwm.dutyCycle = runMpptAlgorithm(sensors, pwm);
  }

  // Constrain PWM duty cycle

  pwm.minDutyCycle = getMinPwmDutyCycle(sensors, pwm);
  pwm.dutyCycle = constrain(pwm.dutyCycle, pwm.minDutyCycle, pwm.maxDutyCycle);

  // Apply the calculated PWM duty cycle to the hardware
  applyPwmDutyCycle(pwm.dutyCycle);

  // Make sure buck and input path are enabled.
  control.isBuckEnabled = buckEnable();
  control.isInputEnabled = inputEnable();
}
