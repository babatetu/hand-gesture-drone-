# GD-1 RuView Integration Setup Instructions

This document provides step-by-step instructions for setting up the RuView CSI integration with the GD-1 Drone project.

## Hardware Requirements
1. **ESP32 Drone Receiver**: 1x standard ESP32 for the drone receiver.
2. **ESP32-S3 CSI Nodes**: 3x ESP32-S3 boards (labeled `front`, `left`, `right`). Note: standard ESP32/ESP32-C3 are not supported for CSI DSP.
3. **WiFi Router**: A dedicated router to provide a stable WiFi network.
4. **Server**: A PC or Raspberry Pi to run the RuView processing server.

## Hardware Wiring
### CSI Nodes
- No special wiring required. Power the ESP32-S3 via USB. Ensure they are positioned appropriately in the room (e.g., in front of, to the left, and to the right of the drone's expected flight path).

### Drone Receiver
- Power the ESP32 via USB or the drone's battery via a BEC.
- Make sure to connect standard drone components (like ultrasonic sensors) to the appropriate GPIOs (refer to Phase 1/2 instructions).

## Software Installation
1. **Clone the Repository**:
   ```bash
   git clone <your-repo-url>
   cd gd1-drone
   ```

2. **Set up PlatformIO**:
   Ensure you have PlatformIO installed. If not, you can install it via a python virtual environment:
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   pip install platformio
   ```

3. **Configure WiFi**:
   Update `GD1_WIFI_SSID` and `GD1_WIFI_PASSWORD` in the following files:
   - `firmware/csi_node/include/CSIConfig.h`
   - `firmware/drone_receiver/include/DroneReceiverConfig.h`

4. **Flash the CSI Nodes**:
   Connect each ESP32-S3 one by one. In `firmware/csi_node/include/CSIConfig.h`, change `GD1_CSI_NODE_ID` to `"front"`, `"left"`, or `"right"` accordingly.
   ```bash
   cd firmware/csi_node
   pio run -e csi_node_esp32s3 --target upload
   ```

5. **Flash the Drone Receiver**:
   Connect the ESP32 for the drone.
   ```bash
   cd firmware/drone_receiver
   pio run -e drone_receiver_esp32 --target upload
   ```

## Running the Server
1. Navigate to the project root.
2. Ensure you have python3 installed.
3. Copy the example configuration:
   ```bash
   cp server/ruview_csi_bridge/config.example.json server/ruview_csi_bridge/config.json
   ```
4. Run the server:
   ```bash
   python -m server.ruview_csi_bridge.server --config server/ruview_csi_bridge/config.json
   ```

## End-to-End Test (Simulation)
If you do not have all the hardware set up yet, you can test the communication between the server and the drone receiver using the included simulation script.

1. Ensure the Drone Receiver is flashed, powered on, and connected to your WiFi router. Let's assume its IP is `192.168.1.80`.
2. Update the `url` in `server/ruview_csi_bridge/test_simulation.py` to point to the drone receiver's IP: `http://192.168.1.80:5010/ruview`.
3. Run the simulation script:
   ```bash
   python server/ruview_csi_bridge/test_simulation.py
   ```
4. Check the serial monitor of the Drone Receiver:
   ```bash
   cd firmware/drone_receiver
   pio device monitor
   ```
   You should see debug logs reflecting the simulated JSON data changing the safe pitch command over time.
