#include "GestureProcessor.h"

#include <cmath>

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

GestureCommand GestureProcessor::process(const GestureAttitude &attitude) const {
  const int8_t pitchCommand = normalizeAxis(attitude.pitchDeg);
  const int8_t rollCommand = normalizeAxis(attitude.rollDeg);

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

