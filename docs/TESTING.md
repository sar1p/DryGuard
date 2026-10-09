# Testing

## Automated Tests

```powershell
python tools/run_tests.py
```

The runner requires GCC or Clang on `PATH`. If the compiler is in another location, set `CXX` to the compiler executable path. The tests compile the **same C++ code used by the firmware**, rather than a Python reimplementation.

The 20 core tests cover OFF at startup/restart, automatic weather response, hysteresis including recovery after an invalid ADC reading, mode changes, manual override, command rejection while OFF, pause/resume after enabling, the full `4000` target, position-hold checkpoints, corrupted records, and debounce across `millis()` rollover.

The 5 gateway tests compile the firmware's `BlynkGateway.cpp` together with its controller. Arduino, Wi-Fi, and Blynk API stubs record published values to check rejected switch corrections while OFF, send intervals, input callbacks, and refresh after reconnect. Source copies and dummy headers are created only in `build/native/gateway/`; the runner does not read or overwrite your private `Secrets.h`.

The 8 storage tests compile the firmware's `StateStore.cpp` against Arduino and Preferences test doubles. They cover current-format record loading with controller startup OFF, unrelated namespace isolation, corrupt checksum and wrong-length/type rejection, valid save/load round-trips, rejection of invalid state without overwriting saved data, initialization failure, and short-write retry and deduplication. The three suites contain 33 tests total: 20 core, 5 gateway, and 8 storage.

GitHub Actions runs two checks:

1. **Native C++ tests**: compile with C++11, treat warnings as errors, and run the core, gateway, and StateStore suites.
2. **ESP32 firmware build**: build with PlatformIO using pinned dependencies, then check configurations with networking disabled and with dummy network settings in a local header.

On Windows, after setting up PlatformIO, `.\.venv\Scripts\python.exe tools/check_configured_build.py` checks the network-enabled variant with dummy credentials. The tool refuses to overwrite an existing `Secrets.h` and removes the dummy header it created after the build. The resulting binary uses dummy data; rebuild with your device credentials before uploading.

The gateway stubs check firmware logic against minimal replacement APIs; the behavior of the actual Blynk library/server, RTOS scheduler, ESP32 flash, power supply, and motor mechanism still requires separate testing. The firmware build checks compile/link compatibility; neither check proves hardware behavior. No physical hardware tests have been verified yet.

## Device Test Matrix

Every scenario below requires physical testing; do not mark it as passed just because CI succeeded.

| Scenario | Expected result |
|---|---|
| First boot or upgrade with only an older checkpoint | Estimate is 0 and system is OFF; align the rack with the retracted position before V0 ON, then select V1 for automatic mode |
| Dry and bright | Target is extended; movement completes without a stall or reset |
| Rain while extending | Target changes to retracted; the mechanism does not continue past its mechanical limit |
| Dark | Automatic mode targets retracted |
| Sensor reading near threshold | Direction does not keep changing due to small amounts of noise |
| V1/V2 ON while V0 is OFF | Motor remains held and the mode switch is corrected to OFF at the next dashboard interval |
| Manual mode and V3/V4 | Mode is correct, button returns to zero, direction matches the wiring |
| V0 OFF during movement | Movement stops; position is saved after the hold is acknowledged |
| V0 ON after a manual pause | The previous target resumes; estimate matches the actual position |
| Reboot during movement | Checkpoint is read and system is OFF; measure the difference between the estimate and physical position before V0 ON |
| Wi-Fi/Blynk disconnects and reconnects | Local logic continues running; dashboard updates on reconnect |
| Light load through normal-use load | Record reset reason, supply voltage, driver/motor temperature, and missed steps |

Increase load gradually within the mechanism's capability. If the estimate differs from the actual position, stop testing and reconcile the position before continuing. Limit switches/encoders and a homing procedure are not yet available in this project.
