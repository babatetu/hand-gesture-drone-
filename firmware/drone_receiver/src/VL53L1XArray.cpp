#include "VL53L1XArray.h"

VL53L1XArray::VL53L1XArray() {
  for (int i = 0; i < 4; ++i) {
    distanceCm[i] = 999;
    sensorOk[i] = false;
  }
}

void VL53L1XArray::begin() {
  Wire.begin();

  for (int i = 0; i < 4; ++i) {
    pinMode(xshutPins[i], OUTPUT);
    digitalWrite(xshutPins[i], LOW);
  }

  // Small delay to ensure all sensors reset. Boot process so delay is acceptable.
  delay(10);

  for (int i = 0; i < 4; ++i) {
    digitalWrite(xshutPins[i], HIGH);
    delay(10); // Give sensor time to boot

    sensors[i].setBus(&Wire);

    // Initialize sensor
    if (!sensors[i].init()) {
      Serial.printf("WARN: VL53L1X init failed on sensor index %d (Pin %d)\n", i, xshutPins[i]);
      sensorOk[i] = false;
      continue;
    }

    // Set custom I2C address
    sensors[i].setAddress(addresses[i]);
    sensors[i].setDistanceMode(VL53L1X::Medium);
    sensors[i].setMeasurementTimingBudget(50000); // 50ms budget
    sensors[i].startContinuous(50); // 50ms interval

    sensorOk[i] = true;
    Serial.printf("VL53L1X init success on sensor index %d (Address 0x%02X)\n", i, addresses[i]);
  }
}

void VL53L1XArray::update() {
  for (int i = 0; i < 4; ++i) {
    if (sensorOk[i] && sensors[i].dataReady()) {
      // Read distance in mm and convert to cm
      uint16_t distMm = sensors[i].read();
      if (!sensors[i].timeoutOccurred()) {
        distanceCm[i] = distMm / 10;
      }
    }
  }
}

uint16_t VL53L1XArray::getLeft() const { return distanceCm[0]; }
uint16_t VL53L1XArray::getRight() const { return distanceCm[1]; }
uint16_t VL53L1XArray::getRear() const { return distanceCm[2]; }
uint16_t VL53L1XArray::getUp() const { return distanceCm[3]; }

bool VL53L1XArray::isOk(uint8_t index) const {
  if (index >= 4) return false;
  return sensorOk[index];
}
