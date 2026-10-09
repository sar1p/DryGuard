#pragma once

#include "../core/Types.h"

namespace dryguard {

enum class CommandType { Power, Automatic, Manual, Inside, Outside };

class CommandSink {
 public:
  virtual ~CommandSink() = default;
  virtual void handleCommand(CommandType type, int value) = 0;
};

struct DashboardState {
  SavedState control;
  SensorReadings sensors;
  MotorSnapshot motor;
};

// Blynk callbacks, connection, and all virtualWrite calls have one task owner.
class BlynkGateway {
 public:
  void begin(CommandSink& sink);
  void poll(uint32_t now);
  void publish(const DashboardState& state, uint32_t now);
  void receive(CommandType type, int value);
  void onConnected();

 private:
  CommandSink* sink_ = nullptr;
  bool configured_ = false;
  bool wifiWasConnected_ = false;
  bool cacheValid_ = false;
  bool refreshSwitches_ = false;
  bool hasPublished_ = false;
  bool resetInside_ = false;
  bool resetOutside_ = false;
  uint32_t lastWifiAttempt_ = 0;
  uint32_t lastBlynkAttempt_ = 0;
  uint32_t lastPublish_ = 0;
  DashboardState cached_{};
};

}  // namespace dryguard
