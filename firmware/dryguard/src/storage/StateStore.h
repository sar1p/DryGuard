#pragma once

#include <Preferences.h>

#include "../core/Types.h"

namespace dryguard {

// Only the application task reads/writes this store; no flash access in the motor task.
class StateStore {
 public:
  bool begin();
  SavedState load();
  bool save(const SavedState& state);

 private:
  bool readPreviousV3(SavedState& state, bool& present);
  bool readLegacyFields(const char* name, SavedState& state);
  Preferences preferences_;
  bool ready_ = false;
  bool hasSaved_ = false;
  SavedState lastSaved_{};
};

}  // namespace dryguard
