#include "SBUSGenerator.h"

// Hardware Serial 2 is used for SBUS
#define SBUS_SERIAL Serial2

SBUSGenerator::SBUSGenerator(int txPin) : txPin(txPin), lastSendMs(0) {
  for (int i = 0; i < 16; ++i) {
    channels[i] = 1500; // Default to neutral PWM
  }
}

void SBUSGenerator::begin() {
  // SBUS is 100k baud, 8 data bits, even parity, 2 stop bits, INVERTED logic.
  // The ESP32 HardwareSerial supports inverted TX natively.
  SBUS_SERIAL.begin(100000, SERIAL_8E2, -1, txPin, true);
}

void SBUSGenerator::setChannel(uint8_t channel, uint16_t value) {
  if (channel < 16) {
    channels[channel] = value;
  }
}

void SBUSGenerator::update() {
  const uint32_t now = millis();
  // SBUS is typically sent every 7 to 14 ms. We'll use 14 ms.
  if (now - lastSendMs >= 14) {
    lastSendMs = now;
    sendPacket();
  }
}

void SBUSGenerator::sendPacket() {
  uint8_t packet[25] = {0};

  packet[0] = 0x0F; // Header

  // Convert PWM range (1000-2000) to SBUS range (172-1811)
  uint16_t sbusChannels[16];
  for (int i = 0; i < 16; ++i) {
    // Basic mapping: 1000us -> 172, 1500us -> 992, 2000us -> 1811
    // (1811 - 172) / (2000 - 1000) = 1.639 multiplier
    int32_t mapped = 172 + (static_cast<int32_t>(channels[i]) - 1000) * 1639 / 1000;
    if (mapped < 172) mapped = 172;
    if (mapped > 1811) mapped = 1811;
    sbusChannels[i] = static_cast<uint16_t>(mapped);
  }

  // Pack 16 channels of 11-bits each into 22 bytes
  packet[1]  = (uint8_t)(sbusChannels[0] & 0xFF);
  packet[2]  = (uint8_t)((sbusChannels[0] >> 8) | (sbusChannels[1] << 3));
  packet[3]  = (uint8_t)((sbusChannels[1] >> 5) | (sbusChannels[2] << 6));
  packet[4]  = (uint8_t)(sbusChannels[2] >> 2);
  packet[5]  = (uint8_t)((sbusChannels[2] >> 10) | (sbusChannels[3] << 1));
  packet[6]  = (uint8_t)((sbusChannels[3] >> 7) | (sbusChannels[4] << 4));
  packet[7]  = (uint8_t)((sbusChannels[4] >> 4) | (sbusChannels[5] << 7));
  packet[8]  = (uint8_t)(sbusChannels[5] >> 1);
  packet[9]  = (uint8_t)((sbusChannels[5] >> 9) | (sbusChannels[6] << 2));
  packet[10] = (uint8_t)((sbusChannels[6] >> 6) | (sbusChannels[7] << 5));
  packet[11] = (uint8_t)(sbusChannels[7] >> 3);
  packet[12] = (uint8_t)((sbusChannels[7] >> 11) | (sbusChannels[8] << 0));
  packet[13] = (uint8_t)(sbusChannels[8] >> 8 | (sbusChannels[9] << 3));
  packet[14] = (uint8_t)(sbusChannels[9] >> 5 | (sbusChannels[10] << 6));
  packet[15] = (uint8_t)(sbusChannels[10] >> 2);
  packet[16] = (uint8_t)(sbusChannels[10] >> 10 | (sbusChannels[11] << 1));
  packet[17] = (uint8_t)(sbusChannels[11] >> 7 | (sbusChannels[12] << 4));
  packet[18] = (uint8_t)(sbusChannels[12] >> 4 | (sbusChannels[13] << 7));
  packet[19] = (uint8_t)(sbusChannels[13] >> 1);
  packet[20] = (uint8_t)(sbusChannels[13] >> 9 | (sbusChannels[14] << 2));
  packet[21] = (uint8_t)(sbusChannels[14] >> 6 | (sbusChannels[15] << 5));
  packet[22] = (uint8_t)(sbusChannels[15] >> 3);

  // Flags: Failsafe, Frame Lost
  packet[23] = 0x00;

  // Footer
  packet[24] = 0x00;

  SBUS_SERIAL.write(packet, 25);
}
