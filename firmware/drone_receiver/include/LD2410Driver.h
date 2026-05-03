#pragma once

#include <Arduino.h>
#include <cstdint>

class LD2410Driver {
public:
  LD2410Driver(int rxPin, int txPin);

  void begin();
  void update();

  bool isPersonDetected() const;
  uint16_t getDistanceCm() const;

private:
  int rxPin;
  int txPin;
  HardwareSerial radarSerial;

  bool personDetected;
  uint16_t detectionDistanceCm;

  uint8_t buffer[32];
  uint8_t bufferIndex;

  void parseFrame();
};
