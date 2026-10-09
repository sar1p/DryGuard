# Troubleshooting Resets and Movement Issues

## Collect Evidence from the Serial Monitor

Open the Serial Monitor at **115200 baud** before reproducing the condition that causes a restart. The new firmware prints the format below; this is an example format, not a measurement from a device:

```text
[BOOT] DryGuard firmware | reset=<REASON> (<CODE>) | SDK=<VERSION>
[DIAG] heap_free=<BYTES> bytes | heap_min=<BYTES> bytes
[STATE] estimate=<STEPS> target=<STEPS> power=<0/1> mode=<MODE>
```

Save the log from before the restart, including any panic/backtrace messages, and the boot lines that follow it. The reset reason is reported through `esp_reset_reason()` according to the [Espressif documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html#reset-reason).

| Reason | Initial meaning and next checks |
|---|---|
| `BROWNOUT` | The brownout detector reported low voltage; check the supply, cables, ground, and voltage while the motor is under load |
| `TASK_WATCHDOG` / `INTERRUPT_WATCHDOG` | A watchdog was triggered; capture the backtrace and inspect the function/task that blocked execution |
| `PANIC` | Capture the complete error message and backtrace; match them against the ELF from the build that was actually uploaded |
| `POWER_ON` / `EXTERNAL_RESET` | Check for power loss, connector issues, or a reset signal; this reason alone does not identify the full cause |
| `SOFTWARE_RESET` / `UNKNOWN` | Check the log before the reset; the new code does not schedule a reboot for Wi-Fi reconnects |

The reset reason helps with investigation, but it is not a voltage measurement or standalone proof that a particular component is faulty.

## 5 V Stepper Motor and ULN2003

Pin configuration and initial speeds follow the legacy project. If the motor misses steps under load, try a lower speed/acceleration in `HardwareConfig.h`, check for mechanical binding, and compare the estimated position with the physical position. Software step counting does not detect a stall.

`kMotorMaxSpeed` is the configured speed limit. Actual speed also depends on how often `run()` is called and on task scheduling; measure travel time on the device before assuming that this speed is reached.

Power the motor/driver from a 5 V supply as specified; do not power the motor from an ESP32 GPIO. The ESP32 and driver need a common ground. Compare operation with the motor stopped, moving without a load, and carrying a normal-use load, then measure the voltage when the symptom occurs. Code changes cannot fix a sagging supply or a jammed mechanism.

## Dashboard Does Not Connect

- Check that `src/Secrets.h` exists and that the token/SSID are filled in. The blank example intentionally disables networking.
- Check the template ID, device token, virtual pins, and Wi-Fi network against the Blynk configuration.
- Confirm that Serial shows a Wi-Fi connection followed by a Blynk connection; Blynk retries every five seconds with a bounded timeout.
- After reconnect, device state is sent again. A device power state of OFF is not replaced by a cloud switch that was previously ON.

## Position or Resume Does Not Match

A moving checkpoint is not written at every step. A sudden power loss can discard changes since the last checkpoint; mechanical shifts while powered off and missed steps are also undetected. Reconcile the physical position, then adjust calibration or add a homing mechanism in a future development change.

At every boot/restart, the firmware waits for V0 ON; read the `[STATE]` estimate and compare it with the physical position first. The legacy namespace is retained for traceability, but an older firmware rollback may read its own checkpoint, which no longer tracks movement under the new firmware.