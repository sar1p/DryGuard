#include "MotorService.h"

#include "../config/HardwareConfig.h"

namespace dryguard {

MotorService::MotorService()
    : stepper_(AccelStepper::HALF4WIRE, config::kMotorIn1, config::kMotorIn3,
               config::kMotorIn2, config::kMotorIn4, false) {}

bool MotorService::begin(int32_t initialPosition) {
  commands_ = xQueueCreate(1, sizeof(Command));
  snapshots_ = xQueueCreate(1, sizeof(MotorSnapshot));
  if (!commands_ || !snapshots_) return false;
  stepper_.setMaxSpeed(config::kMotorMaxSpeed);
  stepper_.setAcceleration(config::kMotorAcceleration);
  stepper_.setCurrentPosition(initialPosition);
  stepper_.enableOutputs();
  publishSnapshot(0);
  return xTaskCreatePinnedToCore(taskEntry, "DryGuardMotor", config::kMotorStackSize,
                               this, 2, &task_, 1) == pdPASS;
}

bool MotorService::request(const MotionRequest& request, uint32_t sequence) {
  if (!commands_) return false;
  const Command command = {request, sequence};
  // A single-slot queue holds the newest complete desired state.
  return xQueueOverwrite(commands_, &command) == pdPASS;
}

bool MotorService::snapshot(MotorSnapshot& value) const {
  return snapshots_ && xQueuePeek(snapshots_, &value, 0) == pdPASS;
}

void MotorService::publishSnapshot(uint32_t sequence) {
  const MotorSnapshot value = {static_cast<int32_t>(stepper_.currentPosition()),
      static_cast<int32_t>(stepper_.targetPosition()), stepper_.isRunning(), sequence};
  xQueueOverwrite(snapshots_, &value);
}

void MotorService::taskEntry(void* instance) {
  static_cast<MotorService*>(instance)->taskLoop();
}

void MotorService::taskLoop() {
  uint32_t sequence = 0;
  uint32_t lastSnapshot = 0;
  bool previouslyMoving = false;
  for (;;) {
    Command command;
    const bool received = xQueueReceive(commands_, &command, 0) == pdPASS;
    if (received) {
      sequence = command.sequence;
      const long position = stepper_.currentPosition();
      if (!command.request.move) {
        // Freeze at the actual position; leave the pending destination in Controller.
        stepper_.setCurrentPosition(position);
      } else {
        const long target = constrain(command.request.target,
            config::kSettings.insidePosition, config::kSettings.outsidePosition);
        if ((stepper_.speed() > 0 && target <= position) ||
            (stepper_.speed() < 0 && target >= position)) {
          // Restart acceleration when reversing, avoiding braking beyond a travel end.
          stepper_.setCurrentPosition(position);
        }
        stepper_.moveTo(target);
      }
    }

    const long position = stepper_.currentPosition();
    if ((position <= config::kSettings.insidePosition && stepper_.speed() < 0) ||
        (position >= config::kSettings.outsidePosition && stepper_.speed() > 0)) {
      const long target = stepper_.targetPosition();
      stepper_.setCurrentPosition(position);
      stepper_.moveTo(target);
    }
    stepper_.run();

    const uint32_t now = millis();
    const bool moving = stepper_.isRunning();
    if (received || moving != previouslyMoving || static_cast<uint32_t>(now - lastSnapshot) >= 20) {
      publishSnapshot(sequence);
      lastSnapshot = now;
    }
    previouslyMoving = moving;
    // At least one tick lets the idle task run, including non-1-ms tick builds.
    vTaskDelay(pdMS_TO_TICKS(1) > 0 ? pdMS_TO_TICKS(1) : 1);
  }
}

}  // namespace dryguard
