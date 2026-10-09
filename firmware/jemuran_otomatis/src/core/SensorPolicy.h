#pragma once

#include "Types.h"

namespace jemuran {

class SensorPolicy {
 public:
  explicit SensorPolicy(const Settings& settings) : settings_(settings) {}
  SensorReadings update(int rainRaw, int lightRaw);

 private:
  Settings settings_;
  bool initialized_ = false;
  bool raining_ = false;
  bool dark_ = false;
};

}  // namespace jemuran
