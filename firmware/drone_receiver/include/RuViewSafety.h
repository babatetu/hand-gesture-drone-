#pragma once

#include <cstdint>

enum class RuViewMotion {
  None,
  Low,
  High,
  Unknown
};

enum class RuViewDirection {
  Front,
  Left,
  Right,
  Unknown
};

struct RuViewDecision {
  bool presence;
  RuViewMotion motion;
  RuViewDirection direction;
  uint32_t receivedAtMs;
  bool valid;
};

bool parseRuViewDecision(const char *json, uint32_t nowMs, RuViewDecision &decision);
bool shouldBlockForwardFromRuView(const RuViewDecision &decision, uint32_t nowMs, uint32_t timeoutMs);
const char *ruViewMotionName(RuViewMotion motion);
const char *ruViewDirectionName(RuViewDirection direction);

