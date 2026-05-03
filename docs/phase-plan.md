# GD-1 Development Phase Plan

## Phase 1: Gesture Module

Status: started

Goal: prove that the glove can produce stable, repeatable command values from hand tilt.

Deliverables:

- ESP32 reads MPU6050 over I2C.
- Firmware calculates pitch and roll.
- Firmware calibrates neutral position on startup.
- Gesture processor applies filtering, deadband, and normalized output.
- Serial debug output shows the current gesture and command values.

Bench acceptance:

- Neutral hand position prints `HOVER`.
- Forward tilt prints positive `pitch_cmd`.
- Backward tilt prints negative `pitch_cmd`.
- Right tilt prints positive `roll_cmd`.
- Left tilt prints negative `roll_cmd`.
- Values return close to zero when the hand returns to neutral.

## Phase 2: Communication

Status: started

Goal: transmit gesture commands from glove ESP32 to drone ESP32 with packet integrity.

Current implementation:

- Shared `GD1Protocol` library.
- Fixed-size 15-byte command packet.
- Magic byte and protocol version checks.
- Sequence number and timestamp.
- Signed pitch, roll, throttle, and yaw commands.
- Flags for calibrated, failsafe, and emergency-stop states.
- CRC-16/CCITT packet validation.

Candidate radios:

- LoRa SX1278 for longer range and lower data rate.
- NRF24L01 for lower latency and shorter practical range.

Planned packet fields:

- Magic byte
- Protocol version
- Sequence number
- Timestamp
- Pitch command
- Roll command
- Throttle command
- Yaw command
- Flags
- CRC

## Phase 3: Drone Integration

Status: planned

Goal: convert receiver commands into flight-controller input while keeping the flight controller responsible for stabilization.

Initial integration target:

- Pixhawk/ArduPilot RC override or SBUS/PPM bridge after bench validation.

Safety rule:

- Do not command motors directly from the ESP32.

## Phase 4: Obstacle Avoidance

Status: planned

Goal: add front, left, and right distance sensing and override unsafe movement.

Initial logic:

- If front distance is below threshold, block positive pitch command.
- If left distance is below threshold, block negative roll command.
- If right distance is below threshold, block positive roll command.

## Phase 5: Optimization

Status: planned

Goal: improve control smoothness, latency, and reliability.

Likely work:

- Tune deadband and max tilt.
- Tune low-pass alpha.
- Add complementary filter if gyro drift and accelerometer noise require it.
- Add packet loss handling and failsafe behavior.
