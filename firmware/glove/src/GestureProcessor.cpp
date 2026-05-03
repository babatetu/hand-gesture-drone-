#include "GestureProcessor.h"

#include <cmath>

static constexpr float YAW_DEADBAND_DPS  = 5.0f;
static constexpr float YAW_MAX_RATE_DPS  = 60.0f;

namespace {

float clampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }

  if (value > maxValue) {
    return maxValue;
  }

  return value;
}

} // namespace

GestureProcessor::GestureProcessor(float deadzoneDeg, float maxTiltDeg)
    : deadzoneDeg_(std::fabs(deadzoneDeg)),
      maxTiltDeg_(std::fabs(maxTiltDeg) < 1.0f ? 1.0f : std::fabs(maxTiltDeg)) {}

// Yaw uses raw gyro Z rate (not integrated angle).
// Rate-based control eliminates gyro drift entirely.
// Deadband filters resting noise. Pixhawk compass handles
// absolute drone heading independently.
GestureCommand GestureProcessor::process(const GestureAttitude &attitude) const {
  const int8_t pitchCommand = normalizeAxis(attitude.pitchDeg);
  const int8_t rollCommand = normalizeAxis(attitude.rollDeg);

  float yawRate = attitude.yawRateDps;
  if (std::fabs(yawRate) < YAW_DEADBAND_DPS) {
    yawRate = 0.0f;
  }

  float normalizedYaw = clampFloat((yawRate / YAW_MAX_RATE_DPS) * 100.0f, -100.0f, 100.0f);
  int8_t yawCommand = static_cast<int8_t>(std::lround(normalizedYaw));

  GestureDirection direction = GestureDirection::Hover;
  const bool neutral = pitchCommand == 0 && rollCommand == 0;

  if (!neutral) {
    if (std::abs(pitchCommand) >= std::abs(rollCommand)) {
      direction = pitchCommand > 0 ? GestureDirection::Forward : GestureDirection::Backward;
    } else {
      direction = rollCommand > 0 ? GestureDirection::Right : GestureDirection::Left;
    }
  }

  return GestureCommand{
      pitchCommand,
      rollCommand,
      yawCommand,
      direction,
      neutral,
  };
}

int8_t GestureProcessor::normalizeAxis(float angleDeg) const {
  const float filteredAngle = applyDeadband(angleDeg);

  if (filteredAngle == 0.0f) {
    return 0;
  }

  const float normalized = clampFloat((filteredAngle / maxTiltDeg_) * 100.0f, -100.0f, 100.0f);
  return static_cast<int8_t>(std::lround(normalized));
}

float GestureProcessor::applyDeadband(float angleDeg) const {
  return std::fabs(angleDeg) < deadzoneDeg_ ? 0.0f : angleDeg;
}

const char *gestureDirectionName(GestureDirection direction) {
  switch (direction) {
  case GestureDirection::Hover:
    return "HOVER";
  case GestureDirection::Forward:
    return "FORWARD";
  case GestureDirection::Backward:
    return "BACKWARD";
  case GestureDirection::Left:
    return "LEFT";
  case GestureDirection::Right:
    return "RIGHT";
  }

  return "UNKNOWN";
}

