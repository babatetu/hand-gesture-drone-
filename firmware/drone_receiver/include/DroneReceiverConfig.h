#pragma once

constexpr char GD1_DRONE_WIFI_SSID[] = "YOUR_ROUTER_SSID";
constexpr char GD1_DRONE_WIFI_PASSWORD[] = "YOUR_ROUTER_PASSWORD";

constexpr uint16_t GD1_RUVIEW_LISTEN_PORT = 5010;
constexpr uint32_t GD1_RUVIEW_TIMEOUT_MS = 500;

// Gesture Protocol
constexpr uint16_t GD1_GESTURE_LISTEN_PORT = 5011;
constexpr uint32_t GD1_GESTURE_TIMEOUT_MS = 500;

// RuView is secondary sensing. Ultrasonic remains the primary immediate obstacle
// safety source and should be checked before applying gesture commands.
constexpr bool GD1_ENABLE_RUVIEW_FORWARD_BLOCK = true;

// Ultrasonic Sensor Config
constexpr int GD1_ULTRASONIC_FRONT_TRIG_PIN = 14;
constexpr int GD1_ULTRASONIC_FRONT_ECHO_PIN = 27;
constexpr float GD1_ULTRASONIC_BLOCK_DISTANCE_CM = 100.0f; // Block if obstacle < 100cm
constexpr uint32_t GD1_ULTRASONIC_MEASURE_INTERVAL_MS = 50;

// SBUS Config
constexpr int GD1_SBUS_TX_PIN = 17; // UART2 TX
