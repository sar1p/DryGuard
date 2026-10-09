# Architecture and data flow

## Component ownership

| Module | Responsibility | Runtime owner |
|---|---|---|
| `app/Application` | Coordination, commands, and checkpoint scheduling | application task, core 0 |
| `core/Controller` | Power, mode, motor destination, and pause/resume | application task |
| `core/SensorPolicy` | Sensor thresholds and hysteresis | application task |
| `core/StateCodec` | Encoding, schema, checksum, and record validation | application task / native tests |
| `motor/MotorService` | AccelStepper, target limits, and position snapshots | motor task, core 1 |
| `sensors/SensorService` | Median of five ADC samples, sampled every 200 ms | application task |
| `storage/StateStore` | Preferences/NVS reads and writes | application task |
| `iot/BlynkGateway` | Wi-Fi, reconnects, Blynk callbacks, and virtualWrite | application task |
| `diagnostics/Diagnostics` | Reset reason and heap statistics | setup / application task |

After initialization, only the motor task calls AccelStepper methods. The application sends `MotionRequest` through a single-slot queue. The latest message contains the complete requested state, so old commands do not accumulate. A separate queue shares snapshots without accessing the motor object from another core.

The command queue retains the latest desired state. A command replaced before the motor task receives it may be skipped; a rapid OFF-to-ON change does not guarantee two separately executed events.

Each motor command carries a sequence number. The application waits for a snapshot acknowledging that sequence before saving a command-change checkpoint. Power OFF therefore saves the position after the motor task applies hold, rather than a snapshot from before the motor stopped.

## Power and modes

- `Automatic`: rain **or** darkness requests retraction; dry **and** bright conditions request extension. Invalid sensor readings request retraction.
- `Manual`: V3/V4 select a destination directly. Manual commands override weather rules until automatic mode is selected again.
- `Idle`: no movement is requested.
- Power OFF is separate from mode. The mode and pending destination are retained, while the motor request becomes hold. Re-enabling automatic mode evaluates the current weather first.
- Every boot starts OFF, even if a saved record contains power ON. Mode and destination are retained, but V0 must be enabled after checking the physical position. Without position feedback, the firmware cannot establish where the motor stopped during an unexpected reset.
- V0 enables or disables motion logic. Hold does not disconnect driver power or automatically release holding-coil current.
- Selecting manual mode through V2 requests hold. Turning off the active mode also requests hold.

Hold uses the motor task's actual step count. A snapshot that may be several steps old is not used as the stopping destination.

## Storage and migration

The new namespace is `jemuran_v3`. Its `state` key contains a 32-byte record with position, target, mode, power, and motion intent. The schema version and checksum are checked before use. A single record avoids mixing values from separate key writes, but position accuracy still depends on step estimation and checkpoint timing.

Valid data from `jemuran_v2` or `jemuran` can be imported when no new record exists. Preferences resume flags from the experimental version are checked. The original EEPROM resume flag is not imported. Older namespaces are retained. Migration starts OFF because the two older sketches did not save power state consistently.

An invalid new record starts the system OFF; legacy data does not replace a corrupt new checkpoint. On first boot, the initial estimate is the retracted position `0`. Check the mechanism before enabling the system.

The application task writes flash when state changes, movement finishes, or a moving checkpoint meets the five-second interval and minimum change of 50 steps. Identical records are not rewritten. ESP32 flash operations may still affect system timing; separate tasks do not guarantee motion without jitter.

## Network and dashboard

Wi-Fi retries every 10 seconds. Blynk retries every five seconds with a `connect(250)` budget. The reconnect path does not contain `delay(1000)`. Connection work may still block the application task; the motor task runs separately.

All virtual pins are published from one location, with at most eight application values per one-second interval, including button resets. Reconnection invalidates the cache and refreshes the dashboard from device state. An older cloud V0 value does not automatically enable a device that is OFF.

Power and mode callbacks mark V0-V2 for publication at the next scheduled interval, even when local state is unchanged. V1/V2 commands rejected while OFF are therefore corrected to zero on the dashboard. Pending corrections survive disconnection or an interval that has not yet elapsed.

Pins, speed, acceleration, intervals, and calibration are configured in `src/config/HardwareConfig.h`. Changes require the relevant build and verification checks.
