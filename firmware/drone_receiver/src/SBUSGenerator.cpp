#include "SBUSGenerator.h"

// Hardware Serial 2 is used for SBUS
#define SBUS_SERIAL Serial2

SBUSGenerator::SBUSGenerator(int txPin) : txPin(txPin), lastSendMs(0) {
  for (int i = 0; i < 16; ++i) {
    channels[i] = 992; // Default to neutral SBUS midpoint
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

  // Pack 16 channels of 11-bits each into 22 bytes
  packet[1]  = (uint8_t)(channels[0] & 0xFF);
  packet[2]  = (uint8_t)((channels[0] >> 8) | (channels[1] << 3));
  packet[3]  = (uint8_t)((channels[1] >> 5) | (channels[2] << 6));
  packet[4]  = (uint8_t)(channels[2] >> 2);
  packet[5]  = (uint8_t)((channels[2] >> 10) | (channels[3] << 1));
  packet[6]  = (uint8_t)((channels[3] >> 7) | (channels[4] << 4));
  packet[7]  = (uint8_t)((channels[4] >> 4) | (channels[5] << 7));
  packet[8]  = (uint8_t)(channels[5] >> 1);
  packet[9]  = (uint8_t)((channels[5] >> 9) | (channels[6] << 2));
  packet[10] = (uint8_t)((channels[6] >> 6) | (channels[7] << 5));
  packet[11] = (uint8_t)(channels[7] >> 3);
  packet[12] = (uint8_t)((channels[7] >> 11) | (channels[8] << 0));
  packet[13] = (uint8_t)(channels[8] >> 8 | (channels[9] << 3));
  packet[14] = (uint8_t)(channels[9] >> 5 | (channels[10] << 6));
  packet[15] = (uint8_t)(channels[10] >> 2);
  packet[16] = (uint8_t)(channels[10] >> 10 | (channels[11] << 1));
  packet[17] = (uint8_t)(channels[11] >> 7 | (channels[12] << 4));
  packet[18] = (uint8_t)(channels[12] >> 4 | (channels[13] << 7));
  packet[19] = (uint8_t)(channels[13] >> 1);
  packet[20] = (uint8_t)(channels[13] >> 9 | (channels[14] << 2));
  packet[21] = (uint8_t)(channels[14] >> 6 | (channels[15] << 5));
  packet[22] = (uint8_t)(channels[15] >> 3);

  // Flags: Failsafe, Frame Lost
  packet[23] = 0x00;

  // Footer
  packet[24] = 0x00;

  SBUS_SERIAL.write(packet, 25);
}
