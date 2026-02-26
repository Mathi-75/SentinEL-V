# SentinEL-V — Scaling Strategy & Future Scope

## 1. Current System Snapshot

| Parameter | Current Value |
|---|---|
| **Board** | VSDSquadron ULTRA (THEJAS32 RISC-V @ 100 MHz) |
| **Sensors** | NTC Thermistor (thermal) + MQ-135 (VOC/gas) |
| **AI Model** | Decision Tree Classifier (sklearn-trained, C-exported) |
| **Filtering** | 1D Kalman Filter per channel |
| **Sampling Rate** | 2 Hz |
| **Outputs** | GPIO (LED + Buzzer) + UART telemetry |
| **Dashboard** | Python/matplotlib local laptop |
| **Detection Lead Time** | Minutes before thermal runaway |
| **False Positive Guard** | 200 ms debounce + Hard Latch |

The current prototype is a **single-cell, single-node** safety monitor. The scaling strategy below defines how SentinEL-V evolves from this point into a production-grade automotive BMS.

---

## 2. Scaling Strategy

**Goal:** Make the single-node system robust, calibrated, and demo-ready for real battery packs.

#### 2.1 Multi-Cell Support
Currently SentinEL-V monitors one cell. Real EV packs contain 96–200+ cells arranged in modules. The first scaling step is to expand the sensing architecture:

- Add a **multiplexed ADC** (e.g., ADS1115 with I2C addressing) to read up to 8 NTC thermistors per board
- Deploy **one SentinEL-V node per battery module** (typically 12–16 cells per module)
- Implement **inter-cell differential temperature** monitoring: if any two cells diverge by more than 5°C, flag an internal short circuit — a failure mode invisible to single-sensor systems

#### 2.2 Hardware Hardening
- Replace the breadboard prototype with a **custom 2-layer PCB** designed in KiCad
- Add **TVS diodes** on all analog input lines for ESD protection (automotive requirement)
- Use **JST connectors** for sensor wiring instead of dupont jumpers
- Encase in an **IP54-rated enclosure** for dust and splash resistance
- Add a **hardware watchdog timer** (external, e.g., TPL5010) as a backup to the firmware watchdog

#### 2.3 Sensor Calibration Pipeline
- Develop a **factory calibration script** that runs at first boot, characterises each NTC thermistor individually, and burns correction coefficients into EEPROM
- Cross-validate MQ-135 readings against a **reference gas sensor** (e.g., SGP30 or Sensirion SEN55) to establish accuracy bounds
- Generate a **per-unit calibration certificate** stored in EEPROM (unit ID, cal date, coefficients)

#### 2.4 Firmware Maturity
- Port codebase from Arduino IDE to **PlatformIO** for reproducible builds and dependency management
- Add **unit tests** using the Unity testing framework for all classifier logic
- Implement **MISRA-C subset compliance** checks using `cppcheck` — required for any automotive software
- Add **CRC32 checksums** on EEPROM data to detect corruption

---

## 3. Future Scope

### 3.1 Hardware Upgrades

#### Differential Temperature Sensing (ΔT Monitoring)
Deploy a **second NTC thermistor on the opposite face** of each battery cell. A divergence of more than 5°C between the two sensors indicates an internal short circuit — a failure mode that surface temperature alone cannot detect. This doubles the sensing coverage with a component cost of under ₹20 per cell.

#### Current Sensor Integration (Coulomb Counting)
Add an **ACS712 Hall-effect current sensor** in series with the battery. Integrate current over time to track State-of-Charge (SoC) alongside the existing SoH. This transforms SentinEL-V from a safety-only device into a true Battery Management System with both safety and capacity intelligence in one unit.

#### Pressure Sensor
Li-ion cells swell before they vent. A **BMP388 barometric pressure sensor** mounted in a sealed enclosure around the cell can detect this swelling (manifesting as a pressure increase) **before** gas actually vents — adding a third early-warning modality. This is currently used only in research-grade BMS systems and would be genuinely novel for a student project.

#### Acoustic Emission Detection
Li-ion cells emit **ultrasonic stress waves** (20–100 kHz) during lithium plating and dendrite formation — internal processes that precede failure. A piezoelectric element used as a contact microphone, coupled to an ADC, can detect these signatures. This third sensing modality would enable SentinEL-V to catch failure modes that are invisible to thermal and chemical sensors.

#### Hardware Security Module (HSM)
For production automotive deployment, add an **ATECC608B crypto chip** via I2C. This provides:
- Secure key storage for OTA update signing
- Unique device identity for fleet authentication
- Tamper-evident audit logs (critical for insurance/liability purposes)

---

### 3.2 AI & Algorithm Enhancements

#### TinyML: On-Device Model Evolution
Move from a static, pre-trained Decision Tree to an **online learning model** that adapts to the specific aging characteristics of the battery it is monitoring. Use **Hoeffding Trees** (an incremental decision tree algorithm suitable for streaming data) to continuously refine classification boundaries as the battery ages. The model weights are stored in EEPROM and evolve over the battery's lifetime.

#### Anomaly Detection with Isolation Forest
Train an **Isolation Forest** on "normal" battery operation data (collected during Phase 1 calibration). Any operating point that the model scores as an anomaly triggers a PRE-WARNING, even if it doesn't match any known fault pattern. This gives SentinEL-V the ability to detect **novel, unknown failure modes** — a genuine step beyond threshold-based and trained-class-based systems.

#### Remaining Useful Life (RUL) Prediction
Use the cumulative stress event history stored in EEPROM to train a **gradient-boosted regression model** (offline, on the fleet dashboard) that predicts Remaining Useful Life in charge cycles. Surface this as a "Replace battery in ~340 cycles" recommendation in the fleet dashboard — transforming the system from reactive safety monitoring to proactive maintenance scheduling.

#### Second-Order Derivative (Thermal Jerk)
The current system uses dT/dt (thermal velocity). Adding **d²T/dt²** (thermal acceleration, or "thermal jerk") as a third feature captures the *rate of change of the rate of change* — a much earlier warning signature. A sudden positive jerk means the thermal runaway process is accelerating, even if the absolute rate of rise is still low. This was described in the original project PDF but not yet implemented.

#### Sensor Fusion with Bayesian Networks
Replace the Decision Tree with a **Dynamic Bayesian Network** that explicitly models the probabilistic causal relationship between sensor readings and fault states. Unlike a Decision Tree (which gives a hard class boundary), a DBN outputs a calibrated probability of failure — enabling risk-proportionate responses (e.g., "73% probability of runaway in the next 2 minutes") rather than binary alerts.

---

### 3.3 Communication & Connectivity

#### ISO 26262 Functional Safety Compliance
Design a **redundant safety channel**: a second independent microcontroller (e.g., STM32 in a dedicated safety domain) receives the same sensor data and independently computes a simplified threshold-based check. If the two systems disagree on the safety state, a hardware comparator triggers a failsafe shutdown. This dual-channel architecture is required for **ASIL-B (Automotive Safety Integrity Level B)** certification under ISO 26262.

#### V2G (Vehicle-to-Grid) Safety Handshake
As EVs increasingly participate in V2G energy trading, SentinEL-V can act as a **safety gatekeeper**: before accepting a V2G discharge command, it checks the current SoH and temperature state. If either is marginal, it negotiates a reduced power level with the grid operator via the ISO 15118 communication protocol — preventing grid operators from inadvertently stressing degraded batteries.

#### Mesh Networking for Battery Rack Monitoring
In stationary energy storage applications (second-life batteries in solar installations), multiple SentinEL-V nodes can form a **Zigbee or Thread mesh network**. Each node monitors one battery module; the mesh collectively elects a coordinator that aggregates data and communicates with the BMS server. This requires no additional hardware beyond a CC2652 radio module.

---

### 3.4 Software & Dashboard

#### Digital Twin Integration
Create a **physics-based digital twin** of the monitored battery cell using PyBaMM (Python Battery Mathematical Modelling). The digital twin runs on the cloud dashboard and:
- Simulates expected temperature and voltage curves given the current charge/discharge profile
- Compares simulation output against real sensor data
- Flags deviations (real > simulated) as early indicators of degradation or internal faults
- Allows "what-if" scenario testing before a maintenance intervention

#### Mobile Application
Develop a **React Native app** (iOS + Android) that connects to the vehicle's MQTT gateway over Wi-Fi/BLE. Features:
- Live risk score dial on the home screen
- Push notifications for state changes (graded: PRE-WARNING = yellow, CRITICAL = red emergency alert)
- Historical SoH trend chart with "expected replacement date"
- One-tap fault report export (PDF + CSV) for sharing with service centres

---

## 4. Impact Metrics

### Safety Impact
- **Target:** Reduce EV battery fire incidents by 80% in fleets where SentinEL-V is deployed, by providing actionable warnings with sufficient lead time for safe vehicle evacuation and fire suppression system activation.
- **Benchmark:** Current NFPA data indicates thermal runaway in Li-ion cells progresses from onset to fire in as little as 90 seconds. SentinEL-V's 2–5 minute prediction window provides a 2–4× safety margin.

### Economic Impact
- **Per-vehicle cost of system:** Estimated ₹3,500–₹5,000 in BOM cost at prototype scale; ₹800–₹1,200 at 10,000-unit production volume.
- **Cost of an EV battery fire:** Average replacement cost ₹4–8 lakhs per vehicle; insurance + liability claims often exceed ₹50 lakhs per incident. SentinEL-V breaks even on the first prevented incident per 40–60 vehicles deployed.
- **SoH-based second-life revenue:** A battery with a documented, trusted SoH history (from SentinEL-V logs) commands a **15–25% premium** in the second-life battery market compared to undocumented packs.

### Strategic / National Impact
- Built entirely on **indigenous RISC-V silicon** (VSDSquadron ULTRA / THEJAS32), contributing to India's semiconductor self-reliance goals under the **India Semiconductor Mission**
- Provides a domestic alternative to imported BMS chipsets (currently dominated by Texas Instruments, Renesas, and Infineon), reducing foreign exchange outflow in the EV supply chain
- Aligns with **FAME-II** and **PM e-DRIVE** policy goals for safe, affordable EV adoption in India

---
