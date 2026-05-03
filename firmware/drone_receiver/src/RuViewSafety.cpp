#include "RuViewSafety.h"

#include <cstring>

namespace {

bool containsToken(const char *json, const char *token) {
  return json != nullptr && std::strstr(json, token) != nullptr;
}

RuViewMotion parseMotion(const char *json) {
  if (containsToken(json, "\"motion\":\"none\"")) {
    return RuViewMotion::None;
  }

  if (containsToken(json, "\"motion\":\"low\"")) {
    return RuViewMotion::Low;
  }

  if (containsToken(json, "\"motion\":\"high\"")) {
    return RuViewMotion::High;
  }

  return RuViewMotion::Unknown;
}

RuViewDirection parseDirection(const char *json) {
  if (containsToken(json, "\"direction\":\"front\"")) {
    return RuViewDirection::Front;
  }

  if (containsToken(json, "\"direction\":\"left\"")) {
    return RuViewDirection::Left;
  }

  if (containsToken(json, "\"direction\":\"right\"")) {
    return RuViewDirection::Right;
  }

  return RuViewDirection::Unknown;
}

} // namespace

bool parseRuViewDecision(const char *json, uint32_t nowMs, RuViewDecision &decision) {
  if (json == nullptr || json[0] == '\0') {
    return false;
  }

  const bool presence =
      containsToken(json, "\"presence\":true") ||
      containsToken(json, "\"presence\": true");
  const bool explicitNoPresence =
      containsToken(json, "\"presence\":false") ||
      containsToken(json, "\"presence\": false");

  if (!presence && !explicitNoPresence) {
    return false;
  }

  decision.presence = presence;
  decision.motion = parseMotion(json);
  decision.direction = parseDirection(json);
  decision.receivedAtMs = nowMs;
  decision.valid = decision.motion != RuViewMotion::Unknown;

  return decision.valid;
}

bool shouldBlockForwardFromRuView(const RuViewDecision &decision, uint32_t nowMs, uint32_t timeoutMs) {
  if (!decision.valid || nowMs - decision.receivedAtMs > timeoutMs) {
    return false;
  }

  const bool motionDetected = decision.motion == RuViewMotion::Low || decision.motion == RuViewMotion::High;

  return decision.presence && motionDetected && decision.direction == RuViewDirection::Front;
}

const char *ruViewMotionName(RuViewMotion motion) {
  switch (motion) {
  case RuViewMotion::None:
    return "none";
  case RuViewMotion::Low:
    return "low";
  case RuViewMotion::High:
    return "high";
  case RuViewMotion::Unknown:
    return "unknown";
  }

  return "unknown";
}

const char *ruViewDirectionName(RuViewDirection direction) {
  switch (direction) {
  case RuViewDirection::Front:
    return "front";
  case RuViewDirection::Left:
    return "left";
  case RuViewDirection::Right:
    return "right";
  case RuViewDirection::Unknown:
    return "unknown";
  }

  return "unknown";
}

