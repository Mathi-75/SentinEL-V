# Calibration Method: Adaptive Baseline Anchoring

![image](../images/safecondition.png)

To ensure SentinEL-V remains highly accurate across diverse operating environments—preventing a hot summer day from being falsely flagged as a thermal runaway—the system employs an adaptive 30-second baseline calibration upon every boot.

### The 30-Second Calibration Sequence

1. **Sensor Stabilization:** Upon powering the VSDSquadron Ultra, the MQ-135 internal heater and the NTC voltage divider require a brief moment to reach electrical equilibrium.
2. **Data Collection (The 60-Sample Window):** The system immediately begins sampling at 2Hz. For the first 30 seconds (exactly 60 samples), the Edge AI fault logic is strictly bypassed.
3. **Mathematical Anchoring:** The RISC-V core calculates the mathematical mean of these initial samples to establish two critical variables:
   * `temp_baseline`: The ambient environmental temperature.
   * `gas_baseline`: The ambient background VOC level.
4. **Kalman Filter Convergence:** During this window, the 1D Digital Kalman Filter algorithms converge, stripping out micro-voltage hardware noise. 

### Visual Verification

As shown in the Command Center telemetry above, a successful calibration is visually confirmed by several indicators:
* The **"✓ Baseline calibrated"** status indicator illuminates green.
* The absolute **Temperature** graph locks perfectly flat (e.g., ~25°C) without micro-fluctuations.
* The absolute **Gas / VOC** graph stabilizes (e.g., ~260 ADC).
* Most importantly, the **Rate of Rise ($dT/dt$)** graph drops to and hovers around exactly `0.0`, proving that the system has successfully anchored its thermal velocity calculations to the newly established baseline.

Once calibrated, the system transitions into active monitoring mode, where the Decision Tree Classifier begins evaluating $dT/dt$ and gas deltas against this established zero-point.

[def]: SentinEL-V/7_Demo/Dashboard_Screenshots/critical.png