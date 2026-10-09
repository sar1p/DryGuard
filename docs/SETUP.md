# Setup, Build, and Upload

## PlatformIO on Windows

Prerequisites: Python 3.12, Git, and internet access to download dependencies. Run these commands from the repository root in PowerShell:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements-dev.txt
Copy-Item firmware\jemuran_otomatis\src\Secrets.example.h firmware\jemuran_otomatis\src\Secrets.h
```

Fill in `Secrets.h` before building firmware for the device:

```cpp
#define BLYNK_TEMPLATE_ID "TMPL_YOUR_TEMPLATE"
#define BLYNK_TEMPLATE_NAME "DryGuard"
#define BLYNK_AUTH_TOKEN "YOUR_DEVICE_TOKEN"
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

Keep this file local. Run `git check-ignore firmware\jemuran_otomatis\src\Secrets.h` to confirm that the ignore rule is active. If the file is not yet prepared, the project can still be compiled with the blank example credentials; Wi-Fi/Blynk will be disabled at runtime.

Build first:

```powershell
.\.venv\Scripts\python.exe -m platformio run -e esp32dev
```

The expected result is `SUCCESS`, with firmware in `.pio/build/esp32dev/`. Upload only after checking the board, wiring, and initial position. Replace `COM5` with the correct device port:

```powershell
.\.venv\Scripts\python.exe -m platformio device list
.\.venv\Scripts\python.exe -m platformio run -e esp32dev -t upload --upload-port COM5
.\.venv\Scripts\python.exe -m platformio device monitor --port COM5 --baud 115200
```

The `esp32dev` target is for a dual-core classic ESP32. The label “ESP32D” does not identify a complete board model; match the exact board type before uploading. ESP32 devices with different cores or pins require their own configuration.

## Arduino IDE

1. Install the **esp32 by Espressif Systems** board package; the PlatformIO build is pinned to Arduino ESP32 core 2.0.17. Use the same version for the initial check.
2. Install the **Blynk 1.3.2** and **AccelStepper 1.64.0** libraries.
3. Open `firmware/jemuran_otomatis/jemuran_otomatis.ino`. Arduino compiles `.cpp` files in `src/` recursively.
4. Prepare `src/Secrets.h`, select ESP32 Dev Module and the correct port, then run Verify before Upload.

The `.ino` sketch is intentionally short because `setup()` and `loop()` are in `src/main.cpp`. Do not copy the two legacy sketches into a new sketch folder, since each has its own entry point.

## Blynk Dashboard

Create the following datastreams in the device template and connect each widget to its corresponding virtual pin:

| Virtual pin | Type | Function / widget |
|---|---|---|
| V0 | Integer, 0–1 | Power, switch |
| V1 | Integer, 0–1 | Automatic mode, switch |
| V2 | Integer, 0–1 | Manual mode, switch |
| V3 | Integer, 0–1 | Retract, push button |
| V4 | Integer, 0–1 | Extend, push button |
| V5 | String | Weather: RAIN/DRY/OFF/READING |
| V6 | String | Light: DARK/BRIGHT/-/READING |
| V7 | String | Position/movement: `RETRACTED`, `EXTENDED`, `PARTIAL`, `EXTENDING >>`, `<< RETRACTING`, `SYSTEM OFF` |

Device state is authoritative after reconnect. On each boot/restart, V0 is published as OFF; the saved mode and target are retained. Power ON activates the saved mode after the position is checked; if the mode is Idle, select V1 or V2/V3/V4.

On first boot, the default mode is automatic; V0 ON may immediately request movement based on the sensors. V1/V2 change the mode only while the system is ON. If a mode is selected while OFF, the firmware corrects the switch at the next dashboard interval.

V3/V4 select manual mode automatically. A manual command can still extend the clothesline in the rain; select V1 to restore automatic sensor-based protection.

## Calibration and Wiring

Pin assignments and initial values are in `src/config/HardwareConfig.h`. Monitor ADC values in the `[SENSOR]` log and match the thresholds to actual sensor conditions. Polarity from the legacy sketch is retained: lower rain readings mean rain, and higher light readings mean darkness.

Power the ULN2003/motor from a 5 V supply as specified for the device, with a common ground to the ESP32. The voltage at ESP32 ADC inputs must stay within the ESP32 device limits; do not assume that a sensor module's analog output equals its supply voltage. See the troubleshooting guide for power-supply and mechanism checks.

## References

- [PlatformIO — ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
- [Arduino — sketch specification](https://docs.arduino.cc/arduino-cli/sketch-specification/)
- [Espressif — Preferences](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html)
- [AccelStepper — class reference](https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html)
- [Blynk — data transmission limits and recommendations](https://docs.blynk.io/en/blynk-library-firmware-api/limitations-and-recommendations)
