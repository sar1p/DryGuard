#pragma once

#include <AccelStepper.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "../core/Types.h"

namespace jemuran {

// After begin(), only taskLoop() may access stepper_.
class MotorService {
 public:
  MotorService();
  bool begin(int32_t initialPosition);
  bool request(const MotionRequest& request, uint32_t sequence);
  bool snapshot(MotorSnapshot& value) const;

 private:
  struct Command { MotionRequest request; uint32_t sequence; };
  static void taskEntry(void* instance);
  void taskLoop();
  void publishSnapshot(uint32_t sequence);
  AccelStepper stepper_;
  QueueHandle_t commands_ = nullptr;
  QueueHandle_t snapshots_ = nullptr;
  TaskHandle_t task_ = nullptr;
};

}  // namespace jemuran
