#include "Controller.h"

namespace jemuran {

SavedState defaultState(const Settings& settings) {
  // A new/invalid position estimate must be checked before enabling the system.
  return {settings.insidePosition, settings.insidePosition, Mode::Automatic, false, false};
}

bool validState(const SavedState& state, const Settings& settings) {
  const bool modeValid = state.mode == Mode::Idle || state.mode == Mode::Automatic ||
                         state.mode == Mode::Manual;
  return modeValid && state.position >= settings.insidePosition &&
         state.position <= settings.outsidePosition &&
         state.target >= settings.insidePosition && state.target <= settings.outsidePosition &&
         !(state.mode == Mode::Idle && state.motionRequested);
}

Controller::Controller(const Settings& settings)
    : settings_(settings), state_(defaultState(settings)) {}

void Controller::restore(const SavedState& state) {
  state_ = validState(state, settings_) ? state : defaultState(settings_);
}

void Controller::updateSensors(const SensorReadings& sensors) {
  if (!state_.enabled || state_.mode != Mode::Automatic) return;
  state_.target = (!sensors.valid || sensors.raining || sensors.dark)
                      ? settings_.insidePosition
                      : settings_.outsidePosition;
  state_.motionRequested = true;
}

void Controller::setEnabled(bool enabled) { state_.enabled = enabled; }

void Controller::hold(Mode mode, int32_t position) {
  state_.mode = mode;
  state_.target = position < settings_.insidePosition ? settings_.insidePosition
      : position > settings_.outsidePosition ? settings_.outsidePosition : position;
  state_.motionRequested = false;
}

void Controller::setAutomatic(bool enabled, int32_t position) {
  if (!state_.enabled) return;
  if (enabled) state_.mode = Mode::Automatic;
  else if (state_.mode == Mode::Automatic) hold(Mode::Idle, position);
}

void Controller::setManual(bool enabled, int32_t position) {
  if (!state_.enabled) return;
  if (enabled) hold(Mode::Manual, position);
  else if (state_.mode == Mode::Manual) hold(Mode::Idle, position);
}

bool Controller::moveTo(int32_t target) {
  if (!state_.enabled) return false;
  state_.mode = Mode::Manual;
  state_.target = target;
  state_.motionRequested = true;
  return true;
}

bool Controller::moveInside() { return moveTo(settings_.insidePosition); }
bool Controller::moveOutside() { return moveTo(settings_.outsidePosition); }

MotionRequest Controller::motionRequest() const {
  return {state_.enabled && state_.mode != Mode::Idle && state_.motionRequested, state_.target};
}

SavedState Controller::checkpoint(int32_t actualPosition) const {
  SavedState saved = state_;
  saved.position = actualPosition;
  if (!saved.motionRequested) saved.target = actualPosition;
  return saved;
}

}  // namespace jemuran
