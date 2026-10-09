#pragma once

#include "../core/Types.h"

namespace dryguard {
namespace config {

constexpr uint8_t kMotorIn1 = 13;
constexpr uint8_t kMotorIn2 = 12;
constexpr uint8_t kMotorIn3 = 14;
constexpr uint8_t kMotorIn4 = 27;
constexpr uint8_t kRainPin = 34;
constexpr uint8_t kLightPin = 35;

// Positions, ADC polarity, speed and acceleration match the original sketch.
constexpr Settings kSettings = {0, 4000, 2800, 3400, 100};
constexpr float kMotorMaxSpeed = 1000.0f;
constexpr float kMotorAcceleration = 1000.0f;
constexpr uint32_t kSensorIntervalMs = 200;
constexpr uint32_t kDashboardIntervalMs = 1000;
constexpr uint32_t kWifiRetryMs = 10000;
constexpr uint32_t kBlynkRetryMs = 5000;
constexpr uint32_t kBlynkConnectTimeoutMs = 250;
constexpr uint32_t kCheckpointIntervalMs = 5000;
constexpr int32_t kCheckpointStepDelta = 50;
constexpr uint32_t kButtonDebounceMs = 300;
constexpr uint32_t kDiagnosticsIntervalMs = 60000;
constexpr uint32_t kMotorStackSize = 4096;
constexpr uint32_t kApplicationStackSize = 10240;

static_assert(kSettings.insidePosition < kSettings.outsidePosition,
              "Motor travel must have a positive range");
static_assert(kSettings.hysteresis >= 0, "Hysteresis cannot be negative");

}  // namespace config
}  // namespace dryguard
