#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Arduino.h"
#include "BlynkSimpleEsp32.h"
#include "WiFi.h"
#include "config/HardwareConfig.h"
#include "core/Controller.h"
#include "iot/BlynkGateway.h"

FakeSerial Serial;
FakeWiFi WiFi;
FakeBlynk Blynk;
namespace fake { uint32_t clockMs = 0; }
uint32_t millis() { return fake::clockMs; }

namespace {
void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

class ControllerSink : public jemuran::CommandSink {
 public:
  explicit ControllerSink(jemuran::Controller& controller) : calls(0), controller_(controller) {}
  void handleCommand(jemuran::CommandType type, int value) override {
    ++calls;
    const int32_t position = controller_.state().position;
    switch (type) {
      case jemuran::CommandType::Power: controller_.setEnabled(value == 1); break;
      case jemuran::CommandType::Automatic: controller_.setAutomatic(value == 1, position); break;
      case jemuran::CommandType::Manual: controller_.setManual(value == 1, position); break;
      case jemuran::CommandType::Inside: if (value == 1) controller_.moveInside(); break;
      case jemuran::CommandType::Outside: if (value == 1) controller_.moveOutside(); break;
    }
  }
  int calls;
 private:
  jemuran::Controller& controller_;
};

struct Fixture {
  jemuran::Controller controller;
  ControllerSink sink;
  jemuran::BlynkGateway gateway;
  Fixture() : controller(jemuran::config::kSettings), sink(controller) {
    fake::clockMs = 0;
    WiFi.statusValue = WL_CONNECTED;
    Blynk.connectedState = true;
    Blynk.writes.clear();
    gateway.begin(sink);
    BlynkWidgetConnected();
  }
  jemuran::DashboardState state() const {
    const jemuran::SavedState& control = controller.state();
    return {control, {3500, 2000, false, false, true},
            {control.position, control.target, controller.motionRequest().move, 0}};
  }
  void publish(uint32_t now) { gateway.publish(state(), now); }
};

void writeInt(int pin, int expected) {
  bool found = false;
  for (const FakeWrite& write : Blynk.writes) {
    if (write.pin == pin && !write.isText) { found = true; require(write.value == expected, "wrong switch echo"); }
  }
  require(found, "expected switch echo missing");
}

void writeText(int pin, const char* expected) {
  bool found = false;
  for (const FakeWrite& write : Blynk.writes) {
    if (write.pin == pin && write.isText) { found = true; require(write.text == expected, "wrong dashboard text"); }
  }
  require(found, "expected dashboard text missing");
}

void allPinsWrittenOnce() {
  require(Blynk.writes.size() == 8, "all eight dashboard pins must be written");
  for (int pin = V0; pin <= V7; ++pin) {
    unsigned count = 0;
    for (const FakeWrite& write : Blynk.writes) if (write.pin == pin) ++count;
    require(count == 1, "each dashboard pin must appear exactly once");
  }
}

void v1OnWhileOffEchoesOff() {
  Fixture f; f.publish(0); Blynk.writes.clear();
  BlynkWidgetWriteV1({1});
  require(!f.controller.state().enabled, "V1 must not enable power");
  f.publish(999); require(Blynk.writes.empty(), "publish must respect 1000 ms cadence");
  f.publish(1000); writeInt(V0, 0); writeInt(V1, 0); writeInt(V2, 0);
}

void v2OnWhileOffEchoesOff() {
  Fixture f; f.publish(0); Blynk.writes.clear();
  BlynkWidgetWriteV2({1});
  require(!f.controller.state().enabled, "V2 must not enable power");
  f.publish(1000); writeInt(V0, 0); writeInt(V1, 0); writeInt(V2, 0);
}

void repeatedSwitchesRespectCadenceAndWriteBound() {
  Fixture f; f.publish(0); Blynk.writes.clear();
  BlynkWidgetWriteV0({1}); BlynkWidgetWriteV1({1}); BlynkWidgetWriteV1({1});
  BlynkWidgetWriteV3({1}); BlynkWidgetWriteV4({1}); BlynkWidgetWriteV1({1});
  f.publish(250); f.publish(999);
  require(Blynk.writes.empty(), "repeated commands must not bypass cadence");
  f.publish(1000);
  require(Blynk.writes.size() <= 8, "one publish may write at most eight dashboard pins");
  allPinsWrittenOnce();
  writeInt(V0, 1); writeInt(V1, 1); writeInt(V2, 0);
  writeInt(V3, 0); writeInt(V4, 0);
  writeText(V5, "DRY"); writeText(V6, "BRIGHT"); writeText(V7, "EXTENDING >>");
}

void invalidInputsNeverReachController() {
  Fixture f;
  BlynkWidgetWriteV0({2}); BlynkWidgetWriteV1({-1}); BlynkWidgetWriteV2({2});
  require(f.sink.calls == 0, "invalid values must not reach the controller sink");
  require(!f.controller.state().enabled, "invalid values must not change controller state");
}

void reconnectRefreshesDashboard() {
  Fixture f; f.publish(0); Blynk.writes.clear();
  Blynk.connectedState = false;
  f.controller.setEnabled(true);
  f.publish(1000); require(Blynk.writes.empty(), "offline publish must be suppressed");
  Blynk.connectedState = true; BlynkWidgetConnected(); f.publish(1000);
  allPinsWrittenOnce();
  writeInt(V0, 1); writeInt(V1, 1); writeInt(V2, 0);
  writeInt(V3, 0); writeInt(V4, 0);
  writeText(V5, "DRY"); writeText(V6, "BRIGHT"); writeText(V7, "RETRACTED");
}
}  // namespace

int main() {
  const std::vector<std::pair<std::string, std::function<void()> > > tests = {
    {"V1 ON while OFF echoes actual OFF state", v1OnWhileOffEchoesOff},
    {"V2 ON while OFF echoes actual OFF state", v2OnWhileOffEchoesOff},
    {"repeated switches respect cadence and write bound", repeatedSwitchesRespectCadenceAndWriteBound},
    {"invalid values never reach controller", invalidInputsNeverReachController},
    {"reconnect refreshes dashboard", reconnectRefreshesDashboard}
  };
  unsigned failures = 0;
  for (const auto& test : tests) {
    try { test.second(); std::cout << "PASS " << test.first << '\n'; }
    catch (const std::exception& error) { ++failures; std::cerr << "FAIL " << test.first << ": " << error.what() << '\n'; }
  }
  std::cout << tests.size() - failures << '/' << tests.size() << " gateway tests passed\n";
  return failures == 0 ? 0 : 1;
}
