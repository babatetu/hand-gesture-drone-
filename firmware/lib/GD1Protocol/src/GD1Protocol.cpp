#include "GD1Protocol.h"

namespace {

constexpr std::size_t GD1_MAGIC_INDEX = 0;
constexpr std::size_t GD1_VERSION_INDEX = 1;
constexpr std::size_t GD1_FLAGS_INDEX = 2;
constexpr std::size_t GD1_SEQUENCE_INDEX = 3;
constexpr std::size_t GD1_TIMESTAMP_INDEX = 5;
constexpr std::size_t GD1_PITCH_INDEX = 9;
constexpr std::size_t GD1_ROLL_INDEX = 10;
constexpr std::size_t GD1_THROTTLE_INDEX = 11;
constexpr std::size_t GD1_YAW_INDEX = 12;
constexpr std::size_t GD1_CRC_INDEX = 13;

void writeUint16(uint8_t *buffer, std::size_t index, uint16_t value) {
  buffer[index] = static_cast<uint8_t>(value & 0xFF);
  buffer[index + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void writeUint32(uint8_t *buffer, std::size_t index, uint32_t value) {
  buffer[index] = static_cast<uint8_t>(value & 0xFF);
  buffer[index + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  buffer[index + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  buffer[index + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

uint16_t readUint16(const uint8_t *buffer, std::size_t index) {
  return static_cast<uint16_t>(buffer[index]) |
         (static_cast<uint16_t>(buffer[index + 1]) << 8);
}

uint32_t readUint32(const uint8_t *buffer, std::size_t index) {
  return static_cast<uint32_t>(buffer[index]) |
         (static_cast<uint32_t>(buffer[index + 1]) << 8) |
         (static_cast<uint32_t>(buffer[index + 2]) << 16) |
         (static_cast<uint32_t>(buffer[index + 3]) << 24);
}

} // namespace

bool gd1EncodeCommandPacket(const GD1ControlCommand &command, uint8_t *buffer, std::size_t length) {
  if (buffer == nullptr || length < GD1_COMMAND_PACKET_SIZE) {
    return false;
  }

  buffer[GD1_MAGIC_INDEX] = GD1_PACKET_MAGIC;
  buffer[GD1_VERSION_INDEX] = GD1_PROTOCOL_VERSION;
  buffer[GD1_FLAGS_INDEX] = command.flags;
  writeUint16(buffer, GD1_SEQUENCE_INDEX, command.sequence);
  writeUint32(buffer, GD1_TIMESTAMP_INDEX, command.timestampMs);
  buffer[GD1_PITCH_INDEX] = static_cast<uint8_t>(command.pitch);
  buffer[GD1_ROLL_INDEX] = static_cast<uint8_t>(command.roll);
  buffer[GD1_THROTTLE_INDEX] = static_cast<uint8_t>(command.throttle);
  buffer[GD1_YAW_INDEX] = static_cast<uint8_t>(command.yaw);

  const uint16_t crc = gd1Crc16Ccitt(buffer, GD1_CRC_INDEX);
  writeUint16(buffer, GD1_CRC_INDEX, crc);

  return true;
}

bool gd1DecodeCommandPacket(const uint8_t *buffer, std::size_t length, GD1ControlCommand &command) {
  if (buffer == nullptr || length < GD1_COMMAND_PACKET_SIZE) {
    return false;
  }

  if (buffer[GD1_MAGIC_INDEX] != GD1_PACKET_MAGIC ||
      buffer[GD1_VERSION_INDEX] != GD1_PROTOCOL_VERSION) {
    return false;
  }

  const uint16_t expectedCrc = readUint16(buffer, GD1_CRC_INDEX);
  const uint16_t actualCrc = gd1Crc16Ccitt(buffer, GD1_CRC_INDEX);

  if (expectedCrc != actualCrc) {
    return false;
  }

  command.flags = buffer[GD1_FLAGS_INDEX];
  command.sequence = readUint16(buffer, GD1_SEQUENCE_INDEX);
  command.timestampMs = readUint32(buffer, GD1_TIMESTAMP_INDEX);
  command.pitch = static_cast<int8_t>(buffer[GD1_PITCH_INDEX]);
  command.roll = static_cast<int8_t>(buffer[GD1_ROLL_INDEX]);
  command.throttle = static_cast<int8_t>(buffer[GD1_THROTTLE_INDEX]);
  command.yaw = static_cast<int8_t>(buffer[GD1_YAW_INDEX]);

  return true;
}

uint16_t gd1Crc16Ccitt(const uint8_t *data, std::size_t length) {
  uint16_t crc = 0xFFFF;

  for (std::size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;

    for (uint8_t bit = 0; bit < 8; ++bit) {
      if ((crc & 0x8000) != 0) {
        crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
      } else {
        crc = static_cast<uint16_t>(crc << 1);
      }
    }
  }

  return crc;
}

