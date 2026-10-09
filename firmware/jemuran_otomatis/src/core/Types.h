#pragma once

#include <stdint.h>

namespace jemuran {

enum class Mode : uint8_t { Idle = 0, Automatic = 1, Manual = 2 };

struct Settings {
  int32_t insidePosition;
  int32_t outsidePosition;
  int rainThreshold;
  int darkThreshold;
  int hysteresis;
};

struct SavedState {
  int32_t position;
  int32_t target;
  Mode mode;
  bool enabled;
  bool motionRequested;
};

struct SensorReadings {
  int rainRaw;
  int lightRaw;
  bool raining;
  bool dark;
  bool valid;
};

struct MotionRequest {
  bool move;
  int32_t target;
};

struct MotorSnapshot {
  int32_t position;
  int32_t target;
  bool moving;
  uint32_t commandSequence;
};

inline bool sameState(const SavedState& a, const SavedState& b) {
  return a.position == b.position && a.target == b.target && a.mode == b.mode &&
         a.enabled == b.enabled && a.motionRequested == b.motionRequested;
}

inline bool sameRequest(const MotionRequest& a, const MotionRequest& b) {
  // A hold request stops at the motor task's actual position, not a stale snapshot.
  return a.move == b.move && (!a.move || a.target == b.target);
}

}  // namespace jemuran
