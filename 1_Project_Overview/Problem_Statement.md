## Problem Statement

Electric Vehicle (EV) battery fires, primarily caused by thermal runaway, occur in milliseconds and have devastating consequences. Traditional Battery Management Systems (BMS) are typically reactive—they trigger alarms only after the battery has reached an absolutely critical temperature (e.g., 60°C), which is often too late to prevent a fire. Furthermore, relying on cloud-based processing for anomaly detection introduces fatal latency.

SentinEL-V addresses this by moving predictive Machine Learning directly to the Edge. By monitoring the *acceleration* of heat (thermal velocity, $dT/dt$) and early-stage electrolyte gas venting, the system predicts and flags a runaway event *before* it reaches critical temperatures, ensuring zero-latency, localized safety actuation.

