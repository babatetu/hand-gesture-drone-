#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "DroneReceiverConfig.h"
#include "RuViewSafety.h"

#ifndef GD1_DRONE_RECEIVER_FIRMWARE
#define GD1_DRONE_RECEIVER_FIRMWARE "GD-1_Drone_Receiver_RuView"
#endif

namespace {

WebServer server(GD1_RUVIEW_LISTEN_PORT);
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

  server.on("/ruview", HTTP_POST, handleRuView);
  server.begin();

  Serial.print("Listening for RuView JSON on HTTP port ");
  Serial.println(GD1_RUVIEW_LISTEN_PORT);
}

void loop() {
  server.handleClient();

  // Replace this placeholder with the decoded gesture pitch command from FR2/FR3.
  gesturePitchCommand = 40;
  safePitchCommand = applySafetyOverrides(gesturePitchCommand);

  printSafetyDebug();
}

