#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <Arduino.h>

#include "config/HardwareConfig.h"
#include "core/Controller.h"
#include "core/StateCodec.h"
#include "storage/StateStore.h"
#include "storage/StorageKeys.h"
#include "Preferences.h"

using namespace dryguard;

FakeSerial Serial;
namespace fake { uint32_t clockMs = 0; }
uint32_t millis() { return fake::clockMs; }

namespace {
void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

void seedRecord(const char* name, const SavedState& state) {
  const StateRecord record = encodeState(state);
  fake_preferences::seedBytes(name, storage_keys::kStateKey, &record, sizeof(record));
}

SavedState safeDefault() { return defaultState(config::kSettings); }

void requireSafeDefault(const SavedState& state, const char* message) {
  require(sameState(state, safeDefault()), message);
  require(!state.enabled && !state.motionRequested, "safe default must remain OFF and hold");
}

SavedState loadFreshStore() {
  StateStore store;
  require(store.begin(), "store must initialize");
  return store.load();
}

void testCurrentNamespaceTakesPrecedence() {
  fake_preferences::reset();
  const SavedState current = {1234, 4000, Mode::Manual, true, true};
  const SavedState previous = {700, 0, Mode::Automatic, false, true};
  seedRecord(storage_keys::kCurrentNamespace, current);
  seedRecord(storage_keys::kPreviousV3Namespace, previous);
  fake_preferences::seedInt(storage_keys::kLegacyV2Namespace,
      storage_keys::kLegacyPositionKey, 2000);

  StateStore store;
  require(store.begin(), "current store must initialize");
  require(sameState(store.load(), current), "current checkpoint must win over every migration source");
}

void testPreviousV3ImportsOffAndSavesCanonically() {
  fake_preferences::reset();
  const SavedState previous = {1532, 4000, Mode::Manual, true, true};
  // Fixed fixture from the previous stored format, independent of the encoder.
  const StateRecord previousRecord = {{
      0x4A454D33u, 1u, 1532u, 4000u, 2u, 1u, 1u, 0x64540b6bu}};
  fake_preferences::seedBytes(storage_keys::kPreviousV3Namespace,
      storage_keys::kStateKey, &previousRecord, sizeof(previousRecord));
  const std::vector<uint8_t> before = fake_preferences::bytes(
      storage_keys::kPreviousV3Namespace, storage_keys::kStateKey);

  StateStore store;
  require(store.begin(), "current store must initialize");
  SavedState imported = store.load();
  SavedState expected = previous;
  expected.enabled = false;
  require(sameState(imported, expected), "previous record must preserve movement intent while powering OFF");
  require(store.save(imported), "normal checkpoint save must persist the imported state");
  require(fake_preferences::hasKey(storage_keys::kCurrentNamespace, storage_keys::kStateKey),
          "canonical checkpoint must be created");

  const std::vector<uint8_t> saved = fake_preferences::bytes(
      storage_keys::kCurrentNamespace, storage_keys::kStateKey);
  require(saved.size() == sizeof(StateRecord), "canonical checkpoint must retain the codec record size");
  StateRecord decodedRecord{};
  std::memcpy(&decodedRecord, &saved[0], sizeof(decodedRecord));
  SavedState decoded = safeDefault();
  require(decodeState(decodedRecord, config::kSettings, decoded), "saved canonical record must decode");
  require(sameState(decoded, expected), "canonical checkpoint must contain the imported OFF state");
  require(fake_preferences::bytes(storage_keys::kPreviousV3Namespace,
              storage_keys::kStateKey) == before,
          "read-only migration must leave the old record byte-for-byte unchanged");
}

void testCorruptCurrentBlocksEveryFallback() {
  fake_preferences::reset();
  StateRecord corrupt = encodeState({1000, 4000, Mode::Manual, true, true});
  corrupt.words[2] ^= 1u;
  fake_preferences::seedBytes(storage_keys::kCurrentNamespace, storage_keys::kStateKey,
                               &corrupt, sizeof(corrupt));
  seedRecord(storage_keys::kPreviousV3Namespace,
             {1400, 4000, Mode::Manual, true, true});
  fake_preferences::seedInt(storage_keys::kLegacyV2Namespace,
      storage_keys::kLegacyPositionKey, 3000);

  SavedState loaded = loadFreshStore();
  requireSafeDefault(loaded, "corrupt current checkpoint must return the safe default");
}

void testCorruptPreviousV3BlocksOlderFields() {
  fake_preferences::reset();
  StateRecord corrupt = encodeState({800, 4000, Mode::Manual, true, true});
  corrupt.words[7] ^= 0x80u;
  fake_preferences::seedBytes(storage_keys::kPreviousV3Namespace, storage_keys::kStateKey,
                               &corrupt, sizeof(corrupt));
  fake_preferences::seedInt(storage_keys::kLegacyV2Namespace,
      storage_keys::kLegacyPositionKey, 2100);
  fake_preferences::seedInt(storage_keys::kLegacyV2Namespace,
      storage_keys::kLegacyTargetKey, 4000);

  SavedState loaded = loadFreshStore();
  requireSafeDefault(loaded, "corrupt previous checkpoint must block stale field imports");
}

void testV2FieldsRetainPositionTargetModeAndIntentOff() {
  fake_preferences::reset();
  fake_preferences::seedInt(storage_keys::kLegacyV2Namespace,
      storage_keys::kLegacyPositionKey, 1425);
  fake_preferences::seedInt(storage_keys::kLegacyV2Namespace,
      storage_keys::kLegacyTargetKey, 4000);
  fake_preferences::seedBool(storage_keys::kLegacyV2Namespace,
      storage_keys::kLegacyAutomaticModeKey, false);
  fake_preferences::seedBool(storage_keys::kLegacyV2Namespace,
      storage_keys::kLegacyManualModeKey, true);

  const SavedState loaded = loadFreshStore();
  require(loaded.position == 1425 && loaded.target == 4000,
          "v2 position and target must be retained");
  require(loaded.mode == Mode::Manual && loaded.motionRequested,
          "v2 mode and pending intent must be retained");
  require(!loaded.enabled, "v2 import must always start OFF");
}

void testV1ResumeRetainsPositionAndDestinationOff() {
  fake_preferences::reset();
  fake_preferences::seedInt(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyPositionKey, 900);
  fake_preferences::seedInt(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyTargetKey, 0);
  fake_preferences::seedBool(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyAutomaticModeKey, true);
  fake_preferences::seedBool(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyResumeFlagKey, true);
  fake_preferences::seedInt(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyResumeTargetKey, 1);
  fake_preferences::seedInt(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyResumePositionKey, 1860);

  const SavedState loaded = loadFreshStore();
  require(loaded.position == 1860 && loaded.target == config::kSettings.outsidePosition,
          "v1 resume must retain its saved position and destination");
  require(loaded.mode == Mode::Manual && loaded.motionRequested,
          "v1 resume must restore manual motion intent");
  require(!loaded.enabled, "v1 resume import must always start OFF");
}

void testMalformedLegacyDataReturnsSafeDefault() {
  fake_preferences::reset();
  fake_preferences::seedInt(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyPositionKey, 4001);
  fake_preferences::seedInt(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyTargetKey, 0);
  fake_preferences::seedBool(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyAutomaticModeKey, false);
  fake_preferences::seedBool(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyManualModeKey, true);
  requireSafeDefault(loadFreshStore(), "out-of-range legacy position must be rejected");

  fake_preferences::reset();
  fake_preferences::seedInt(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyPositionKey, 1200);
  fake_preferences::seedInt(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyTargetKey, 4000);
  fake_preferences::seedBool(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyResumeFlagKey, true);
  fake_preferences::seedInt(storage_keys::kLegacyV1Namespace,
      storage_keys::kLegacyResumeTargetKey, 7);
  requireSafeDefault(loadFreshStore(), "invalid legacy resume destination must be rejected");
}

void testInitializationFailureAndFailedWriteRetry() {
  fake_preferences::reset();
  fake_preferences::setBeginFailure(true);
  StateStore unavailable;
  require(!unavailable.begin(), "configured initialization failure must be reported");
  requireSafeDefault(unavailable.load(), "unavailable storage must return safe default");
  require(!unavailable.save(safeDefault()), "unavailable storage must reject writes");

  fake_preferences::setBeginFailure(false);
  StateStore store;
  require(store.begin(), "store must initialize after failure is cleared");
  const SavedState checkpoint = {1111, 4000, Mode::Manual, false, true};
  fake_preferences::setNextPutLimit(sizeof(StateRecord) - 1);
  require(!store.save(checkpoint), "short write must be reported as failed");
  require(store.save(checkpoint), "failed write must be retryable");
  require(fake_preferences::putBytesCalls() == 2, "retry must perform a second flash write");
  const std::vector<uint8_t> saved = fake_preferences::bytes(
      storage_keys::kCurrentNamespace, storage_keys::kStateKey);
  require(saved.size() == sizeof(StateRecord), "retry must replace the incomplete record");
  StateRecord record{};
  std::memcpy(&record, &saved[0], sizeof(record));
  SavedState decoded = safeDefault();
  require(decodeState(record, config::kSettings, decoded) && sameState(decoded, checkpoint),
          "retried checkpoint must be complete and valid");
  require(store.save(checkpoint) && fake_preferences::putBytesCalls() == 2,
          "identical saved state must avoid another write");
}
}  // namespace

int main() {
  const std::vector<std::pair<std::string, std::function<void()> > > tests = {
    {"current namespace takes precedence", testCurrentNamespaceTakesPrecedence},
    {"previous v3 imports OFF and saves canonically", testPreviousV3ImportsOffAndSavesCanonically},
    {"corrupt current blocks every fallback", testCorruptCurrentBlocksEveryFallback},
    {"corrupt previous v3 blocks older fields", testCorruptPreviousV3BlocksOlderFields},
    {"v2 fields retain state while OFF", testV2FieldsRetainPositionTargetModeAndIntentOff},
    {"v1 resume retains position and destination while OFF", testV1ResumeRetainsPositionAndDestinationOff},
    {"malformed legacy data returns safe default", testMalformedLegacyDataReturnsSafeDefault},
    {"initialization failure and failed write retry", testInitializationFailureAndFailedWriteRetry}
  };

  unsigned failures = 0;
  for (const auto& test : tests) {
    try {
      test.second();
      std::cout << "PASS " << test.first << '\n';
    } catch (const std::exception& error) {
      ++failures;
      std::cerr << "FAIL " << test.first << ": " << error.what() << '\n';
    }
  }
  std::cout << tests.size() - failures << '/' << tests.size() << " storage tests passed\n";
  return failures == 0 ? 0 : 1;
}
