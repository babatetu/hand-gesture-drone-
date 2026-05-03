from __future__ import annotations

import argparse
import json
from pathlib import Path
import socket
import time
import urllib.request
import urllib.error

from .csi_processor import CsiProcessor, CsiProcessorConfig, CsiDecision
from .protocol import parse_csi_datagram
from .ruview_adapter import RuViewAdapter


def load_config(path: Path) -> dict[str, object]:
    with path.open("r", encoding="utf-8") as config_file:
        return json.load(config_file)


def send_decision(drone_host: str, drone_port: int, decision: CsiDecision) -> None:
    url = f"http://{drone_host}:{drone_port}/ruview"
    payload = json.dumps(decision.to_json_dict(), separators=(",", ":")).encode("utf-8")
    req = urllib.request.Request(url, data=payload, headers={'Content-Type': 'application/json'})
    try:
        with urllib.request.urlopen(req, timeout=0.2) as response:
            pass
        print(payload.decode("utf-8"), flush=True)
    except Exception as exc:
        print(f"WARN: Failed to send to drone: {exc}", flush=True)


def run(config_path: Path) -> None:
    config = load_config(config_path)
    listen_host = str(config.get("listen_host", "0.0.0.0"))
    listen_port = int(config.get("listen_port", 5006))
    drone_host = str(config["drone_host"])
    drone_port = int(config.get("drone_port", 5010))
    emit_interval_seconds = float(config.get("emit_interval_seconds", 0.1))

    processor = CsiProcessor(
        CsiProcessorConfig(
            node_directions=dict(config.get("node_directions", {})),
            presence_energy_threshold=float(config.get("presence_energy_threshold", 18.0)),
            motion_low_threshold=float(config.get("motion_low_threshold", 8.0)),
            motion_high_threshold=float(config.get("motion_high_threshold", 22.0)),
            node_timeout_seconds=float(config.get("node_timeout_seconds", 2.0)),
        )
    )

    ruview = RuViewAdapter(str(config.get("ruview_path", "../../third_party/RuView")))
    print(ruview.describe(), flush=True)
    print("CSI-only mode active. Camera, OpenCV, YOLO, and visual processing are not used.", flush=True)

    receive_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    receive_sock.bind((listen_host, listen_port))
    receive_sock.settimeout(0.2)

    last_emit = 0.0
    latest_decision = CsiDecision(False, "none", "unknown")

    print(f"Listening for CSI on {listen_host}:{listen_port}", flush=True)
    print(f"Sending drone JSON to {drone_host}:{drone_port} via HTTP POST", flush=True)

    while True:
        try:
            payload, _addr = receive_sock.recvfrom(2048)
        except socket.timeout:
            latest_decision = processor.decide()
        else:
            try:
                frame = parse_csi_datagram(payload)
                if frame is not None:
                    latest_decision = processor.update(frame)
            except ValueError as exc:
                print(f"WARN: {exc}", flush=True)

        now = time.monotonic()
        if now - last_emit >= emit_interval_seconds:
            last_emit = now
            send_decision(drone_host, drone_port, latest_decision)


def main() -> None:
    parser = argparse.ArgumentParser(description="GD-1 RuView CSI bridge")
    parser.add_argument(
        "--config",
        type=Path,
        default=Path(__file__).with_name("config.example.json"),
        help="Path to JSON configuration file",
    )
    args = parser.parse_args()
    run(args.config)


if __name__ == "__main__":
    main()

