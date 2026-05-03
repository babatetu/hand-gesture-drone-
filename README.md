# GD-1 Gesture Drone

GD-1 is a gesture-controlled obstacle-avoiding quadcopter project. This repository starts with Phase 1: the ESP32 glove firmware that reads an MPU6050 IMU, calculates pitch and roll, filters noise, maps tilt into normalized control values, and prints serial debug output.

## Current Build Scope

Phase 1 is bench-only and should be tested with no drone motors or propellers connected.

- Glove ESP32 firmware
- MPU6050 I2C reading
- Startup neutral calibration
- Pitch and roll calculation
- Low-pass filtering and deadband
- Gesture mapping:
  - Forward
  - Backward
  - Left
  - Right
  - Hover
- CSV serial debug output

## Project Layout

```text
firmware/
  lib/
    GD1Protocol/
      src/
        GD1Protocol.h
        GD1Protocol.cpp
  glove/
    platformio.ini
    include/
      GD1Config.h
      GestureProcessor.h
    src/
      GestureProcessor.cpp
      main.cpp
    test/
      test_gesture_processor/
        test_gesture_processor.cpp
  csi_node/
    include/
      CSIConfig.h
    src/
      main.cpp
  drone_receiver/
    include/
      DroneReceiverConfig.h
      RuViewSafety.h
    src/
      RuViewSafety.cpp
      main.cpp
server/
  ruview_csi_bridge/
third_party/
  RuView/
docs/
  phase-plan.md
  ruview-csi-integration.md
  wiring-fr1.md
```

## RuView CSI Perception

RuView WiFi CSI sensing is integrated as a secondary perception layer. It uses an existing WiFi router and ESP32-S3 CSI nodes only. No camera, OpenCV, YOLO, or visual processing is part of the GD-1 pipeline.

See `docs/ruview-csi-integration.md`.

## Build

Install PlatformIO first, then run:

```powershell
cd firmware/glove
pio run
```

This workspace also has PlatformIO installed locally in `.venv`:

```powershell
cd firmware/glove
..\..\.venv\Scripts\platformio.exe run -e glove_esp32
```

Upload to an ESP32:

```powershell
pio run --target upload
```

Open serial monitor:

```powershell
pio device monitor
```

Run pure gesture logic tests:

```powershell
pio test -e native
```

Note: native tests require a local `gcc/g++` toolchain on Windows. The ESP32 firmware build does not require a separately installed native compiler because PlatformIO downloads the ESP32 cross-compiler.

## Serial Output

The glove prints CSV rows:

```text
ms,seq,pitch_deg,roll_deg,pitch_cmd,roll_cmd,packet_ok,gesture
1234,42,8.42,-1.20,28,0,1,FORWARD
```

Command conventions:

- `pitch_cmd` positive means forward.
- `pitch_cmd` negative means backward.
- `roll_cmd` positive means right.
- `roll_cmd` negative means left.
- Values range from `-100` to `100`.

If your physical glove orientation is reversed, change `GD1_PITCH_COMMAND_SIGN` or `GD1_ROLL_COMMAND_SIGN` in `firmware/glove/include/GD1Config.h`.
