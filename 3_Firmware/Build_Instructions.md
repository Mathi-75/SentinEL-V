# Build and Flash Instructions: SentinEL-V Firmware

This guide outlines the steps to compile and upload the SentinEL-V Edge AI firmware to the VSDSquadron Ultra (THEJAS32 RISC-V SoC) using the Arduino IDE.

## Prerequisites

1. **Arduino IDE:** Ensure you have the latest version of the Arduino IDE installed.
2. **VSDSquadron Board Support:** You must have the VSDSquadron Ultra board package installed in your Arduino IDE Boards Manager.
3. **USB Driver:** Ensure the CH340/CP210x USB-to-UART driver is installed on your system so the board is recognized.

## Project Structure Setup

Arduino IDE requires that the main `.ino` sketch and its associated header files live inside a folder bearing the exact same name as the sketch.

1. Create a new folder on your computer named `sentinelv_firmware`.
2. Move the following two files into that folder:
   * `sentinelv_firmware.ino` (The main C++ firmware)
   * `classifier_export.h` (The auto-generated Decision Tree logic)

## Flashing the Firmware

1. **Open the Project:** Double-click `sentinelv_firmware.ino` to open the project in the Arduino IDE. The IDE should open with two tabs visible at the top: one for the `.ino` file and one for the `.h` file.
2. **Connect the Hardware:** Plug the VSDSquadron Ultra into your computer via a USB-C data cable.
3. **Select the Board:** * Go to **Tools > Board**.
   * Select **VSDSquadron Ultra** (or THEJAS32, depending on your board manager naming convention).
4. **Select the Port:**
   * Go to **Tools > Port**.
   * Select the appropriate COM port (e.g., `COM3` on Windows or `/dev/ttyUSB0` on Linux/Ubuntu).
5. **Compile and Upload:**
   * Click the **Verify** (Checkmark) button to compile the code and ensure there are no syntax errors.
   * Click the **Upload** (Right Arrow) button to flash the compiled binary to the RISC-V core. 

## Verification

Once the upload is complete, the VSDSquadron Ultra will automatically reboot, and you will hear a 3-beep power-on sequence from the buzzer. 

**Important Note for the Command Center:** Do *not* open the Arduino Serial Monitor. The Python Command Center requires exclusive access to the COM port. Proceed to the `5_Data` or root folder and run `python dashboard.py` to view the live telemetry.