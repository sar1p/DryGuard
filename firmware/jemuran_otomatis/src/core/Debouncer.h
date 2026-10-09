#pragma once

#include <stdint.h>

namespace jemuran {

class Debouncer {
 public:
  explicit Debouncer(uint32_t intervalMs) : intervalMs_(intervalMs) {}
  bool accept(uint32_t now) {
    if (used_ && static_cast<uint32_t>(now - lastAccepted_) < intervalMs_) return false;
    used_ = true;
    lastAccepted_ = now;
    return true;
  }

 private:
  uint32_t intervalMs_;
  uint32_t lastAccepted_ = 0;
  bool used_ = false;
};

}  // namespace jemuran
