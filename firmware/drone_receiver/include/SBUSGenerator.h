#pragma once

#include <Arduino.h>
#include <cstdint>

class SBUSGenerator {
public:
  SBUSGenerator(int txPin);

  void begin();

  // Channels are 0-indexed. Expects mapped values in standard SBUS range (172-1811, center 992).
  void setChannel(uint8_t channel, uint16_t value);
  void update();

private:
  int txPin;
  uint16_t channels[16];
  uint32_t lastSendMs;

  void sendPacket();
};
