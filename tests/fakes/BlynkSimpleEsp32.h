#pragma once

#include <string>
#include <vector>

enum { V0, V1, V2, V3, V4, V5, V6, V7 };

struct BlynkFakeParam {
  int value;
  int asInt() const { return value; }
};

struct FakeWrite {
  int pin;
  bool isText;
  int value;
  std::string text;
};

class FakeBlynk {
 public:
  bool connectedState = true;
  std::vector<FakeWrite> writes;
  bool connected() const { return connectedState; }
  void config(const char*) {}
  bool connect(unsigned long) { return connectedState; }
  void run() {}
  void virtualWrite(int pin, int value) { writes.push_back({pin, false, value, std::string()}); }
  void virtualWrite(int pin, const char* value) { writes.push_back({pin, true, 0, value}); }
};

extern FakeBlynk Blynk;
void BlynkWidgetConnected();
void BlynkWidgetWriteV0(const BlynkFakeParam& param);
void BlynkWidgetWriteV1(const BlynkFakeParam& param);
void BlynkWidgetWriteV2(const BlynkFakeParam& param);
void BlynkWidgetWriteV3(const BlynkFakeParam& param);
void BlynkWidgetWriteV4(const BlynkFakeParam& param);

#define BLYNK_CONNECTED() void BlynkWidgetConnected()
#define BLYNK_WRITE(pin) void BlynkWidgetWrite##pin(const BlynkFakeParam& param)
