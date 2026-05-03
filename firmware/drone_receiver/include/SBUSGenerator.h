#pragma once

#include <Arduino.h>
#include <cstdint>

class SBUSGenerator {
public:
  SBUSGenerator(int txPin);

  void begin();

  // Channels are 0-indexed. Expects mapped values (typically 1000-2000 representing PWM us).
  // SBUS native scale is approximately 172 to 1811.
  void setChannel(uint8_t channel, uint16_t value);
  void update();

private:
  int txPin;
  uint16_t channels[16];
  uint32_t lastSendMs;

  void sendPacket();
};
