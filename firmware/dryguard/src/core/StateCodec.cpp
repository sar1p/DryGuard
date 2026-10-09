#include "StateCodec.h"

#include "Controller.h"

namespace dryguard {
namespace {
constexpr uint32_t kMagic = 0x4A454D33;
constexpr uint32_t kSchema = 1;
uint32_t checksum(const StateRecord& record) {
  uint32_t result = 2166136261u;
  for (unsigned i = 0; i < 7; ++i) {
    for (unsigned byte = 0; byte < 4; ++byte) {
      result ^= (record.words[i] >> (byte * 8)) & 0xffu;
      result *= 16777619u;
    }
  }
  return result;
}
}  // namespace

StateRecord encodeState(const SavedState& state) {
  StateRecord record = {{kMagic, kSchema, static_cast<uint32_t>(state.position),
      static_cast<uint32_t>(state.target), static_cast<uint32_t>(state.mode),
      state.enabled ? 1u : 0u, state.motionRequested ? 1u : 0u, 0u}};
  record.words[7] = checksum(record);
  return record;
}

bool decodeState(const StateRecord& record, const Settings& settings, SavedState& state) {
  if (record.words[0] != kMagic || record.words[1] != kSchema ||
      record.words[7] != checksum(record) || record.words[4] > 2 ||
      record.words[5] > 1 || record.words[6] > 1 ||
      record.words[2] > static_cast<uint32_t>(settings.outsidePosition) ||
      record.words[3] > static_cast<uint32_t>(settings.outsidePosition)) return false;
  const SavedState decoded = {static_cast<int32_t>(record.words[2]),
      static_cast<int32_t>(record.words[3]), static_cast<Mode>(record.words[4]),
      record.words[5] == 1, record.words[6] == 1};
  if (!validState(decoded, settings)) return false;
  state = decoded;
  return true;
}

}  // namespace dryguard
