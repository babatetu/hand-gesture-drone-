#pragma once

// ESP32 default I2C pins. Change these if your glove wiring uses other pins.
constexpr int GD1_I2C_SDA_PIN = 21;
constexpr int GD1_I2C_SCL_PIN = 22;
constexpr int GD1_MPU6050_ADDRESS = 0x68;

constexpr unsigned long GD1_SAMPLE_INTERVAL_MS = 20;       // 50 Hz IMU loop
constexpr unsigned long GD1_SERIAL_INTERVAL_MS = 100;      // 10 Hz debug output
constexpr unsigned int GD1_CALIBRATION_SAMPLES = 150;
constexpr unsigned int GD1_CALIBRATION_DELAY_MS = 8;

// Tune these after watching serial output with the glove on your hand.
constexpr float GD1_DEADZONE_DEG = 6.0f;
constexpr float GD1_MAX_TILT_DEG = 30.0f;
constexpr float GD1_LOW_PASS_ALPHA = 0.85f;

// Change signs if your mounted MPU6050 orientation is reversed.
constexpr float GD1_PITCH_COMMAND_SIGN = 1.0f;
constexpr float GD1_ROLL_COMMAND_SIGN = 1.0f;

