#pragma once

enum { WIFI_STA = 1, WL_CONNECTED = 3, WL_DISCONNECTED = 6 };

class FakeWiFi {
 public:
  int statusValue = WL_CONNECTED;
  void mode(int) {}
  void setAutoReconnect(bool) {}
  void begin(const char*, const char*) {}
  int status() const { return statusValue; }
  void reconnect() {}
};

extern FakeWiFi WiFi;
