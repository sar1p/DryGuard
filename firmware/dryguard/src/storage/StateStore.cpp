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

  if (!preferences_.isKey(storage_keys::kStateKey)) {
    Serial.println("[NVS] No checkpoint; system starts OFF at estimate 0");
    return state;
  }

  StateRecord record{};
  if (preferences_.getBytesLength(storage_keys::kStateKey) != sizeof(record) ||
      preferences_.getBytes(storage_keys::kStateKey, &record, sizeof(record)) != sizeof(record) ||
      !decodeState(record, config::kSettings, state)) {
    Serial.println("[NVS] Invalid checkpoint; system starts OFF at estimate 0");
    return defaultState(config::kSettings);
  }

  lastSaved_ = state;
  hasSaved_ = true;
  Serial.println("[NVS] Checkpoint loaded");
  return state;
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
