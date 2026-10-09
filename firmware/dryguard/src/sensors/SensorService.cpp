#include "SensorService.h"

#include <algorithm>
#include <Arduino.h>

#include "../config/HardwareConfig.h"

namespace dryguard {

SensorService::SensorService() : policy_(config::kSettings) {}

void SensorService::begin() {
  pinMode(config::kRainPin, INPUT);
  pinMode(config::kLightPin, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(config::kRainPin, ADC_11db);
  analogSetPinAttenuation(config::kLightPin, ADC_11db);
}

int SensorService::medianRead(uint8_t pin) {
  int values[5];
  for (int& value : values) value = analogRead(pin);
  std::sort(values, values + 5);
  return values[2];
}

SensorReadings SensorService::sample() {
  return policy_.update(medianRead(config::kRainPin), medianRead(config::kLightPin));
}

}  // namespace dryguard
