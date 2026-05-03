#pragma once

#include <Arduino.h>

class UltrasonicSensor {
public:
  UltrasonicSensor(int trigPin, int echoPin);

  void begin();
  void update();

  float getDistanceCm() const;
  bool isObstacleDetected(float thresholdCm) const;

private:
  int trigPin;
  int echoPin;
  float distanceCm;
  uint32_t lastTriggerMs;
  uint32_t echoStartMicros;

  enum class State {
    Idle,
    Triggered,
    WaitingEcho,
    Measuring
  };

  State state;
};
