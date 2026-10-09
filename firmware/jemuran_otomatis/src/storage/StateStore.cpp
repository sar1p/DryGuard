#include "StateStore.h"

#include <Arduino.h>

#include "../config/HardwareConfig.h"
#include "../core/Controller.h"
#include "../core/StateCodec.h"

namespace jemuran {

bool StateStore::begin() {
  ready_ = preferences_.begin("jemuran_v3", false);
  return ready_;
}

SavedState StateStore::load() {
  SavedState state = defaultState(config::kSettings);
  if (!ready_) return state;
  if (preferences_.isKey("state")) {
    StateRecord record{};
    if (preferences_.getBytesLength("state") == sizeof(record) &&
        preferences_.getBytes("state", &record, sizeof(record)) == sizeof(record) &&
        decodeState(record, config::kSettings, state)) {
      lastSaved_ = state;
      hasSaved_ = true;
      Serial.println("[NVS] Checkpoint loaded");
    } else {
      Serial.println("[NVS] Invalid checkpoint: system starts OFF at position estimate 0");
    }
    return state;
  }

  if (readLegacy("jemuran_v2", state) || readLegacy("jemuran", state)) {
    Serial.println("[NVS] Legacy estimate imported; verify physical position before V0 ON");
  } else {
    Serial.println("[NVS] First start: place rack inside before V0 ON");
  }
  return state;
}

bool StateStore::readLegacy(const char* name, SavedState& state) {
  Preferences legacy;
  if (!legacy.begin(name, true)) return false;
  const bool exists = legacy.isKey("posisi");
  if (!exists) {
    legacy.end();
    return false;
  }

  SavedState candidate = {legacy.getInt("posisi", 0), legacy.getInt("target", 0),
      Mode::Idle, false, false};
  const bool automatic = legacy.getBool("modeAuto", true);
  const bool manual = legacy.getBool("modeManual", false);
  candidate.mode = automatic ? Mode::Automatic : manual ? Mode::Manual : Mode::Idle;
  candidate.motionRequested = candidate.mode != Mode::Idle && candidate.target != candidate.position;
  if (legacy.getBool("res_flag", false)) {
    const int destination = legacy.getInt("res_tgt", -1);
    if (destination != 0 && destination != 1) {
      legacy.end();
      return false;
    }
    candidate.position = legacy.getInt("res_pos", -1);
    candidate.target = destination == 0 ? config::kSettings.insidePosition
                                      : config::kSettings.outsidePosition;
    candidate.mode = Mode::Manual;
    candidate.motionRequested = true;
  }
  legacy.end();
  if (!validState(candidate, config::kSettings)) return false;
  state = candidate;
  return true;
}

bool StateStore::save(const SavedState& state) {
  if (!ready_ || !validState(state, config::kSettings)) return false;
  if (hasSaved_ && sameState(state, lastSaved_)) return true;
  const StateRecord record = encodeState(state);
  if (preferences_.putBytes("state", &record, sizeof(record)) != sizeof(record)) return false;
  lastSaved_ = state;
  hasSaved_ = true;
  return true;
}

}  // namespace jemuran
