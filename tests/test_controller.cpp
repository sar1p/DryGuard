#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "config/HardwareConfig.h"
#include "core/Controller.h"
#include "core/Debouncer.h"
#include "core/SensorPolicy.h"
#include "core/StateCodec.h"

using namespace dryguard;

namespace {
void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

Controller runningController() {
  Controller controller(config::kSettings);
  controller.setEnabled(true);
  return controller;
}

SensorReadings sunny() { return {3500, 2000, false, false, true}; }
SensorReadings rain() { return {1700, 2000, true, false, true}; }
SensorReadings darkness() { return {3500, 3800, false, true, true}; }
}  // namespace

int main() {
  const std::vector<std::pair<std::string, std::function<void()> > > tests = {
    {"first boot waits for power ON", [] {
      Controller controller(config::kSettings);
      controller.updateSensors(sunny());
      require(!controller.state().enabled && !controller.motionRequest().move, "first boot must hold");
    }},
    {"sunny automatic mode extends", [] {
      Controller controller = runningController();
      controller.updateSensors(sunny());
      require(controller.motionRequest().move && controller.motionRequest().target == 4000, "sunny target");
    }},
    {"rain and darkness each retract", [] {
      Controller controller = runningController();
      controller.updateSensors(sunny());
      controller.updateSensors(rain());
      require(controller.motionRequest().target == 0, "rain must retract");
      controller.updateSensors(sunny());
      controller.updateSensors(darkness());
      require(controller.motionRequest().target == 0, "darkness must retract");
    }},
    {"rain threshold noise does not reverse repeatedly", [] {
      SensorPolicy sensors(config::kSettings);
      require(!sensors.update(2800, 2000).raining, "original strict boundary");
      require(sensors.update(2799, 2000).raining, "rain alarm must be immediate");
      require(sensors.update(2801, 2000).raining, "noise must not clear rain");
      require(sensors.update(2899, 2000).raining, "clearing margin");
      require(!sensors.update(2900, 2000).raining, "dry confirmation");
    }},
    {"light threshold has a clearing margin", [] {
      SensorPolicy sensors(config::kSettings);
      require(!sensors.update(3500, 3400).dark, "original strict boundary");
      require(sensors.update(3500, 3401).dark, "dark alarm must be immediate");
      require(sensors.update(3500, 3399).dark, "noise must not clear darkness");
      require(!sensors.update(3500, 3300).dark, "bright confirmation");
    }},
    {"rain recovery after invalid input preserves the clearing margin", [] {
      SensorPolicy sensors(config::kSettings);
      Controller controller = runningController();
      controller.updateSensors(sensors.update(2799, 2000));
      require(controller.motionRequest().target == 0, "rain must retract");
      const SensorReadings invalid = sensors.update(-1, 2000);
      controller.updateSensors(invalid);
      require(!invalid.valid && controller.motionRequest().target == 0, "invalid input must retract");
      const SensorReadings recovering = sensors.update(2850, 2000);
      controller.updateSensors(recovering);
      require(recovering.valid && recovering.raining && controller.motionRequest().target == 0,
              "recovery within rain hysteresis must not extend");
      controller.updateSensors(sensors.update(2900, 2000));
      require(controller.motionRequest().target == 4000, "dry margin must permit extension");
    }},
    {"darkness recovery after invalid input preserves the clearing margin", [] {
      SensorPolicy sensors(config::kSettings);
      Controller controller = runningController();
      controller.updateSensors(sensors.update(3500, 3401));
      require(controller.motionRequest().target == 0, "darkness must retract");
      const SensorReadings invalid = sensors.update(3500, 4096);
      controller.updateSensors(invalid);
      require(!invalid.valid && controller.motionRequest().target == 0, "invalid input must retract");
      const SensorReadings recovering = sensors.update(3500, 3350);
      controller.updateSensors(recovering);
      require(recovering.valid && recovering.dark && controller.motionRequest().target == 0,
              "recovery within light hysteresis must not extend");
      controller.updateSensors(sensors.update(3500, 3300));
      require(controller.motionRequest().target == 4000, "bright margin must permit extension");
    }},
    {"invalid ADC samples request inside in automatic mode", [] {
      SensorPolicy sensors(config::kSettings);
      Controller controller = runningController();
      controller.updateSensors(sunny());
      const SensorReadings invalid = sensors.update(-1, 2000);
      require(!invalid.valid, "invalid reading must be marked");
      controller.updateSensors(invalid);
      require(controller.motionRequest().target == 0, "invalid automatic input must retract");
      require(sensors.update(3500, 2000).valid, "sampling must recover");
    }},
    {"automatic and manual modes are mutually exclusive", [] {
      Controller controller = runningController();
      controller.updateSensors(sunny());
      controller.setManual(true, 1234);
      require(controller.state().mode == Mode::Manual && !controller.motionRequest().move, "manual starts holding");
      controller.setAutomatic(false, 1234);
      require(controller.state().mode == Mode::Manual, "inactive automatic switch must not cancel manual");
      controller.setAutomatic(true, 1234);
      controller.updateSensors(rain());
      require(controller.state().mode == Mode::Automatic && controller.motionRequest().target == 0, "automatic selection");
    }},
    {"manual command bypasses weather until automatic is selected", [] {
      Controller controller = runningController();
      require(controller.moveOutside(), "manual extension must be accepted");
      controller.updateSensors(rain());
      require(controller.state().mode == Mode::Manual && controller.motionRequest().target == 4000, "manual destination");
      require(controller.moveInside() && controller.motionRequest().target == 0, "manual retraction");
    }},
    {"power OFF rejects motion and retains pending destination", [] {
      Controller controller = runningController();
      controller.moveOutside();
      controller.setEnabled(false);
      require(!controller.motionRequest().move, "power OFF must hold");
      require(!controller.moveInside(), "OFF must reject manual movement");
      controller.setAutomatic(true, 1000);
      require(controller.state().mode == Mode::Manual, "OFF must reject mode changes");
      controller.setEnabled(true);
      require(controller.motionRequest().move && controller.motionRequest().target == 4000, "pending target must resume");
    }},
    {"paused manual travel survives checkpoint and reboot", [] {
      Controller controller = runningController();
      controller.moveOutside();
      controller.setEnabled(false);
      const SavedState saved = controller.checkpoint(1357);
      Controller rebooted(config::kSettings);
      rebooted.restore(saved);
      require(!rebooted.motionRequest().move && saved.position == 1357, "paused boot must hold actual snapshot");
      rebooted.setEnabled(true);
      require(rebooted.motionRequest().target == 4000, "resume target must not be overwritten with position");
    }},
    {"active manual travel waits for enable after reboot", [] {
      Controller controller = runningController();
      controller.moveOutside();
      const SavedState saved = controller.checkpoint(1450);
      Controller rebooted(config::kSettings);
      rebooted.restore(saved);
      require(!rebooted.motionRequest().move, "reboot must not start from an unverified position estimate");
      rebooted.setEnabled(true);
      require(rebooted.motionRequest().move && rebooted.motionRequest().target == 4000, "resume must preserve full-width target");
    }},
    {"automatic reboot waits for enable and evaluates current weather", [] {
      Controller controller = runningController();
      controller.updateSensors(sunny());
      Controller rebooted(config::kSettings);
      rebooted.restore(controller.checkpoint(1350));
      rebooted.updateSensors(rain());
      require(!rebooted.motionRequest().move, "automatic reboot must hold until enabled");
      rebooted.setEnabled(true);
      rebooted.updateSensors(rain());
      require(rebooted.motionRequest().target == 0, "re-enabled automatic must use current weather");
    }},
    {"automatic resume evaluates current weather", [] {
      Controller controller = runningController();
      controller.updateSensors(sunny());
      controller.setEnabled(false);
      controller.updateSensors(rain());
      controller.setEnabled(true);
      controller.updateSensors(rain());
      require(controller.motionRequest().target == 0, "power ON during rain must not resume extension");
    }},
    {"holding checkpoints use actual position instead of stale estimate", [] {
      Controller controller = runningController();
      controller.updateSensors(sunny());
      controller.setAutomatic(false, 1500);
      const SavedState saved = controller.checkpoint(1512);
      require(saved.mode == Mode::Idle && !saved.motionRequested, "disabled mode must hold");
      require(saved.target == 1512 && saved.position == 1512, "snapshot replaces stale hold target");
    }},
    {"invalid saved positions and modes cannot start motion", [] {
      const std::vector<SavedState> invalid = {
        {-1, 4000, Mode::Manual, true, true}, {4001, 0, Mode::Manual, true, true},
        {1200, 5000, Mode::Manual, true, true}, {0, 0, static_cast<Mode>(99), true, true},
        {0, 0, Mode::Idle, true, true}
      };
      for (const SavedState& state : invalid) {
        Controller controller(config::kSettings);
        controller.restore(state);
        require(!controller.state().enabled && !controller.motionRequest().move, "invalid restore must fall back OFF");
      }
    }},
    {"checkpoint record round trips all valid modes", [] {
      const std::vector<SavedState> states = {
        {1234, 4000, Mode::Manual, true, true}, {1234, 4000, Mode::Manual, false, true},
        {4000, 0, Mode::Automatic, true, true}, {1720, 1720, Mode::Idle, true, false},
        defaultState(config::kSettings)
      };
      for (const SavedState& state : states) {
        SavedState restored = defaultState(config::kSettings);
        require(decodeState(encodeState(state), config::kSettings, restored), "valid record must decode");
        require(sameState(state, restored), "record fields must survive reboot");
      }
    }},
    {"corrupt records are rejected without altering output", [] {
      const SavedState state = {1357, 4000, Mode::Manual, true, true};
      StateRecord record = encodeState(state);
      record.words[2] ^= 1;
      SavedState output = defaultState(config::kSettings);
      const SavedState before = output;
      require(!decodeState(record, config::kSettings, output), "checksum must reject corruption");
      require(sameState(output, before), "failed decode must not mutate output");
      const SavedState invalid = {100, 4000, static_cast<Mode>(100), true, true};
      require(!decodeState(encodeState(invalid), config::kSettings, output), "valid checksum cannot authorize invalid mode");
    }},
    {"button debounce accepts first press and handles millis wrap", [] {
      Debouncer first(config::kButtonDebounceMs);
      require(first.accept(0), "first button press must not be lost at boot");
      require(!first.accept(100) && first.accept(300), "300-ms debounce");
      Debouncer wrapping(config::kButtonDebounceMs);
      require(wrapping.accept(UINT32_MAX - 100), "initial wrapped timestamp");
      require(!wrapping.accept(20) && wrapping.accept(250), "unsigned wrap must preserve elapsed time");
    }}
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
  std::cout << tests.size() - failures << '/' << tests.size() << " tests passed\n";
  return failures == 0 ? 0 : 1;
}
