#pragma once

#include "Types.h"

namespace jemuran {

// Fixed-width record: magic, schema, position, target, mode, power, intent, checksum.
struct StateRecord { uint32_t words[8]; };
StateRecord encodeState(const SavedState& state);
bool decodeState(const StateRecord& record, const Settings& settings, SavedState& state);

}  // namespace jemuran
