#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include "DroneReceiverConfig.h"
#include "UltrasonicSensor.h"
#include "LD2410Driver.h"
#include "VL53L1XArray.h"
#include "GD1Protocol.h"
#include "SBUSGenerator.h"

#ifndef GD1_DRONE_RECEIVER_FIRMWARE
#define GD1_DRONE_RECEIVER_FIRMWARE "GD-1_Drone_Receiver_Full"
#endif

static constexpr uint16_t HARD_BLOCK_SIDE_CM  = 30;
static constexpr uint16_t HARD_BLOCK_REAR_CM  = 30;
static constexpr uint16_t HARD_BLOCK_UP_CM    = 40;
static constexpr uint16_t SOFT_WARN_SIDE_CM   = 80;
static constexpr uint16_t SOFT_WARN_REAR_CM   = 80;
static constexpr uint16_t SOFT_WARN_UP_CM     = 80;

namespace {

UltrasonicSensor frontUltrasonic(GD1_ULTRASONIC_FRONT_TRIG_PIN, GD1_ULTRASONIC_FRONT_ECHO_PIN, GD1_ULTRASONIC_MEASURE_INTERVAL_MS);
UltrasonicSensor bottomUltrasonic(GD1_ULTRASONIC_BOTTOM_TRIG_PIN, GD1_ULTRASONIC_BOTTOM_ECHO_PIN, GD1_ULTRASONIC_BOTTOM_MEASURE_INTERVAL_MS);
LD2410Driver radar(GD1_LD2410_RX_PIN, GD1_LD2410_TX_PIN);
VL53L1XArray tofArray;

RF24 radio(GD1_NRF24_CE_PIN, GD1_NRF24_CSN_PIN);
const uint64_t radioPipe = 0xE8E8F0F0E1LL;

GD1ControlCommand latestGestureCommand{};
uint32_t lastGestureReceivedMs = 0;
SBUSGenerator sbus(GD1_SBUS_TX_PIN);

int16_t gesturePitchCommand = 0;
int16_t gestureRollCommand = 0;
int16_t gestureThrottleCommand = 0;
int16_t gestureYawCommand = 0;
bool isFastMode = false;

int16_t safePitchCommand = 0;
int16_t safeThrottleCommand = 0;
int16_t previousThrottleCommand = 0;
uint16_t lastSafeThrottle = 0; // TODO: replace with ArduPilot GCS failsafe / RTL in future

bool ultrasonicBlocksForward() {
  return frontUltrasonic.isObstacleDetected(GD1_ULTRASONIC_BLOCK_DISTANCE_CM);
}

void applySafetyOverrides() {
  // Pitch Forward
  // Tier 1: Ultrasonic
  if (ultrasonicBlocksForward() && gesturePitchCommand > 0) {
    gesturePitchCommand = 0;
  }
  // Tier 2: LD2410 Radar
  else if (radar.isPersonDetected() && gesturePitchCommand > 0) {
    gesturePitchCommand = 0;
  }

  // Pitch Backward (Rear VL53L1X)
  if (tofArray.isOk(2) && gesturePitchCommand < 0) {
    uint16_t rearDist = tofArray.getRear();
    if (rearDist < HARD_BLOCK_REAR_CM) {
      gesturePitchCommand = 0;
      Serial.println("HARD BLOCK: REAR");
    } else if (rearDist < SOFT_WARN_REAR_CM) {
      gesturePitchCommand = static_cast<int16_t>(gesturePitchCommand * 0.5f);
    }
  }

  // Roll Left (Left VL53L1X)
  if (tofArray.isOk(0) && gestureRollCommand < 0) {
    uint16_t leftDist = tofArray.getLeft();
    if (leftDist < HARD_BLOCK_SIDE_CM) {
      gestureRollCommand = 0;
      Serial.println("HARD BLOCK: LEFT");
    } else if (leftDist < SOFT_WARN_SIDE_CM) {
      gestureRollCommand = static_cast<int16_t>(gestureRollCommand * 0.5f);
    }
  }

  // Roll Right (Right VL53L1X)
  if (tofArray.isOk(1) && gestureRollCommand > 0) {
    uint16_t rightDist = tofArray.getRight();
    if (rightDist < HARD_BLOCK_SIDE_CM) {
      gestureRollCommand = 0;
      Serial.println("HARD BLOCK: RIGHT");
    } else if (rightDist < SOFT_WARN_SIDE_CM) {
      gestureRollCommand = static_cast<int16_t>(gestureRollCommand * 0.5f);
    }
  }

  // Throttle Up (Up VL53L1X)
  if (tofArray.isOk(3) && gestureThrottleCommand > previousThrottleCommand) {
    uint16_t upDist = tofArray.getUp();
    if (upDist < HARD_BLOCK_UP_CM) {
      gestureThrottleCommand = previousThrottleCommand; // Clamp to previous safe state
      Serial.println("HARD BLOCK: UP");
    } else if (upDist < SOFT_WARN_UP_CM) {
      int16_t diff = gestureThrottleCommand - previousThrottleCommand;
      gestureThrottleCommand = previousThrottleCommand + static_cast<int16_t>(diff * 0.5f);
    }
  }
}

void handleGesturePacket() {
  if (!radio.available()) {
    return;
  }

  uint8_t buffer[32] = {};
  radio.read(buffer, GD1_COMMAND_PACKET_SIZE);

  GD1ControlCommand command;
  if (gd1DecodeCommandPacket(buffer, GD1_COMMAND_PACKET_SIZE, command)) {
    latestGestureCommand = command;
    lastGestureReceivedMs = millis();
  } else {
    Serial.println("WARN: Invalid gesture packet received.");
  }
}

void printSafetyDebug() {
  static uint32_t lastPrintMs = 0;
  const uint32_t now = millis();

  if (now - lastPrintMs < 250) {
    return;
  }

  lastPrintMs = now;

  Serial.print("f_dist=");
  Serial.print(frontUltrasonic.getDistanceCm(), 1);
  Serial.print(",b_dist=");
  Serial.print(bottomUltrasonic.getDistanceCm(), 1);
  Serial.print(",radar=");
  Serial.print(radar.isPersonDetected() ? "1" : "0");
  Serial.printf(" L:%dcm R:%dcm REAR:%dcm UP:%dcm\n",
    tofArray.getLeft(), tofArray.getRight(), tofArray.getRear(), tofArray.getUp());
}

} // namespace

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println(GD1_DRONE_RECEIVER_FIRMWARE);
  Serial.println("GD-1 Full Receiver: Ultrasonic + Radar + NRF24 + GD1Protocol + SBUS");

  frontUltrasonic.begin();
  bottomUltrasonic.begin();
  radar.begin();
  tofArray.begin();
  sbus.begin();

  SPI.begin(18, 19, 23, 15); // SCK, MISO, MOSI, SS (CSN) - using standard ESP32 VSPI pins

  if (!radio.begin()) {
    Serial.println("ERROR: NRF24L01 radio hardware is not responding!");
    while (true) {
      delay(1000);
    }
  }

  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(1, radioPipe);
  radio.startListening();

  Serial.println("Listening for GD1Protocol via NRF24L01");
}

void loop() {
  frontUltrasonic.update();
  bottomUltrasonic.update();
  radar.update();
  tofArray.update();
  handleGesturePacket();

  const uint32_t now = millis();

  // Failsafe: if no gesture received recently, hover
  if (now - lastGestureReceivedMs > GD1_GESTURE_TIMEOUT_MS) {
    gesturePitchCommand = 0;
    gestureRollCommand = 0;
    gestureYawCommand = 0;
    gestureThrottleCommand = lastSafeThrottle;
  } else {
    isFastMode = (latestGestureCommand.flags & GD1_FLAG_FAST_MODE) != 0;

    // In Stable Mode, scale inputs by 50% to restrict maximum lean angle
    float scale = isFastMode ? 1.0f : 0.5f;

    gesturePitchCommand = latestGestureCommand.pitch * scale;
    gestureRollCommand = latestGestureCommand.roll * scale;
    gestureThrottleCommand = latestGestureCommand.throttle;
    gestureYawCommand = latestGestureCommand.yaw * scale;
  }

  applySafetyOverrides();

  safePitchCommand = gesturePitchCommand;
  safeThrottleCommand = gestureThrottleCommand;

  // We explicitly update lastSafeThrottle AFTER all safety modifications
  lastSafeThrottle = safeThrottleCommand;

  // Soft landing guard:
  float alt = bottomUltrasonic.getDistanceCm();
  if (alt > 0.0f && alt < 30.0f) {
    if (safeThrottleCommand < previousThrottleCommand) {
      // TODO: tune threshold for maximum throttle reduction rate near ground
      int16_t maxReduction = 5;
      if ((previousThrottleCommand - safeThrottleCommand) > maxReduction) {
        safeThrottleCommand = previousThrottleCommand - maxReduction;
      }
    }
  }

  // Ensure throttle never goes below 0 to prevent SBUS calculation underflow
  if (safeThrottleCommand < 0) {
    safeThrottleCommand = 0;
  }

  previousThrottleCommand = safeThrottleCommand;

  // Map -100 to 100 range to SBUS 172-1811 (center 992)
  // Half range is ~819 (1811 - 992)
  uint16_t sbusPitch = 992 + (safePitchCommand * 819 / 100);
  uint16_t sbusRoll = 992 + (gestureRollCommand * 819 / 100);
  uint16_t sbusYaw = 992 + (gestureYawCommand * 819 / 100);
  // Throttle 0 to 100 mapping: 0 -> 172, 100 -> 1811 (diff 1639)
  uint16_t sbusThrottle = 172 + (safeThrottleCommand * 1639 / 100);

  // AETR mapping for SBUS: Ch1=Roll, Ch2=Pitch, Ch3=Throttle, Ch4=Yaw
  sbus.setChannel(0, sbusRoll);
  sbus.setChannel(1, sbusPitch);
  sbus.setChannel(2, sbusThrottle);
  sbus.setChannel(3, sbusYaw);

  sbus.update();

  printSafetyDebug();
}

