#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <Arduino.h>

#include "Preferences.h"
#include "config/HardwareConfig.h"
#include "core/Controller.h"
#include "core/StateCodec.h"
#include "storage/StateStore.h"
#include "storage/StorageKeys.h"

using namespace dryguard;

FakeSerial Serial;
namespace fake { uint32_t clockMs = 0; }
uint32_t millis() { return fake::clockMs; }

namespace {
void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

void requireSafeDefault(const SavedState& state, const char* message) {
  const SavedState expected = {0, 0, Mode::Automatic, false, false};
  require(sameState(state, expected), message);
  require(state.position == 0 && !state.enabled && !state.motionRequested,
          "safe default must be OFF at estimate 0");
}

SavedState loadFreshStore() {
  StateStore store;
  require(store.begin(), "canonical store must initialize");
  return store.load();
}

void testCurrentCheckpointLoadsFixedRecordAndRestoresOff() {
  fake_preferences::reset();
  const StateRecord fixture = {{
      0x4A454D33u, 1u, 1532u, 4000u, 2u, 1u, 1u, 0x64540b6bu}};
  fake_preferences::seedBytes(storage_keys::kCurrentNamespace,
      storage_keys::kStateKey, &fixture, sizeof(fixture));

  StateStore store;
  require(store.begin(), "canonical store must initialize");
  const SavedState loaded = store.load();
  require(loaded.position == 1532 && loaded.target == 4000,
          "fixed current checkpoint must preserve position and target");
  require(loaded.mode == Mode::Manual && loaded.enabled && loaded.motionRequested,
          "fixed current checkpoint must preserve mode, power, and intent");

  Controller controller(config::kSettings);
  controller.restore(loaded);
  require(!controller.state().enabled && !controller.motionRequest().move,
          "controller restore must keep startup OFF");
  require(controller.state().target == 4000 && controller.state().motionRequested,
          "controller restore must keep the stored destination and intent");
}

void testMissingCurrentCheckpointIgnoresUnrelatedNamespace() {
  fake_preferences::reset();
  const StateRecord unrelated = {{
      0x4A454D33u, 1u, 1532u, 4000u, 2u, 1u, 1u, 0x64540b6bu}};
  fake_preferences::seedBytes("unrelated_store", storage_keys::kStateKey,
      &unrelated, sizeof(unrelated));

  const SavedState loaded = loadFreshStore();
  requireSafeDefault(loaded,
      "missing canonical checkpoint must ignore data in an unrelated namespace");
}

void testCorruptChecksumReturnsSafeDefault() {
  fake_preferences::reset();
  StateRecord corrupt = {{
      0x4A454D33u, 1u, 1532u, 4000u, 2u, 1u, 1u, 0x64540b6bu}};
  corrupt.words[7] ^= 1u;
  fake_preferences::seedBytes(storage_keys::kCurrentNamespace,
      storage_keys::kStateKey, &corrupt, sizeof(corrupt));

  requireSafeDefault(loadFreshStore(), "bad checkpoint checksum must return safe default");
}

void testIncorrectRecordLengthAndTypeReturnSafeDefault() {
  fake_preferences::reset();
  const uint8_t shortRecord[] = {1u, 2u, 3u};
  fake_preferences::seedBytes(storage_keys::kCurrentNamespace,
      storage_keys::kStateKey, shortRecord, sizeof(shortRecord));
  requireSafeDefault(loadFreshStore(), "short checkpoint blob must return safe default");

  fake_preferences::reset();
  fake_preferences::seedInt(storage_keys::kCurrentNamespace,
      storage_keys::kStateKey, 42);
  requireSafeDefault(loadFreshStore(), "non-blob checkpoint value must return safe default");
}

void testSaveAndLoadRoundTripPreservesIntent() {
  fake_preferences::reset();
  const SavedState expected = {2715, 4000, Mode::Manual, false, true};
  StateStore writer;
  require(writer.begin(), "writer must initialize");
  require(writer.save(expected), "valid checkpoint must save");

  StateStore reader;
  require(reader.begin(), "reader must initialize");
  const SavedState loaded = reader.load();
  require(sameState(loaded, expected),
          "round trip must preserve position, target, mode, power, and intent");
}

void testInvalidSaveDoesNotReplaceLastGoodCheckpoint() {
  fake_preferences::reset();
  const SavedState good = {1100, 4000, Mode::Manual, false, true};
  const SavedState invalid = {4001, 0, Mode::Manual, true, true};
  StateStore store;
  require(store.begin(), "store must initialize");
  require(store.save(good), "valid checkpoint must save");
  const std::vector<uint8_t> before = fake_preferences::bytes(
      storage_keys::kCurrentNamespace, storage_keys::kStateKey);
  require(!store.save(invalid), "out-of-range state must be rejected");
  require(fake_preferences::bytes(storage_keys::kCurrentNamespace,
              storage_keys::kStateKey) == before,
          "rejected state must not overwrite the last good checkpoint");

  StateStore reader;
  require(reader.begin(), "reader must initialize");
  require(sameState(reader.load(), good), "last good checkpoint must remain readable");
}

void testInitializationFailureIsReportedSafely() {
  fake_preferences::reset();
  fake_preferences::setBeginFailure(true);
  StateStore store;
  require(!store.begin(), "storage initialization failure must be reported");
  requireSafeDefault(store.load(), "unavailable storage must return safe default");
  require(!store.save({0, 0, Mode::Automatic, false, false}),
          "unavailable storage must reject writes");
}

void testShortWriteCanRetryAndIdenticalSaveIsDeduplicated() {
  fake_preferences::reset();
  StateStore store;
  require(store.begin(), "store must initialize");
  const SavedState expected = {1111, 4000, Mode::Manual, false, true};
  fake_preferences::setNextPutLimit(sizeof(StateRecord) - 1);
  require(!store.save(expected), "short write must be reported as failed");
  require(store.save(expected), "short write must be retryable");
  require(fake_preferences::putBytesCalls() == 2, "retry must issue a second write");

  const std::vector<uint8_t> saved = fake_preferences::bytes(
      storage_keys::kCurrentNamespace, storage_keys::kStateKey);
  require(saved.size() == sizeof(StateRecord), "retry must replace the short blob");
  StateRecord record{};
  std::memcpy(&record, &saved[0], sizeof(record));
  SavedState decoded = {0, 0, Mode::Automatic, false, false};
  require(decodeState(record, config::kSettings, decoded) && sameState(decoded, expected),
          "retried checkpoint must be complete and valid");
  require(store.save(expected), "identical checkpoint save must succeed");
  require(fake_preferences::putBytesCalls() == 2,
          "identical checkpoint save must not issue another write");
}
}  // namespace

int main() {
  const std::vector<std::pair<std::string, std::function<void()> > > tests = {
    {"current fixed checkpoint loads and controller restores OFF",
        testCurrentCheckpointLoadsFixedRecordAndRestoresOff},
    {"missing current checkpoint ignores unrelated namespace",
        testMissingCurrentCheckpointIgnoresUnrelatedNamespace},
    {"corrupt checksum returns safe default", testCorruptChecksumReturnsSafeDefault},
    {"incorrect record length and type return safe default",
        testIncorrectRecordLengthAndTypeReturnSafeDefault},
    {"save and load round trip preserves intent", testSaveAndLoadRoundTripPreservesIntent},
    {"invalid save does not replace last good checkpoint",
        testInvalidSaveDoesNotReplaceLastGoodCheckpoint},
    {"initialization failure is reported safely", testInitializationFailureIsReportedSafely},
    {"short write retry and identical save deduplication",
        testShortWriteCanRetryAndIdenticalSaveIsDeduplicated}
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
