#include <Arduino.h>

#include "app/Application.h"
#include "diagnostics/Diagnostics.h"

namespace {
jemuran::Application application;
}

void setup() {
  Serial.begin(115200);
  jemuran::printBootDiagnostics();
  if (!application.begin()) {
    Serial.println("[FATAL] Startup failed; motor remains stopped");
    for (;;) delay(1000);
  }
  Serial.println("[BOOT] Application and motor tasks started");
}

void loop() {
  // Keep Arduino's loop task alive; runtime tasks yield to the idle/watchdog tasks.
  delay(1000);
}
