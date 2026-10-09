#include "Application.h"

#include <Arduino.h>
#include <stdlib.h>

#include "../config/HardwareConfig.h"
#include "../diagnostics/Diagnostics.h"

namespace dryguard {

Application::Application() : controller_(config::kSettings), buttons_(config::kButtonDebounceMs) {}

bool Application::begin() {
  if (!store_.begin()) {
    Serial.println("[FATAL] Could not open NVS storage");
    return false;
  }
  const SavedState restored = store_.load();
  controller_.restore(restored);
  snapshot_.position = restored.position;
  snapshot_.target = restored.position;
  lastSavedPosition_ = restored.position;
  Serial.printf("[STATE] estimate=%ld target=%ld power=%d mode=%d\n",
      static_cast<long>(restored.position), static_cast<long>(restored.target),
      controller_.state().enabled, static_cast<int>(restored.mode));
  Serial.println("[STATE] Startup OFF; verify physical position before V0 ON");
  sensors_.begin();
  if (!motor_.begin(restored.position)) {
    Serial.println("[FATAL] Could not start motor task/queues");
    return false;
  }
  return xTaskCreatePinnedToCore(taskEntry, "DryGuardApp", config::kApplicationStackSize,
                               this, 1, &task_, 0) == pdPASS;
}

void Application::taskEntry(void* instance) {
  static_cast<Application*>(instance)->taskLoop();
}

void Application::noteStateChange(const SavedState& before) {
  if (!sameState(before, controller_.state())) saveRequested_ = true;
}

void Application::handleCommand(CommandType type, int value) {
  const SavedState before = controller_.state();
  switch (type) {
    case CommandType::Power: controller_.setEnabled(value == 1); break;
    case CommandType::Automatic: controller_.setAutomatic(value == 1, snapshot_.position); break;
    case CommandType::Manual: controller_.setManual(value == 1, snapshot_.position); break;
    case CommandType::Inside:
    case CommandType::Outside:
      if (value != 1 || !controller_.state().enabled || !buttons_.accept(millis())) return;
      if (type == CommandType::Inside) controller_.moveInside();
      else controller_.moveOutside();
      break;
  }
  noteStateChange(before);
}

bool Application::publishMotion() {
  const MotionRequest desired = controller_.motionRequest();
  if (requestPublished_ && sameRequest(desired, lastRequest_)) return true;
  const uint32_t nextSequence = commandSequence_ + 1;
  if (!motor_.request(desired, nextSequence)) {
    Serial.println("[ERROR] Motor command could not be queued");
    return false;
  }
  commandSequence_ = nextSequence;
  lastRequest_ = desired;
  requestPublished_ = true;
  saveRequested_ = true;
  return true;
}

void Application::saveCheckpoint(uint32_t now) {
  // Wait for the motor to acknowledge the latest command before saving its snapshot.
  if (!requestPublished_ || snapshot_.commandSequence != commandSequence_) return;
  if (previouslyMoving_ && !snapshot_.moving) saveRequested_ = true;
  previouslyMoving_ = snapshot_.moving;
  const bool periodic = static_cast<uint32_t>(now - lastSavedAt_) >= config::kCheckpointIntervalMs &&
      labs(static_cast<long>(snapshot_.position - lastSavedPosition_)) >= config::kCheckpointStepDelta;
  if (!saveRequested_ && !periodic) return;
  if (retrySave_ && static_cast<uint32_t>(now - lastSaveAttempt_) < 1000) return;
  lastSaveAttempt_ = now;
  if (store_.save(controller_.checkpoint(snapshot_.position))) {
    saveRequested_ = false;
    retrySave_ = false;
    lastSavedAt_ = now;
    lastSavedPosition_ = snapshot_.position;
  } else {
    retrySave_ = true;
    Serial.println("[NVS] Checkpoint write failed; retry in 1 second");
  }
}

void Application::taskLoop() {
  readings_ = sensors_.sample();
  lastSensorRead_ = millis();
  controller_.updateSensors(readings_);
  publishMotion();
  network_.begin(*this);
  for (;;) {
    const uint32_t now = millis();
    motor_.snapshot(snapshot_);
    if (static_cast<uint32_t>(now - lastSensorRead_) >= config::kSensorIntervalMs) {
      readings_ = sensors_.sample();
      lastSensorRead_ = now;
    }
    network_.poll(now);
    const SavedState before = controller_.state();
    // This also evaluates weather immediately after a power/mode callback.
    controller_.updateSensors(readings_);
    noteStateChange(before);
    if (publishMotion()) saveCheckpoint(millis());
    const DashboardState dashboard = {controller_.state(), readings_, snapshot_};
    network_.publish(dashboard, millis());
    if (static_cast<uint32_t>(now - lastDiagnostics_) >= config::kDiagnosticsIntervalMs) {
      printMemoryDiagnostics();
      Serial.printf("[SENSOR] rain=%d light=%d raining=%d dark=%d\n",
                    readings_.rainRaw, readings_.lightRaw, readings_.raining, readings_.dark);
      lastDiagnostics_ = now;
    }
    vTaskDelay(pdMS_TO_TICKS(10) > 0 ? pdMS_TO_TICKS(10) : 1);
  }
}

}  // namespace dryguard
