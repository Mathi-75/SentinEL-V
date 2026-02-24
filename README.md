## Project Title
**SentinEL-V: Predictive Edge AI Battery Safety System v2.0**

### Theme Selected
EV Battery Safety & Thermal Runaway Prediction

---

## 1. Problem Statement

Electric Vehicle (EV) battery fires, primarily caused by thermal runaway, occur in milliseconds and have devastating consequences. Traditional Battery Management Systems (BMS) are typically reactive—they trigger alarms only after the battery has reached an absolutely critical temperature (e.g., 60°C), which is often too late to prevent a fire. Furthermore, relying on cloud-based processing for anomaly detection introduces fatal latency.

SentinEL-V addresses this by moving predictive Machine Learning directly to the Edge. By monitoring the *acceleration* of heat (thermal velocity, $dT/dt$) and early-stage electrolyte gas venting, the system predicts and flags a runaway event *before* it reaches critical temperatures, ensuring zero-latency, localized safety actuation.

---

## 2. System Overview

* **Compute Platform:** VSDSquadron ULTRA (THEJAS32 RISC-V SoC)
* **Sensors Used:** 
    * NTC Thermistor (with 10kΩ voltage divider) for thermal tracking.
  * MQ-135 VOC Gas Sensor for early electrolyte vent detection.
* **Interfaces Used:** 
    * ADC: For continuous NTC (`A0`) and MQ-135 (`A1`) sampling.
  * GPIO: Active buzzer (`Pin 6`) and status LEDs.
  * UART: 115200 baud serial telemetry for the Command Center dashboard.
* **Edge Processing Performed:** Digital Kalman Filtering for noise elimination, dynamic environmental baseline calibration, rolling-window feature extraction ($dT/dt$), and local execution of a trained Decision Tree Classifier. Also features EEPROM-based Non-Volatile State of Health (SoH) tracking.
* **Dashboard:** A local, dark-mode Python TkAgg Command Center GUI that visualizes UART telemetry, calculating a real-time Risk Score (0-100), Time-to-Failure (TTF) countdown, and maintaining a forensic CSV Black Box log.

---

## 3. What Runs on VSDSquadron ULTRA

* **Sampling frequency:** 2Hz (500ms intervals) continuous sampling.
* **Signal conditioning:** Hardware ADC values are mathematically converted into standard physical units (ADC → Voltage Ratio → NTC Resistance → Kelvin → Celsius).
* **Filtering method:** A digital Kalman filter is applied in real-time to both the NTC and MQ-135 data streams to aggressively smooth micro-voltage fluctuations and eliminate hardware noise, preventing false positives.
* **Detection / estimation logic:**
    1. Maintains a rolling history array to calculate $dT/dt$.  
  2. Compares extracted features against a deterministic Decision Tree Classifier (trained via `scikit-learn` on synthetic runaway data).  
  3. Evaluates fault persistence (Debounce Latch) to trigger a SCRAM alert.  
* **Output generated:** Actuates local GPIO (Buzzer/LEDs) and streams a comma-separated telemetry packet via UART: `DATA, timestamp, temp, ror, gas_raw, class_id, state, risk_score, ttf_sec, soh_pct, baseline_ready`.

---

## 4. Measured Results Summary

| Test Case        | Expected Behavior | Observed Result |
| ---------------- | ----------------- | --------------- |
| **Normal Operation** | System stabilizes room temp/gas baselines. Green heartbeat LED pulses. Dashboard reads "SAFE". | Kalman filter successfully held $dT/dt$ at ~0.00. System maintained SAFE state without false alarms. |
| **Fault Condition (Gas / Venting)** | Spraying unlit butane near MQ-135 should trigger immediate shutdown regardless of temperature. | Gas ADC spiked >150 points above baseline. AI classified as CRITICAL. TTF dropped to 0, Buzzer triggered instantly. |
| **Stress Test (Thermal Acceleration)** | Applying rapid heat (soldering iron) should trigger an alarm *before* absolute temp reaches 45°C. | $dT/dt$ slope exceeded 1.5. AI classified as CRITICAL. System SCRAM latched successfully while absolute temp was still safely below 40°C. EEPROM recorded stress event, dropping overall SoH %. |

---

## 5. Repository Guide

* **`1_Project_Overview/`** → Contains system architecture documentation and overall block diagrams.
* **`2_Hardware/`** → Bill of Materials (BoM), wiring/circuit diagrams, and pin mapping for the VSDSquadron Ultra.
* **`3_Firmware/`** → Contains `sentinelv_firmware.ino` (the C++ embedded implementation).
* **`4_Algorithms/`** → Documentation on the Kalman Filter, $dT/dt$ mathematics, and Decision Tree logic. Also includes `train_model.py` which generated the ML model.
* **`5_Data/`** → Contains the `sentinelv_log_*.csv` "Black Box" forensic datasets automatically generated during our fault tests.
* **`6_Validation/`** → Details our calibration period logic, test methodologies (soldering iron/gas), and final results.
* **`7_Demo/`** → Contains high-res dashboard screenshots, hardware photos, and the demo video link.
* **`8_Future_Scope/`** → Plans for implementing SentinEL-V across multi-cell automotive battery packs via CAN bus.

---

### 6. Demo Video
