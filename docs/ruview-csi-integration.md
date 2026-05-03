# GD-1 RuView CSI Integration

This integration uses WiFi CSI as a secondary perception layer for GD-1.

Hard rule: no camera, OpenCV, YOLO, webcam, optical flow, or visual processing is used in this pipeline.

## Architecture

```text
WiFi Router
  |
  | radio signal field
  v
ESP32-S3 CSI Nodes: front / left / right
  |
  | UDP: GD1CSI frames
  v
Processing Server: server/ruview_csi_bridge
  |
  | UDP JSON
  v
Drone ESP32 Receiver
  |
  | secondary safety override
  v
Flight-controller command path
```

Ultrasonic sensors remain the primary immediate obstacle avoidance layer. RuView CSI only adds a secondary forward-motion block when it detects front-side presence/motion.

## Important Hardware Note

RuView's own README says original ESP32 and ESP32-C3 are not supported for advanced CSI DSP. Keep the current drone control ESP32, but use ESP32-S3 boards as CSI sensing nodes.

Recommended minimum:

- 1 existing WiFi router
- 1 processing server on the same WiFi network
- 1 drone ESP32 receiver
- 3 ESP32-S3 CSI nodes labeled `front`, `left`, and `right`

## RuView Setup

The RuView source is placed at:

```text
third_party/RuView
```

This machine did not have `git`, so the GitHub archive was downloaded and expanded locally. If you have Git installed, the equivalent command is:

```powershell
git clone https://github.com/ruvnet/RuView third_party/RuView
```

Do not run RuView demos or adapters that request webcam/camera input. This GD-1 integration uses only CSI UDP frames.

## CSI Node Setup

Firmware:

```text
firmware/csi_node
```

Edit:

```text
firmware/csi_node/include/CSIConfig.h
```

Set:

- `GD1_WIFI_SSID`
- `GD1_WIFI_PASSWORD`
- `GD1_CSI_NODE_ID`: `front`, `left`, or `right`
- `GD1_CSI_SERVER_IP`: IP address of the processing server
- `GD1_CSI_SERVER_PORT`: default `5006`
- `GD1_WIFI_CHANNEL`: router channel

Build:

```powershell
cd firmware/csi_node
..\..\.venv\Scripts\platformio.exe run -e csi_node_esp32s3
```

Upload:

```powershell
..\..\.venv\Scripts\platformio.exe run -e csi_node_esp32s3 --target upload
```

Each node streams UDP frames like:

```text
GD1CSI,1,front,42,123456,-48,1,384,FF00A10B...
```

## Processing Server Setup

Server:

```text
server/ruview_csi_bridge
```

Create your config from:

```text
server/ruview_csi_bridge/config.example.json
```

Set:

- `drone_host`: IP address of the drone ESP32 receiver
- `drone_port`: default `5010`
- `node_directions`: map CSI node IDs to physical directions
- thresholds after bench testing

Run:

```powershell
python -m server.ruview_csi_bridge.server --config server/ruview_csi_bridge/config.example.json
```

Output sent to the drone ESP32 is exactly:

```json
{"presence":true,"motion":"low","direction":"front"}
```

Valid values:

- `presence`: `true` or `false`
- `motion`: `none`, `low`, or `high`
- `direction`: `front`, `left`, `right`, or `unknown`

## Drone Receiver Setup

Firmware:

```text
firmware/drone_receiver
```

Edit:

```text
firmware/drone_receiver/include/DroneReceiverConfig.h
```

Set:

- `GD1_DRONE_WIFI_SSID`
- `GD1_DRONE_WIFI_PASSWORD`
- `GD1_RUVIEW_LISTEN_PORT`: default `5010`

Build:

```powershell
cd firmware/drone_receiver
..\..\.venv\Scripts\platformio.exe run -e drone_receiver_esp32
```

Upload:

```powershell
..\..\.venv\Scripts\platformio.exe run -e drone_receiver_esp32 --target upload
```

Current integration behavior:

- If ultrasonic blocks forward movement, pitch command is forced to `0`.
- Else if RuView JSON says `presence=true`, `motion=low|high`, and `direction=front`, positive pitch command is forced to `0`.
- RuView stale data times out after `GD1_RUVIEW_TIMEOUT_MS`.

## Bench Test Order

1. Start the processing server.
2. Flash one ESP32-S3 node as `front`.
3. Confirm server prints JSON with `presence`, `motion`, and `direction`.
4. Flash the drone receiver ESP32.
5. Confirm receiver serial logs show RuView state.
6. Move in the front sensing zone and confirm `safe_pitch=0`.
7. Add `left` and `right` nodes after front behavior is stable.
8. Integrate with gesture receiver only after bench tests pass.

## Tuning

Start with conservative thresholds:

- Increase `motion_low_threshold` if false positives block forward movement.
- Decrease `motion_low_threshold` if motion is missed.
- Use `motion_high_threshold` for stronger emergency-style front blocking.
- Keep `node_timeout_seconds` near `2.0` so stale CSI data does not keep blocking commands.

## Safety Notes

- Do not rely on CSI for high-speed collision avoidance.
- Keep ultrasonic sensors as the primary safety system.
- Treat RuView direction as approximate.
- Test without propellers until the complete command path is proven.
- CSI sensing depends on router placement, channel traffic, node placement, and RF reflections.

