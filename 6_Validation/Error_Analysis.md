# Error Analysis & Edge Case Handling

![image](../images/warning.png)

In real-world EV battery environments, sensors are subjected to extreme electrical interference, physical vibrations, and ambient environmental changes. SentinEL-V is designed to handle these edge cases without triggering catastrophic false positive SCRAM events.

Based on the telemetry dashboard captured during live testing, we can analyze how the system mitigates errors:

### 1. Transient Thermal Noise (False Positive Mitigation)
Notice the event log at **T+127.8s**. The NTC thermistor picked up a sudden, brief fluctuation in heat (likely a physical disturbance or a localized gust of hot air). 
* **The Error:** A simple threshold-based system might have panicked and triggered an alarm. 
* **The Mitigation:** Because the AI evaluates the *sustained* mathematical derivative ($dT/dt$), it momentarily flagged a `[PRE-WARNING]` as the rate of rise crossed `0.3`, but safely decayed back to `[SAFE]` less than a second later at **T+128.5s**. This proves the system resists false positives caused by physical noise.

### 2. True Positive Lock-On (The Warning State)
At **T+160.0s**, a sustained thermal event is introduced. 
* The absolute temperature has barely risen from 25°C to 28°C (which is perfectly normal for a battery).
* However, the **Rate of Rise** immediately spikes to ~0.8 °C/sample. 
* The AI correctly identifies the *velocity* of the heat as a runaway signature, locking the system into a `WARNING` state and accurately estimating a 14-second Time-To-Failure (TTF) before the cell hits the critical 60°C threshold.

### 3. Kalman Phase Lag
* **Identified Limitation:** The 1D Digital Kalman filter utilized to clean the raw ADC micro-fluctuations introduces a slight computational phase lag. 
* **Impact:** There is a roughly 2-sample (~1.0 second) delay between the physical heat application and the graphical spike.
* **Resolution:** Because SentinEL-V detects runaway in the *predictive* phase (using $dT/dt$) rather than the *reactive* phase (waiting for absolute heat), this 1-second filter delay is heavily outweighed by the early-warning advantage.

### 4. MQ-135 Baseline Drift
* **Identified Limitation:** Chemiresistor gas sensors like the MQ-135 are highly sensitive to ambient humidity and weather changes. An absolute trigger value programmed in a dry lab will fail on a humid day.
* **Resolution:** SentinEL-V mitigates this completely using the 30-second Adaptive Baseline Calibration on boot. The AI calculates the gas delta against the dynamic baseline (shown as the green dotted line at ~260 ADC), rendering atmospheric drift irrelevant.