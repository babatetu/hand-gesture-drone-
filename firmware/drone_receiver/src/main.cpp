#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "DroneReceiverConfig.h"
#include "RuViewSafety.h"

#ifndef GD1_DRONE_RECEIVER_FIRMWARE
#define GD1_DRONE_RECEIVER_FIRMWARE "GD-1_Drone_Receiver_RuView"
#endif

namespace {

WiFiUDP udp;
RuViewDecision latestRuViewDecision{false, RuViewMotion::None, RuViewDirection::Unknown, 0, false};

int16_t gesturePitchCommand = 0;
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
  // Placeholder for primary safety. Wire HC-SR04 checks here before flight tests.
  return false;
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

void readRuViewJson() {
  const int packetSize = udp.parsePacket();

  if (packetSize <= 0) {
    return;
  }

  char packet[160] = {};
  const int bytesRead = udp.read(packet, sizeof(packet) - 1);

  if (bytesRead <= 0) {
    return;
  }

  packet[bytesRead] = '\0';

  RuViewDecision parsed{};

  if (parseRuViewDecision(packet, millis(), parsed)) {
    latestRuViewDecision = parsed;
  } else {
    Serial.print("WARN: Invalid RuView JSON: ");
    Serial.println(packet);
  }
}

void printSafetyDebug() {
  static uint32_t lastPrintMs = 0;
  const uint32_t now = millis();

  if (now - lastPrintMs < 250) {
    return;
  }

  lastPrintMs = now;

  Serial.print("gesture_pitch=");
  Serial.print(gesturePitchCommand);
  Serial.print(",safe_pitch=");
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
  Serial.println("RuView CSI JSON integration. No camera, OpenCV, YOLO, or visual processing.");

  connectWiFi();
  udp.begin(GD1_RUVIEW_LISTEN_PORT);

  Serial.print("Listening for RuView JSON on UDP port ");
  Serial.println(GD1_RUVIEW_LISTEN_PORT);
}

void loop() {
  readRuViewJson();

  // Replace this placeholder with the decoded gesture pitch command from FR2/FR3.
  gesturePitchCommand = 40;
  safePitchCommand = applySafetyOverrides(gesturePitchCommand);

  printSafetyDebug();
}

