#include "StateStore.h"

#include <Arduino.h>

#include "StorageKeys.h"
#include "../config/HardwareConfig.h"
#include "../core/Controller.h"
#include "../core/StateCodec.h"

namespace dryguard {

bool StateStore::begin() {
  hasSaved_ = false;
  ready_ = preferences_.begin(storage_keys::kCurrentNamespace, false);
  return ready_;
}

SavedState StateStore::load() {
  SavedState state = defaultState(config::kSettings);
  hasSaved_ = false;
  if (!ready_) return state;

  if (preferences_.isKey(storage_keys::kStateKey)) {
    StateRecord record{};
    if (preferences_.getBytesLength(storage_keys::kStateKey) == sizeof(record) &&
        preferences_.getBytes(storage_keys::kStateKey, &record, sizeof(record)) == sizeof(record) &&
        decodeState(record, config::kSettings, state)) {
      lastSaved_ = state;
      hasSaved_ = true;
      Serial.println("[NVS] Checkpoint loaded");
    } else {
      Serial.println("[NVS] Invalid checkpoint: system starts OFF at the safe default");
    }
    return state;
  }

  bool previousV3Present = false;
  if (readPreviousV3(state, previousV3Present)) {
    state.enabled = false;
    Serial.println("[NVS] Previous checkpoint imported; power remains OFF until verified");
    return state;
  }
  if (previousV3Present) {
    Serial.println("[NVS] Invalid previous checkpoint: system starts OFF at the safe default");
    return state;
  }

  if (readLegacyFields(storage_keys::kLegacyV2Namespace, state) ||
      readLegacyFields(storage_keys::kLegacyV1Namespace, state)) {
    state.enabled = false;
    Serial.println("[NVS] Legacy estimate imported; verify physical position before V0 ON");
  } else {
    Serial.println("[NVS] First start: place rack inside before V0 ON");
  }
  return state;
}

bool StateStore::readPreviousV3(SavedState& state, bool& present) {
  present = false;
  Preferences previous;
  if (!previous.begin(storage_keys::kPreviousV3Namespace, true)) return false;
  present = previous.isKey(storage_keys::kStateKey);
  if (!present) {
    previous.end();
    return false;
  }

  StateRecord record{};
  const bool complete = previous.getBytesLength(storage_keys::kStateKey) == sizeof(record) &&
      previous.getBytes(storage_keys::kStateKey, &record, sizeof(record)) == sizeof(record);
  previous.end();
  return complete && decodeState(record, config::kSettings, state);
}

bool StateStore::readLegacyFields(const char* name, SavedState& state) {
  Preferences legacy;
  if (!legacy.begin(name, true)) return false;
  if (!legacy.isKey(storage_keys::kLegacyPositionKey)) {
    legacy.end();
    return false;
  }

  SavedState candidate = {
      legacy.getInt(storage_keys::kLegacyPositionKey, 0),
      legacy.getInt(storage_keys::kLegacyTargetKey, 0),
      Mode::Idle, false, false};
  const bool automatic = legacy.getBool(storage_keys::kLegacyAutomaticModeKey, true);
  const bool manual = legacy.getBool(storage_keys::kLegacyManualModeKey, false);
  candidate.mode = automatic ? Mode::Automatic : manual ? Mode::Manual : Mode::Idle;
  candidate.motionRequested = candidate.mode != Mode::Idle &&
                              candidate.target != candidate.position;
  if (legacy.getBool(storage_keys::kLegacyResumeFlagKey, false)) {
    const int destination = legacy.getInt(storage_keys::kLegacyResumeTargetKey, -1);
    if (destination != 0 && destination != 1) {
      legacy.end();
      return false;
    }
    candidate.position = legacy.getInt(storage_keys::kLegacyResumePositionKey, -1);
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
  if (preferences_.putBytes(storage_keys::kStateKey, &record, sizeof(record)) != sizeof(record)) {
    return false;
  }
  lastSaved_ = state;
  hasSaved_ = true;
  return true;
}

}  // namespace dryguard
