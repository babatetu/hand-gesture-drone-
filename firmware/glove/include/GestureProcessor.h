#pragma once

#include <cstdint>

enum class GestureDirection {
  Hover,
  Forward,
  Backward,
  Left,
  Right
};

struct GestureAttitude {
  float pitchDeg;
  float rollDeg;
};

struct GestureCommand {
  int8_t pitch;
  int8_t roll;
  GestureDirection direction;
  bool neutral;
};

class GestureProcessor {
public:
  GestureProcessor(float deadzoneDeg, float maxTiltDeg);

  GestureCommand process(const GestureAttitude &attitude) const;

private:
  float deadzoneDeg_;
  float maxTiltDeg_;

  int8_t normalizeAxis(float angleDeg) const;
  float applyDeadband(float angleDeg) const;
};

const char *gestureDirectionName(GestureDirection direction);

