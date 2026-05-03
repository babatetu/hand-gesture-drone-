#pragma once

#include <cstddef>
#include <cstdint>

constexpr uint8_t GD1_PACKET_MAGIC = 0xD1;
constexpr uint8_t GD1_PROTOCOL_VERSION = 1;
constexpr std::size_t GD1_COMMAND_PACKET_SIZE = 15;

enum GD1CommandFlag : uint8_t {
  GD1_FLAG_NONE = 0,
  GD1_FLAG_EMERGENCY_STOP = 1 << 0,
  GD1_FLAG_FAILSAFE = 1 << 1,
  GD1_FLAG_CALIBRATED = 1 << 2
};

struct GD1ControlCommand {
  uint16_t sequence;
  uint32_t timestampMs;
  int8_t pitch;
  int8_t roll;
  int8_t throttle;
  int8_t yaw;
  uint8_t flags;
};

bool gd1EncodeCommandPacket(const GD1ControlCommand &command, uint8_t *buffer, std::size_t length);
bool gd1DecodeCommandPacket(const uint8_t *buffer, std::size_t length, GD1ControlCommand &command);
uint16_t gd1Crc16Ccitt(const uint8_t *data, std::size_t length);

