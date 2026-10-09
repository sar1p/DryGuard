#pragma once

#include "../core/Controller.h"
#include "../core/Debouncer.h"
#include "../iot/BlynkGateway.h"
#include "../motor/MotorService.h"
#include "../sensors/SensorService.h"
#include "../storage/StateStore.h"

namespace jemuran {

class Application : public CommandSink {
 public:
  Application();
  bool begin();
  void handleCommand(CommandType type, int value) override;

 private:
  static void taskEntry(void* instance);
  void taskLoop();
  bool publishMotion();
  void saveCheckpoint(uint32_t now);
  void noteStateChange(const SavedState& before);
  Controller controller_;
  Debouncer buttons_;
  MotorService motor_;
  SensorService sensors_;
  StateStore store_;
  BlynkGateway network_;
  SensorReadings readings_{4095, 0, false, false, false};
  MotorSnapshot snapshot_{};
  MotionRequest lastRequest_{false, 0};
  bool requestPublished_ = false;
  bool saveRequested_ = true;
  bool previouslyMoving_ = false;
  bool retrySave_ = false;
  uint32_t commandSequence_ = 0;
  uint32_t lastSensorRead_ = 0;
  uint32_t lastSavedAt_ = 0;
  uint32_t lastSaveAttempt_ = 0;
  uint32_t lastDiagnostics_ = 0;
  int32_t lastSavedPosition_ = 0;
  TaskHandle_t task_ = nullptr;
};

}  // namespace jemuran
