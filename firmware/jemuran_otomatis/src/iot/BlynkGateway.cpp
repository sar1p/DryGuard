#if __has_include("../Secrets.h")
#include "../Secrets.h"
#else
#include "../Secrets.example.h"
#endif

#define BLYNK_PRINT Serial
#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <string.h>

#include "BlynkGateway.h"
#include "../config/HardwareConfig.h"

namespace {
jemuran::BlynkGateway* gateway = nullptr;

const char* weatherText(const jemuran::DashboardState& state) {
  if (!state.control.enabled) return "OFF";
  if (!state.sensors.valid) return "MEMBACA";
  return state.sensors.raining ? "HUJAN" : "KERING";
}

const char* lightText(const jemuran::DashboardState& state) {
  if (!state.control.enabled) return "-";
  if (!state.sensors.valid) return "MEMBACA";
  return state.sensors.dark ? "GELAP" : "TERANG";
}

const char* positionText(const jemuran::DashboardState& state) {
  if (!state.control.enabled) return "SISTEM MATI";
  if (state.motor.moving) return state.motor.target > state.motor.position ? "KELUAR >>" : "<< MASUK";
  if (state.motor.position <= jemuran::config::kSettings.insidePosition + 100) return "DALAM";
  if (state.motor.position >= jemuran::config::kSettings.outsidePosition - 100) return "LUAR";
  return "TENGAH";
}
}  // namespace

namespace jemuran {

void BlynkGateway::begin(CommandSink& sink) {
  sink_ = &sink;
  gateway = this;
  configured_ = WIFI_SSID[0] != '\0' && BLYNK_AUTH_TOKEN[0] != '\0';
  if (!configured_) {
    Serial.println("[IOT] Credentials empty: Wi-Fi/Blynk disabled; local controller remains running");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Blynk.config(BLYNK_AUTH_TOKEN);
  lastWifiAttempt_ = millis();
  lastBlynkAttempt_ = millis() - config::kBlynkRetryMs;
}

void BlynkGateway::poll(uint32_t now) {
  if (!configured_) return;
  const bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (wifiConnected != wifiWasConnected_) {
    Serial.println(wifiConnected ? "[IOT] Wi-Fi connected" : "[IOT] Wi-Fi disconnected");
    wifiWasConnected_ = wifiConnected;
  }
  if (!wifiConnected) {
    if (static_cast<uint32_t>(now - lastWifiAttempt_) >= config::kWifiRetryMs) {
      lastWifiAttempt_ = now;
      WiFi.reconnect();
    }
    return;
  }
  if (Blynk.connected()) {
    Blynk.run();
  } else if (static_cast<uint32_t>(now - lastBlynkAttempt_) >= config::kBlynkRetryMs) {
    lastBlynkAttempt_ = now;
    Blynk.connect(config::kBlynkConnectTimeoutMs);
  }
}

void BlynkGateway::onConnected() {
  // Publish the device's current state; stale cloud switches do not overwrite it.
  cacheValid_ = false;
  resetInside_ = resetOutside_ = true;
  Serial.println("[IOT] Blynk connected; dashboard will be refreshed");
}

void BlynkGateway::receive(CommandType type, int value) {
  if (type == CommandType::Inside && value != 0) resetInside_ = true;
  if (type == CommandType::Outside && value != 0) resetOutside_ = true;
  if (!sink_ || (value != 0 && value != 1)) return;
  if ((type == CommandType::Inside || type == CommandType::Outside) && value != 1) return;
  sink_->handleCommand(type, value);
}

void BlynkGateway::publish(const DashboardState& state, uint32_t now) {
  if (!configured_ || !Blynk.connected()) return;
  if (hasPublished_ && static_cast<uint32_t>(now - lastPublish_) < config::kDashboardIntervalMs) return;
  const bool automatic = state.control.enabled && state.control.mode == Mode::Automatic;
  const bool manual = state.control.enabled && state.control.mode == Mode::Manual;
  const bool oldAutomatic = cached_.control.enabled && cached_.control.mode == Mode::Automatic;
  const bool oldManual = cached_.control.enabled && cached_.control.mode == Mode::Manual;
  if (!cacheValid_ || state.control.enabled != cached_.control.enabled) Blynk.virtualWrite(V0, state.control.enabled);
  if (!cacheValid_ || automatic != oldAutomatic) Blynk.virtualWrite(V1, automatic);
  if (!cacheValid_ || manual != oldManual) Blynk.virtualWrite(V2, manual);
  if (resetInside_) { Blynk.virtualWrite(V3, 0); resetInside_ = false; }
  if (resetOutside_) { Blynk.virtualWrite(V4, 0); resetOutside_ = false; }
  if (!cacheValid_ || strcmp(weatherText(state), weatherText(cached_)) != 0) Blynk.virtualWrite(V5, weatherText(state));
  if (!cacheValid_ || strcmp(lightText(state), lightText(cached_)) != 0) Blynk.virtualWrite(V6, lightText(state));
  if (!cacheValid_ || strcmp(positionText(state), positionText(cached_)) != 0) Blynk.virtualWrite(V7, positionText(state));
  cached_ = state;
  cacheValid_ = true;
  hasPublished_ = true;
  lastPublish_ = now;
}

}  // namespace jemuran

BLYNK_CONNECTED() { if (gateway) gateway->onConnected(); }
BLYNK_WRITE(V0) { if (gateway) gateway->receive(jemuran::CommandType::Power, param.asInt()); }
BLYNK_WRITE(V1) { if (gateway) gateway->receive(jemuran::CommandType::Automatic, param.asInt()); }
BLYNK_WRITE(V2) { if (gateway) gateway->receive(jemuran::CommandType::Manual, param.asInt()); }
BLYNK_WRITE(V3) { if (gateway) gateway->receive(jemuran::CommandType::Inside, param.asInt()); }
BLYNK_WRITE(V4) { if (gateway) gateway->receive(jemuran::CommandType::Outside, param.asInt()); }
