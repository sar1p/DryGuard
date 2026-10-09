#pragma once

#include "../core/SensorPolicy.h"

namespace dryguard {

class SensorService {
 public:
  SensorService();
  void begin();
  SensorReadings sample();

 private:
  static int medianRead(uint8_t pin);
  SensorPolicy policy_;
};

}  // namespace dryguard
