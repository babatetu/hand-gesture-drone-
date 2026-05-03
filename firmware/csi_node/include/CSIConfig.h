#pragma once

// Copy this file into your private config flow or edit these constants locally.
// Keep the router as the RF source. Do not add camera or visual sensing hardware.

constexpr char GD1_WIFI_SSID[] = "YOUR_ROUTER_SSID";
constexpr char GD1_WIFI_PASSWORD[] = "YOUR_ROUTER_PASSWORD";

// Label each CSI node by where it is mounted relative to the drone.
// Use "front", "left", or "right" so the server can infer direction.
constexpr char GD1_CSI_NODE_ID[] = "front";

constexpr char GD1_CSI_SERVER_IP[] = "192.168.1.20";
constexpr uint16_t GD1_CSI_SERVER_PORT = 5006;

constexpr uint8_t GD1_WIFI_CHANNEL = 1;
constexpr uint32_t GD1_CSI_HEARTBEAT_MS = 1000;
constexpr uint32_t GD1_CSI_RECONNECT_MS = 5000;

