# Pin Mapping Table: SentinEL-V & VSDSquadron Ultra

The following table details the minimal hardware connections between the SentinEL-V edge AI sensors and the VSDSquadron Ultra (THEJAS32 RISC-V SoC). All visual and audio alerting is handled via the UART-connected Python Command Center dashboard.

| Component / Module | VSDSquadron Ultra Pin | Pin Type | Operating Voltage | Function / Description |
| :--- | :---: | :---: | :---: | :--- |
| **NTC Thermistor** (Leg 1) | `3.3V` | Power | 3.3V | Supplies stable voltage to the thermal voltage divider. |
| **NTC Thermistor** (Leg 2)* | `A0` | Analog Input | 0 - 3.3V | Captures micro-fluctuations in battery cell temperature. |
| **MQ-135 Gas Sensor** (VCC) | `5V` | Power | 5.0V | Powers the internal heater of the gas sensor. |
| **MQ-135 Gas Sensor** (AOUT) | `A1` | Analog Input | 0 - 5.0V | Reads VOC / electrolyte venting chemical signatures. |
| **Common Ground** | `GND` | Ground | 0V | Common ground for the thermistor divider and gas sensor. |

> **\* Note on NTC Wiring:** The NTC thermistor is wired in a voltage divider configuration with a $10k\Omega$ fixed resistor. The junction between the NTC and the $10k\Omega$ resistor connects to Analog Pin `A0`, while the other end of the resistor connects to `GND`.

### Hardware Communication / Telemetry
| Interface | Pins | Function |
| :--- | :--- | :--- |
| **USB-C (UART)** | Internal | Streams 115200 baud telemetry (10Hz) to the Python Command Center dashboard for real-time risk assessment and fault logging. |