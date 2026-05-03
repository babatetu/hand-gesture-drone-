#include "LD2410Driver.h"

// The LD2410 standard data frame is 24 bytes long.
// Frame Header: 0xF4, 0xF3, 0xF2, 0xF1
// Frame Tail: 0xF8, 0xF7, 0xF6, 0xF5

LD2410Driver::LD2410Driver(int rxPin, int txPin)
    : rxPin(rxPin), txPin(txPin), radarSerial(1), personDetected(false), detectionDistanceCm(0), bufferIndex(0) {}

void LD2410Driver::begin() {
  radarSerial.begin(256000, SERIAL_8N1, rxPin, txPin);
}

void LD2410Driver::update() {
  while (radarSerial.available()) {
    uint8_t b = radarSerial.read();

    // Check for frame header sync
    if (bufferIndex == 0 && b != 0xF4) continue;
    if (bufferIndex == 1 && b != 0xF3) { bufferIndex = 0; continue; }
    if (bufferIndex == 2 && b != 0xF2) { bufferIndex = 0; continue; }
    if (bufferIndex == 3 && b != 0xF1) { bufferIndex = 0; continue; }

    buffer[bufferIndex++] = b;

    if (bufferIndex >= 24) {
      // Check frame tail sync
      if (buffer[20] == 0xF8 && buffer[21] == 0xF7 && buffer[22] == 0xF6 && buffer[23] == 0xF5) {
        parseFrame();
      }
      bufferIndex = 0;
    }
  }
}

void LD2410Driver::parseFrame() {
  // Target status is at byte 8 (0: no target, 1: moving, 2: stationary, 3: moving & stationary)
  uint8_t targetStatus = buffer[8];
  personDetected = (targetStatus != 0x00);

  if (personDetected) {
    // If moving target (1 or 3), distance is at bytes 9-10 (Little Endian).
    // If stationary target (2), distance is at bytes 12-13.
    // If both (3), distance is provided for moving target at 9-10, and stationary at 12-13.
    // We'll prioritize moving target distance if available, else stationary.
    if (targetStatus == 0x01 || targetStatus == 0x03) {
      detectionDistanceCm = buffer[9] | (buffer[10] << 8);
    } else if (targetStatus == 0x02) {
      detectionDistanceCm = buffer[12] | (buffer[13] << 8);
    }
  } else {
    detectionDistanceCm = 0;
  }
}

bool LD2410Driver::isPersonDetected() const {
  return personDetected;
}

uint16_t LD2410Driver::getDistanceCm() const {
  return detectionDistanceCm;
}
