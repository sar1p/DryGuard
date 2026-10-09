#pragma once

#include "Types.h"

namespace jemuran {

// This class has no Arduino/network dependency and is owned by the application task.
class Controller {
 public:
  explicit Controller(const Settings& settings);
  void restore(const SavedState& state);
  void updateSensors(const SensorReadings& sensors);
  void setEnabled(bool enabled);
  void setAutomatic(bool enabled, int32_t position);
  void setManual(bool enabled, int32_t position);
  bool moveInside();
  bool moveOutside();
  MotionRequest motionRequest() const;
  SavedState checkpoint(int32_t actualPosition) const;
  const SavedState& state() const { return state_; }

 private:
  bool moveTo(int32_t target);
  void hold(Mode mode, int32_t position);
  Settings settings_;
  SavedState state_;
};

bool validState(const SavedState& state, const Settings& settings);
SavedState defaultState(const Settings& settings);

}  // namespace jemuran
