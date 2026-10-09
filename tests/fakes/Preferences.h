#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace fake_preferences {

struct Value {
  enum Kind { Integer, Boolean, Bytes } kind;
  int32_t integerValue;
  bool booleanValue;
  std::vector<uint8_t> byteValue;

  Value() : kind(Bytes), integerValue(0), booleanValue(false) {}
  explicit Value(int32_t value) : kind(Integer), integerValue(value), booleanValue(false) {}
  explicit Value(bool value) : kind(Boolean), integerValue(0), booleanValue(value) {}
  Value(const void* data, size_t size)
      : kind(Bytes), integerValue(0), booleanValue(false),
        byteValue(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size) {}
};

typedef std::map<std::string, Value> KeyValues;
typedef std::map<std::string, KeyValues> Database;

inline Database& database() {
  static Database value;
  return value;
}

inline bool& beginFailure() {
  static bool value = false;
  return value;
}

inline size_t& nextPutLimit() {
  static size_t value = static_cast<size_t>(-1);
  return value;
}

inline size_t& putCalls() {
  static size_t value = 0;
  return value;
}

inline void reset() {
  database().clear();
  beginFailure() = false;
  nextPutLimit() = static_cast<size_t>(-1);
  putCalls() = 0;
}

inline void setBeginFailure(bool fail) { beginFailure() = fail; }
inline void setNextPutLimit(size_t limit) { nextPutLimit() = limit; }
inline size_t putBytesCalls() { return putCalls(); }

inline void seedInt(const char* name, const char* key, int32_t value) {
  database()[name][key] = Value(value);
}

inline void seedBool(const char* name, const char* key, bool value) {
  database()[name][key] = Value(value);
}

inline void seedBytes(const char* name, const char* key, const void* data, size_t size) {
  database()[name][key] = Value(data, size);
}

inline bool hasKey(const char* name, const char* key) {
  Database::const_iterator space = database().find(name);
  return space != database().end() && space->second.find(key) != space->second.end();
}

inline std::vector<uint8_t> bytes(const char* name, const char* key) {
  Database::const_iterator space = database().find(name);
  if (space == database().end()) return std::vector<uint8_t>();
  KeyValues::const_iterator value = space->second.find(key);
  if (value == space->second.end() || value->second.kind != Value::Bytes) {
    return std::vector<uint8_t>();
  }
  return value->second.byteValue;
}

}  // namespace fake_preferences

class Preferences {
 public:
  Preferences() : opened_(false), readOnly_(false) {}

  bool begin(const char* name, bool readOnly = false) {
    if (!name || fake_preferences::beginFailure()) return false;
    fake_preferences::Database& values = fake_preferences::database();
    fake_preferences::Database::iterator space = values.find(name);
    if (readOnly && space == values.end()) return false;
    if (!readOnly) values[name];
    namespace_ = name;
    opened_ = true;
    readOnly_ = readOnly;
    return true;
  }

  void end() {
    opened_ = false;
    namespace_.clear();
  }

  bool isKey(const char* key) const {
    return opened_ && lookup(key) != 0;
  }

  size_t getBytesLength(const char* key) const {
    const fake_preferences::Value* value = lookup(key);
    return value && value->kind == fake_preferences::Value::Bytes
        ? value->byteValue.size() : 0;
  }

  size_t getBytes(const char* key, void* buffer, size_t length) const {
    const fake_preferences::Value* value = lookup(key);
    if (!value || value->kind != fake_preferences::Value::Bytes ||
        value->byteValue.size() != length) return 0;
    if (length != 0) std::memcpy(buffer, &value->byteValue[0], length);
    return length;
  }

  int32_t getInt(const char* key, int32_t defaultValue = 0) const {
    const fake_preferences::Value* value = lookup(key);
    return value && value->kind == fake_preferences::Value::Integer
        ? value->integerValue : defaultValue;
  }

  bool getBool(const char* key, bool defaultValue = false) const {
    const fake_preferences::Value* value = lookup(key);
    return value && value->kind == fake_preferences::Value::Boolean
        ? value->booleanValue : defaultValue;
  }

  size_t putBytes(const char* key, const void* data, size_t length) {
    ++fake_preferences::putCalls();
    if (!opened_ || readOnly_) return 0;
    const size_t limit = fake_preferences::nextPutLimit();
    fake_preferences::nextPutLimit() = static_cast<size_t>(-1);
    const size_t written = std::min(length, limit);
    fake_preferences::database()[namespace_][key] = fake_preferences::Value(data, written);
    return written;
  }

 private:
  const fake_preferences::Value* lookup(const char* key) const {
    if (!opened_ || !key) return 0;
    fake_preferences::Database::const_iterator space =
        fake_preferences::database().find(namespace_);
    if (space == fake_preferences::database().end()) return 0;
    fake_preferences::KeyValues::const_iterator value = space->second.find(key);
    return value == space->second.end() ? 0 : &value->second;
  }

  std::string namespace_;
  bool opened_;
  bool readOnly_;
};
