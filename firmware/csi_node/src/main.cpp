#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

extern "C" {
#include "esp_wifi.h"
}

#include "CSIConfig.h"

#ifndef GD1_CSI_NODE_FIRMWARE
#define GD1_CSI_NODE_FIRMWARE "GD-1_CSI_Node"
#endif

namespace {

constexpr uint16_t GD1_MAX_CSI_BYTES = 384;
constexpr uint8_t GD1_QUEUE_DEPTH = 8;

struct CsiFrame {
  uint32_t sequence;
  uint32_t timestampMs;
  int8_t rssi;
  uint8_t channel;
  uint16_t length;
  int8_t data[GD1_MAX_CSI_BYTES];
};

QueueHandle_t csiQueue = nullptr;
WiFiUDP udp;
IPAddress serverIp;

uint32_t frameSequence = 0;
uint32_t lastHeartbeatMs = 0;
uint32_t lastReconnectAttemptMs = 0;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(GD1_WIFI_SSID, GD1_WIFI_PASSWORD);

  Serial.print("Connecting to router SSID: ");
  Serial.println(GD1_WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print('.');
  }

  Serial.println();
  Serial.print("WiFi connected, node IP: ");
  Serial.println(WiFi.localIP());

  esp_wifi_set_channel(GD1_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
}

void configureCsi() {
  wifi_csi_config_t csiConfig = {};
  csiConfig.lltf_en = true;
  csiConfig.htltf_en = true;
  csiConfig.stbc_htltf2_en = true;
  csiConfig.ltf_merge_en = true;
  csiConfig.channel_filter_en = false;
  csiConfig.manu_scale = false;
  csiConfig.shift = false;

  esp_wifi_set_csi_config(&csiConfig);
  esp_wifi_set_csi_rx_cb(
      [](void *ctx, wifi_csi_info_t *info) {
        (void)ctx;

        if (info == nullptr || info->buf == nullptr || csiQueue == nullptr) {
          return;
        }

        CsiFrame frame = {};
        frame.sequence = frameSequence++;
        frame.timestampMs = millis();
        frame.rssi = info->rx_ctrl.rssi;
        frame.channel = info->rx_ctrl.channel;
        frame.length = static_cast<uint16_t>(info->len > GD1_MAX_CSI_BYTES ? GD1_MAX_CSI_BYTES : info->len);

        for (uint16_t i = 0; i < frame.length; ++i) {
          frame.data[i] = info->buf[i];
        }

        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(csiQueue, &frame, &higherPriorityTaskWoken);

        if (higherPriorityTaskWoken == pdTRUE) {
          portYIELD_FROM_ISR();
        }
      },
      nullptr);
  esp_wifi_set_csi(true);
}

void appendHexByte(char *buffer, size_t &offset, size_t capacity, uint8_t value) {
  static constexpr char hex[] = "0123456789ABCDEF";

  if (offset + 2 >= capacity) {
    return;
  }

  buffer[offset++] = hex[(value >> 4) & 0x0F];
  buffer[offset++] = hex[value & 0x0F];
  buffer[offset] = '\0';
}

void sendCsiFrame(const CsiFrame &frame) {
  char payload[1100] = {};
  size_t offset = 0;

  const int written = snprintf(
      payload,
      sizeof(payload),
      "GD1CSI,1,%s,%lu,%lu,%d,%u,%u,",
      GD1_CSI_NODE_ID,
      static_cast<unsigned long>(frame.sequence),
      static_cast<unsigned long>(frame.timestampMs),
      static_cast<int>(frame.rssi),
      static_cast<unsigned int>(frame.channel),
      static_cast<unsigned int>(frame.length));

  if (written <= 0 || static_cast<size_t>(written) >= sizeof(payload)) {
    return;
  }

  offset = static_cast<size_t>(written);

  for (uint16_t i = 0; i < frame.length && offset + 2 < sizeof(payload); ++i) {
    appendHexByte(payload, offset, sizeof(payload), static_cast<uint8_t>(frame.data[i]));
  }

  udp.beginPacket(serverIp, GD1_CSI_SERVER_PORT);
  udp.write(reinterpret_cast<const uint8_t *>(payload), strlen(payload));
  udp.endPacket();
}

void sendHeartbeat() {
  char payload[160] = {};
  snprintf(
      payload,
      sizeof(payload),
      "GD1HEARTBEAT,1,%s,%lu,%d,%s",
      GD1_CSI_NODE_ID,
      static_cast<unsigned long>(millis()),
      static_cast<int>(WiFi.RSSI()),
      WiFi.localIP().toString().c_str());

  udp.beginPacket(serverIp, GD1_CSI_SERVER_PORT);
  udp.write(reinterpret_cast<const uint8_t *>(payload), strlen(payload));
  udp.endPacket();
}

} // namespace

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println(GD1_CSI_NODE_FIRMWARE);
  Serial.println("CSI-only sensing node. No camera, OpenCV, YOLO, or visual processing.");

  csiQueue = xQueueCreate(GD1_QUEUE_DEPTH, sizeof(CsiFrame));

  if (csiQueue == nullptr) {
    Serial.println("ERROR: Failed to create CSI queue");
    while (true) {
      delay(1000);
    }
  }

  serverIp.fromString(GD1_CSI_SERVER_IP);
  connectWiFi();
  udp.begin(0);
  configureCsi();

  Serial.print("Streaming CSI UDP to ");
  Serial.print(serverIp);
  Serial.print(':');
  Serial.println(GD1_CSI_SERVER_PORT);
}

void loop() {
  const uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED && now - lastReconnectAttemptMs >= GD1_CSI_RECONNECT_MS) {
    lastReconnectAttemptMs = now;
    connectWiFi();
  }

  CsiFrame frame = {};
  while (xQueueReceive(csiQueue, &frame, 0) == pdTRUE) {
    sendCsiFrame(frame);
  }

  if (now - lastHeartbeatMs >= GD1_CSI_HEARTBEAT_MS) {
    lastHeartbeatMs = now;
    sendHeartbeat();
  }
}

