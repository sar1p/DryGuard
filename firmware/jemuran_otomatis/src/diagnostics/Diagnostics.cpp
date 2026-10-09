#include "Diagnostics.h"

#include <Arduino.h>
#include <esp_system.h>

namespace jemuran {
namespace {
const char* resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "POWER_ON";
    case ESP_RST_EXT: return "EXTERNAL_RESET";
    case ESP_RST_SW: return "SOFTWARE_RESET";
    case ESP_RST_PANIC: return "PANIC";
    case ESP_RST_INT_WDT: return "INTERRUPT_WATCHDOG";
    case ESP_RST_TASK_WDT: return "TASK_WATCHDOG";
    case ESP_RST_WDT: return "WATCHDOG";
    case ESP_RST_DEEPSLEEP: return "DEEP_SLEEP";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    default: return "UNKNOWN";
  }
}
}  // namespace

void printBootDiagnostics() {
  const esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("\n[BOOT] Jemuran modular | reset=%s (%d) | SDK=%s\n",
                resetReasonName(reason), static_cast<int>(reason), ESP.getSdkVersion());
  printMemoryDiagnostics();
}

void printMemoryDiagnostics() {
  Serial.printf("[DIAG] heap_free=%u bytes | heap_min=%u bytes\n",
      static_cast<unsigned>(esp_get_free_heap_size()),
      static_cast<unsigned>(esp_get_minimum_free_heap_size()));
}

}  // namespace jemuran
