# DryGuard — ESP32 and Blynk Automatic Clothesline

DryGuard is automatic clothesline firmware with a rain sensor, a light sensor, a 5 V stepper motor, and a ULN2003 driver. Automatic mode retracts the clothesline when it rains or gets dark, then extends it when conditions are dry and bright. Manual mode provides retract/extend commands through Blynk.

The firmware separates code by responsibility, uses a FreeRTOS queue for motor control, stores checkpoints through Preferences/NVS, and provides automated builds and tests.

## Target hardware

- Classic ESP32 with two cores; build target `esp32dev` / ESP32 Dev Module. The user refers to the device as "ESP32D"; the full board model has not been confirmed.
- Four-phase 5 V stepper motor with a ULN2003 driver, matching the existing hardware configuration.
- Analog rain and light sensors, sampled with 12-bit ADC resolution.
- Blynk with virtual pins V0–V7.

| Device | GPIO |
|---|---:|
| ULN2003 IN1 | 13 |
| ULN2003 IN2 | 12 |
| ULN2003 IN3 | 14 |
| ULN2003 IN4 | 27 |
| Rain sensor — analog | 34 |
| Light sensor — analog | 35 |

The motor constructor order is **IN1, IN3, IN2, IN4**. The retracted position is `0`, the extended position is `4000`, the rain threshold is `< 2800`, and the darkness threshold is `> 3400`. These are initial values from the project code, not the result of a new mechanical calibration.

## Project structure

```text
firmware/dryguard/
  dryguard.ino             # Arduino IDE entry sketch
  src/
    main.cpp               # brief setup/loop
    Secrets.example.h      # example network configuration
    config/                # pins, calibration, intervals, task sizes
    core/                  # pure C++ logic and checkpoint format
    app/                   # application orchestration
    motor/                 # single task that owns AccelStepper
    sensors/               # ADC readings and filtering
    storage/               # Preferences/NVS checkpoint storage
    iot/                   # Wi-Fi, Blynk, dashboard
    diagnostics/           # reset reason and heap status
tests/                     # core, Blynk gateway, and storage regression tests
tools/                     # test runner
docs/                      # setup, architecture, testing, troubleshooting
.github/workflows/ci.yml    # ESP32 build and tests on GitHub Actions
platformio.ini             # pinned platform/library versions
```

## Getting started

1. Copy `firmware/dryguard/src/Secrets.example.h` to `Secrets.h` in the same folder, then fill in the Blynk template, token, SSID, and Wi-Fi password. `Secrets.h` is ignored by Git.
2. Prepare the dashboard according to the [Blynk virtual pin table](docs/SETUP.md#blynk-dashboard).
3. Build with PlatformIO or open `firmware/dryguard/dryguard.ino` in the Arduino IDE. There is no need to change the `.cpp` file extension.
4. The system starts **OFF** on every boot/restart. A valid current checkpoint retains the saved target, but position is only an estimate. Before V0 ON, compare the estimate with the physical position. If no valid current checkpoint exists, including an upgrade with only an older checkpoint, the estimate starts at `0`; align the rack with the retracted position before enabling V0. On first boot, the default mode is automatic.
5. After V0 ON, select automatic mode through V1, or manual mode through V2/V3/V4. Mode commands issued while OFF are ignored, and the dashboard switch is restored to the device status.

Full guides: [setup and build](docs/SETUP.md), [architecture](docs/ARCHITECTURE.md), [testing](docs/TESTING.md), and [restart troubleshooting](docs/TROUBLESHOOTING.md).

## Main improvements

- The motor object is accessed only by the motor task; the application task sends targets/holds through a queue and reads snapshots.
- Power OFF stops movement while preserving a pending target. The resume target is not overwritten by the boot process; after restart, movement resumes when V0 is turned ON after the position is checked.
- Automatic/manual modes use a single enum, so both cannot be active at the same time.
- Median sampling and hysteresis reduce direction changes caused by noise around sensor thresholds. The previous alarm still uses a clearing margin after an invalid sample.
- Storage uses a single record with a schema, checksum, and range validation. Motion checkpoints are written no more frequently than every five seconds, in addition to command/status changes and the end of movement.
- Dashboard updates are sent when values change, at one-second intervals. Power/mode switches are also corrected after a command is received, including mode commands rejected while OFF. Device state is republished on reconnect.
- Boot prints the reset reason; heap and sensor diagnostics are printed every minute.

## Testing and position limits

Firmware counts steps; the current project code has no limit switch input or encoder. The position estimate can drift because of missed steps, movement while power is off, or steps lost since the last checkpoint. Resume does not replace physical homing.

Builds and logic tests do not prove that restarts under full load have been eliminated. Testing the power supply, driver, mechanism, and firmware on the device follows the [testing guide](docs/TESTING.md). CI checks an empty configuration and a dummy network. CI artifacts use dummy credentials; create your own build with `Secrets.h` for a Blynk connection.
