#pragma once

// NRF24L01 SPI Pins
constexpr int GD1_NRF24_CE_PIN = 5;
constexpr int GD1_NRF24_CSN_PIN = 15;

// Gesture Protocol
constexpr uint32_t GD1_GESTURE_TIMEOUT_MS = 500;

// Ultrasonic Sensor Config
constexpr int GD1_ULTRASONIC_FRONT_TRIG_PIN = 14;
constexpr int GD1_ULTRASONIC_FRONT_ECHO_PIN = 27;

// Bottom Ultrasonic (Altitude Assist) Config
constexpr int GD1_ULTRASONIC_BOTTOM_TRIG_PIN = 25;
constexpr int GD1_ULTRASONIC_BOTTOM_ECHO_PIN = 26; // Requires 5V -> 3.3V voltage divider

constexpr float GD1_ULTRASONIC_BLOCK_DISTANCE_CM = 100.0f; // Block if obstacle < 100cm
constexpr uint32_t GD1_ULTRASONIC_MEASURE_INTERVAL_MS = 50;
constexpr uint32_t GD1_ULTRASONIC_BOTTOM_MEASURE_INTERVAL_MS = 100;

// LD2410 Radar Config
constexpr int GD1_LD2410_RX_PIN = 16;
constexpr int GD1_LD2410_TX_PIN = 4;

// SBUS Config
constexpr int GD1_SBUS_TX_PIN = 17; // UART2 TX
