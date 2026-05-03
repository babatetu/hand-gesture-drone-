#pragma once

constexpr char GD1_DRONE_WIFI_SSID[] = "YOUR_ROUTER_SSID";
constexpr char GD1_DRONE_WIFI_PASSWORD[] = "YOUR_ROUTER_PASSWORD";

constexpr uint16_t GD1_RUVIEW_LISTEN_PORT = 5010;
constexpr uint32_t GD1_RUVIEW_TIMEOUT_MS = 500;

// RuView is secondary sensing. Ultrasonic remains the primary immediate obstacle
// safety source and should be checked before applying gesture commands.
constexpr bool GD1_ENABLE_RUVIEW_FORWARD_BLOCK = true;

