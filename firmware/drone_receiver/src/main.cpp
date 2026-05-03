#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>

#include "DroneReceiverConfig.h"
#include "RuViewSafety.h"
#include "UltrasonicSensor.h"
#include "GD1Protocol.h"
#include "SBUSGenerator.h"

#ifndef GD1_DRONE_RECEIVER_FIRMWARE
#define GD1_DRONE_RECEIVER_FIRMWARE "GD-1_Drone_Receiver_Full"
#endif

namespace {

WebServer server(GD1_RUVIEW_LISTEN_PORT);
RuViewDecision latestRuViewDecision{false, RuViewMotion::None, RuViewDirection::Unknown, 0, false};

UltrasonicSensor frontUltrasonic(GD1_ULTRASONIC_FRONT_TRIG_PIN, GD1_ULTRASONIC_FRONT_ECHO_PIN);
WiFiUDP gestureUdp;
GD1ControlCommand latestGestureCommand{};
uint32_t lastGestureReceivedMs = 0;
SBUSGenerator sbus(GD1_SBUS_TX_PIN);

int16_t gesturePitchCommand = 0;
int16_t gestureRollCommand = 0;
int16_t gestureThrottleCommand = 0;
int16_t gestureYawCommand = 0;

int16_t safePitchCommand = 0;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(GD1_DRONE_WIFI_SSID, GD1_DRONE_WIFI_PASSWORD);

  Serial.print("Connecting drone receiver to router SSID: ");
  Serial.println(GD1_DRONE_WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print('.');
  }

  Serial.println();
  Serial.print("Drone receiver IP: ");
  Serial.println(WiFi.localIP());
}

bool ultrasonicBlocksForward() {
  return frontUltrasonic.isObstacleDetected(GD1_ULTRASONIC_BLOCK_DISTANCE_CM);
}

int16_t applySafetyOverrides(int16_t requestedPitch) {
  const uint32_t now = millis();

  if (ultrasonicBlocksForward() && requestedPitch > 0) {
    return 0;
  }

  if (GD1_ENABLE_RUVIEW_FORWARD_BLOCK &&
      shouldBlockForwardFromRuView(latestRuViewDecision, now, GD1_RUVIEW_TIMEOUT_MS) &&
      requestedPitch > 0) {
    return 0;
  }

  return requestedPitch;
}

void handleGesturePacket() {
  const int packetSize = gestureUdp.parsePacket();
  if (packetSize <= 0) {
    return;
  }

  uint8_t buffer[64] = {};
  const int bytesRead = gestureUdp.read(buffer, sizeof(buffer));

  if (bytesRead > 0) {
    GD1ControlCommand command;
    if (gd1DecodeCommandPacket(buffer, bytesRead, command)) {
      latestGestureCommand = command;
      lastGestureReceivedMs = millis();
    } else {
      Serial.println("WARN: Invalid gesture packet received.");
    }
  }
}

void handleRuView() {
  if (server.hasArg("plain")) {
    String payload = server.arg("plain");
    RuViewDecision parsed{};

    if (parseRuViewDecision(payload.c_str(), millis(), parsed)) {
      latestRuViewDecision = parsed;
      server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      Serial.print("WARN: Invalid RuView JSON: ");
      Serial.println(payload);
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"invalid json\"}");
    }
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"missing body\"}");
  }
}

void printSafetyDebug() {
  static uint32_t lastPrintMs = 0;
  const uint32_t now = millis();

  if (now - lastPrintMs < 250) {
    return;
  }

  lastPrintMs = now;

  Serial.print("dist=");
  Serial.print(frontUltrasonic.getDistanceCm(), 1);
  Serial.print(",g_pitch=");
  Serial.print(gesturePitchCommand);
  Serial.print(",s_pitch=");
  Serial.print(safePitchCommand);
  Serial.print(",ruview_presence=");
  Serial.print(latestRuViewDecision.presence ? "true" : "false");
  Serial.print(",ruview_motion=");
  Serial.print(ruViewMotionName(latestRuViewDecision.motion));
  Serial.print(",ruview_direction=");
  Serial.println(ruViewDirectionName(latestRuViewDecision.direction));
}

} // namespace

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println(GD1_DRONE_RECEIVER_FIRMWARE);
  Serial.println("GD-1 Full Receiver: Ultrasonic + RuView + GD1Protocol + SBUS");

  frontUltrasonic.begin();
  sbus.begin();

  connectWiFi();

  server.on("/ruview", HTTP_POST, handleRuView);
  server.begin();

  Serial.print("Listening for RuView JSON on HTTP port ");
  Serial.println(GD1_RUVIEW_LISTEN_PORT);

  gestureUdp.begin(GD1_GESTURE_LISTEN_PORT);
  Serial.print("Listening for GD1Protocol via UDP on port ");
  Serial.println(GD1_GESTURE_LISTEN_PORT);
}

void loop() {
  server.handleClient();
  frontUltrasonic.update();
  handleGesturePacket();

  const uint32_t now = millis();

  // Failsafe: if no gesture received recently, hover
  if (now - lastGestureReceivedMs > GD1_GESTURE_TIMEOUT_MS) {
    gesturePitchCommand = 0;
    gestureRollCommand = 0;
    gestureThrottleCommand = 0;
    gestureYawCommand = 0;
  } else {
    gesturePitchCommand = latestGestureCommand.pitch;
    gestureRollCommand = latestGestureCommand.roll;
    gestureThrottleCommand = latestGestureCommand.throttle;
    gestureYawCommand = latestGestureCommand.yaw;
  }

  safePitchCommand = applySafetyOverrides(gesturePitchCommand);

  // Map -100 to 100 range to 1000-2000 PWM
  uint16_t pwmPitch = 1500 + (safePitchCommand * 5);
  uint16_t pwmRoll = 1500 + (gestureRollCommand * 5);
  uint16_t pwmYaw = 1500 + (gestureYawCommand * 5);
  uint16_t pwmThrottle = 1000 + (gestureThrottleCommand * 10); // Assume throttle is 0 to 100

  // AETR mapping for SBUS: Ch1=Roll, Ch2=Pitch, Ch3=Throttle, Ch4=Yaw
  sbus.setChannel(0, pwmRoll);
  sbus.setChannel(1, pwmPitch);
  sbus.setChannel(2, pwmThrottle);
  sbus.setChannel(3, pwmYaw);

  sbus.update();

  printSafetyDebug();
}

