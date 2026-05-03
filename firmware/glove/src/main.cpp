#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include <cmath>

#include "GD1Config.h"
#include "GD1Protocol.h"
#include "GestureProcessor.h"

#ifndef GD1_FIRMWARE_NAME
#define GD1_FIRMWARE_NAME "GD-1_Glove_FR1"
#endif

namespace {

Adafruit_MPU6050 mpu;
GestureProcessor gestureProcessor(GD1_DEADZONE_DEG, GD1_MAX_TILT_DEG);

RF24 radio(GD1_NRF24_CE_PIN, GD1_NRF24_CSN_PIN);
const uint64_t radioPipe = 0xE8E8F0F0E1LL;

GestureAttitude neutralOffset{0.0f, 0.0f};
GestureAttitude filteredAttitude{0.0f, 0.0f};
bool hasFilteredSample = false;

bool isFastMode = false;
unsigned long lastButtonPressMs = 0;

unsigned long lastSampleMs = 0;
unsigned long lastSerialMs = 0;
uint16_t commandSequence = 0;

float radiansToDegrees(float radians) {
  return radians * 57.2957795131f;
}

GestureAttitude calculateAttitude(const sensors_event_t &accel, const sensors_event_t &gyro) {
  const float ax = accel.acceleration.x;
  const float ay = accel.acceleration.y;
  const float az = accel.acceleration.z;

  const float pitch = std::atan2(-ax, std::sqrt((ay * ay) + (az * az)));
  const float roll = std::atan2(ay, az);

  return GestureAttitude{
      radiansToDegrees(pitch),
      radiansToDegrees(roll),
      radiansToDegrees(gyro.gyro.z),
  };
}

bool readAttitude(GestureAttitude &attitude) {
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;

  if (!mpu.getEvent(&accel, &gyro, &temp)) {
    return false;
  }

  attitude = calculateAttitude(accel, gyro);
  return true;
}

GestureAttitude lowPassFilter(const GestureAttitude &current) {
  if (!hasFilteredSample) {
    hasFilteredSample = true;
    filteredAttitude = current;
    return filteredAttitude;
  }

  filteredAttitude.pitchDeg =
      (GD1_LOW_PASS_ALPHA * filteredAttitude.pitchDeg) +
      ((1.0f - GD1_LOW_PASS_ALPHA) * current.pitchDeg);
  filteredAttitude.rollDeg =
      (GD1_LOW_PASS_ALPHA * filteredAttitude.rollDeg) +
      ((1.0f - GD1_LOW_PASS_ALPHA) * current.rollDeg);
  filteredAttitude.yawRateDps =
      (GD1_LOW_PASS_ALPHA * filteredAttitude.yawRateDps) +
      ((1.0f - GD1_LOW_PASS_ALPHA) * current.yawRateDps);

  return filteredAttitude;
}

bool calibrateNeutral() {
  Serial.println("Hold glove still in neutral position for calibration...");

  float pitchSum = 0.0f;
  float rollSum = 0.0f;
  unsigned int validSamples = 0;

  for (unsigned int i = 0; i < GD1_CALIBRATION_SAMPLES; ++i) {
    GestureAttitude sample;

    if (readAttitude(sample)) {
      pitchSum += sample.pitchDeg;
      rollSum += sample.rollDeg;
      validSamples++;
    }

    delay(GD1_CALIBRATION_DELAY_MS);
  }

  if (validSamples == 0) {
    return false;
  }

  neutralOffset.pitchDeg = pitchSum / static_cast<float>(validSamples);
  neutralOffset.rollDeg = rollSum / static_cast<float>(validSamples);

  filteredAttitude = neutralOffset;
  hasFilteredSample = true;

  Serial.print("Neutral pitch offset: ");
  Serial.println(neutralOffset.pitchDeg, 2);
  Serial.print("Neutral roll offset: ");
  Serial.println(neutralOffset.rollDeg, 2);

  return true;
}

GestureAttitude applyNeutralOffset(const GestureAttitude &attitude) {
  return GestureAttitude{
      (attitude.pitchDeg - neutralOffset.pitchDeg) * GD1_PITCH_COMMAND_SIGN,
      (attitude.rollDeg - neutralOffset.rollDeg) * GD1_ROLL_COMMAND_SIGN,
      attitude.yawRateDps, // Gyro rate doesn't need a static neutral offset
  };
}

void printCsvHeader() {
  Serial.println("ms,seq,pitch_deg,roll_deg,pitch_cmd,roll_cmd,packet_ok,gesture");
}

void printDebugRow(uint16_t sequence, const GestureAttitude &attitude, const GestureCommand &command, bool packetOk) {
  Serial.print(millis());
  Serial.print(',');
  Serial.print(sequence);
  Serial.print(',');
  Serial.print(attitude.pitchDeg, 2);
  Serial.print(',');
  Serial.print(attitude.rollDeg, 2);
  Serial.print(',');
  Serial.print(static_cast<int>(command.pitch));
  Serial.print(',');
  Serial.print(static_cast<int>(command.roll));
  Serial.print(',');
  Serial.print(packetOk ? "1" : "0");
  Serial.print(',');
  Serial.println(gestureDirectionName(command.direction));
}

void configureMpu() {
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void checkModeButton() {
  const unsigned long now = millis();
  // Button is active LOW (assuming built-in BOOT button with pullup)
  if (digitalRead(GD1_MODE_BUTTON_PIN) == LOW) {
    if (now - lastButtonPressMs > 500) { // 500ms debounce
      isFastMode = !isFastMode;
      lastButtonPressMs = now;
      Serial.print("Mode changed: ");
      Serial.println(isFastMode ? "FAST" : "STABLE");
    }
  }
}

} // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println(GD1_FIRMWARE_NAME);
  Serial.println("Phase 1: IMU gesture processing");

  Wire.begin(GD1_I2C_SDA_PIN, GD1_I2C_SCL_PIN);

  if (!mpu.begin(GD1_MPU6050_ADDRESS, &Wire)) {
    Serial.println("ERROR: MPU6050 not found. Check wiring and I2C address.");
    while (true) {
      delay(1000);
    }
  }

  configureMpu();

  if (!calibrateNeutral()) {
    Serial.println("ERROR: Calibration failed. Reboot and keep glove still.");
    while (true) {
      delay(1000);
    }
  }

  SPI.begin(18, 19, 23, 15); // SCK, MISO, MOSI, SS (CSN) - using standard ESP32 VSPI pins

  if (!radio.begin()) {
    Serial.println("ERROR: NRF24L01 radio hardware is not responding!");
    while (true) {
      delay(1000);
    }
  }

  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(radioPipe);
  radio.stopListening();

  pinMode(GD1_MODE_BUTTON_PIN, INPUT_PULLUP);

  printCsvHeader();
}

void loop() {
  const unsigned long now = millis();

  checkModeButton();

  if (now - lastSampleMs < GD1_SAMPLE_INTERVAL_MS) {
    return;
  }

  lastSampleMs = now;

  GestureAttitude rawAttitude;

  if (!readAttitude(rawAttitude)) {
    Serial.println("WARN: IMU read failed");
    return;
  }

  const GestureAttitude smoothedAttitude = lowPassFilter(rawAttitude);
  const GestureAttitude commandAttitude = applyNeutralOffset(smoothedAttitude);
  const GestureCommand command = gestureProcessor.process(commandAttitude);
  const uint16_t sequence = commandSequence++;

  uint8_t flags = GD1_FLAG_CALIBRATED;
  if (isFastMode) {
    flags |= GD1_FLAG_FAST_MODE;
  }

  uint8_t packet[GD1_COMMAND_PACKET_SIZE] = {};
  const GD1ControlCommand radioCommand{
      sequence,
      static_cast<uint32_t>(now),
      command.pitch,
      command.roll,
      0,
      command.yaw,
      flags,
  };
  const bool packetOk = gd1EncodeCommandPacket(radioCommand, packet, sizeof(packet));

  if (packetOk) {
    radio.write(packet, GD1_COMMAND_PACKET_SIZE);
  }

  if (now - lastSerialMs >= GD1_SERIAL_INTERVAL_MS) {
    lastSerialMs = now;
    printDebugRow(sequence, commandAttitude, command, packetOk);
  }
}
