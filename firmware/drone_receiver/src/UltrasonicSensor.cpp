#include "UltrasonicSensor.h"

UltrasonicSensor::UltrasonicSensor(int trigPin, int echoPin, uint32_t intervalMs)
    : trigPin(trigPin), echoPin(echoPin), measureIntervalMs(intervalMs), distanceCm(999.0f), lastTriggerMs(0), echoStartMicros(0), state(State::Idle) {}

void UltrasonicSensor::begin() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);
}

void UltrasonicSensor::update() {
  const uint32_t nowMs = millis();
  const uint32_t nowMicros = micros();

  switch (state) {
  case State::Idle:
    if (nowMs - lastTriggerMs >= measureIntervalMs) {
      lastTriggerMs = nowMs;
      digitalWrite(trigPin, HIGH);
      echoStartMicros = nowMicros;
      state = State::Triggered;
    }
    break;

  case State::Triggered:
    if (nowMicros - echoStartMicros >= 10) { // 10us trigger pulse
      digitalWrite(trigPin, LOW);
      state = State::WaitingEcho;
    }
    break;

  case State::WaitingEcho:
    if (digitalRead(echoPin) == HIGH) {
      echoStartMicros = nowMicros;
      state = State::Measuring;
    } else if (nowMicros - echoStartMicros > 50000) { // Timeout waiting for echo to start
      state = State::Idle;
    }
    break;

  case State::Measuring:
    if (digitalRead(echoPin) == LOW) {
      const uint32_t echoDuration = nowMicros - echoStartMicros;
      // Speed of sound is ~343 m/s, or 29.1 us/cm. Round trip means divide by 2.
      distanceCm = static_cast<float>(echoDuration) / 58.2f;
      state = State::Idle;
    } else if (nowMicros - echoStartMicros > 30000) { // Timeout waiting for echo to end (~5 meters max)
      distanceCm = 999.0f; // out of range
      state = State::Idle;
    }
    break;
  }
}

float UltrasonicSensor::getDistanceCm() const {
  return distanceCm;
}

bool UltrasonicSensor::isObstacleDetected(float thresholdCm) const {
  return distanceCm > 0.0f && distanceCm < thresholdCm;
}
