# FR1 Bench Wiring

Use this wiring for Phase 1 glove-side testing with an ESP32 and MPU6050.

## ESP32 to MPU6050

| ESP32 Pin | MPU6050 Pin | Notes |
| --- | --- | --- |
| 3V3 | VCC | Use 3.3 V unless your breakout specifically requires another voltage. |
| GND | GND | Common ground is required. |
| GPIO 21 | SDA | Default ESP32 I2C SDA used by this firmware. |
| GPIO 22 | SCL | Default ESP32 I2C SCL used by this firmware. |
| Not connected | INT | Not used in Phase 1. |

## Bench Safety

- Test Phase 1 away from the drone frame.
- Do not connect ESCs or propellers while validating gestures.
- Keep the glove still during startup calibration.
- Reboot the ESP32 whenever you change the neutral hand position.

