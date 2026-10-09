#pragma once

#include <stdint.h>

struct FakeSerial {
  void println(const char*) {}
};

extern FakeSerial Serial;
namespace fake { extern uint32_t clockMs; }
uint32_t millis();
