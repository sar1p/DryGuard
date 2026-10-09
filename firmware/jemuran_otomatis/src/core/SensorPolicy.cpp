#include "SensorPolicy.h"

namespace jemuran {

SensorReadings SensorPolicy::update(int rainRaw, int lightRaw) {
  if (rainRaw < 0 || rainRaw > 4095 || lightRaw < 0 || lightRaw > 4095) {
    initialized_ = false;
    return {rainRaw, lightRaw, true, true, false};
  }

  if (!initialized_) {
    raining_ = rainRaw < settings_.rainThreshold;
    dark_ = lightRaw > settings_.darkThreshold;
    initialized_ = true;
  } else {
    // Alarm thresholds remain immediate; clearing requires a stable margin.
    if (rainRaw < settings_.rainThreshold) raining_ = true;
    if (rainRaw >= settings_.rainThreshold + settings_.hysteresis) raining_ = false;
    if (lightRaw > settings_.darkThreshold) dark_ = true;
    if (lightRaw <= settings_.darkThreshold - settings_.hysteresis) dark_ = false;
  }
  return {rainRaw, lightRaw, raining_, dark_, true};
}

}  // namespace jemuran
