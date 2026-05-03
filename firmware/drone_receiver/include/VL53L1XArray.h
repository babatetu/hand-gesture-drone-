#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>

#define XSHUT_LEFT   13
#define XSHUT_RIGHT  12
#define XSHUT_REAR   32
#define XSHUT_UP     33

#define ADDR_LEFT   0x30
#define ADDR_RIGHT  0x31
#define ADDR_REAR   0x32
#define ADDR_UP     0x33

class VL53L1XArray {
public:
  VL53L1XArray();

  void begin();
  void update();

  uint16_t getLeft() const;
  uint16_t getRight() const;
  uint16_t getRear() const;
  uint16_t getUp() const;
  bool isOk(uint8_t index) const;

private:
  VL53L1X sensors[4];
  uint16_t distanceCm[4];
  bool sensorOk[4];

  const int xshutPins[4] = {XSHUT_LEFT, XSHUT_RIGHT, XSHUT_REAR, XSHUT_UP};
  const uint8_t addresses[4] = {ADDR_LEFT, ADDR_RIGHT, ADDR_REAR, ADDR_UP};

  // State machine for non-blocking initialization if needed,
  // but PRD specifies using a simple delay sequence during begin()
  // which is typically run once at boot.
};
