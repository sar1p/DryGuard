#pragma once

#include "../core/SensorPolicy.h"

namespace jemuran {

class SensorService {
 public:
  SensorService();
  void begin();
  SensorReadings sample();

 private:
  static int medianRead(uint8_t pin);
  SensorPolicy policy_;
};

}  // namespace jemuran
