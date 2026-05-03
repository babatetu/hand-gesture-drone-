from __future__ import annotations

from .csi_processor import CsiFrame


def parse_csi_datagram(payload: bytes) -> CsiFrame | None:
    text = payload.decode("utf-8", errors="ignore").strip()

    if text.startswith("GD1HEARTBEAT,"):
        return None

    parts = text.split(",", 8)

    if len(parts) != 9 or parts[0] != "GD1CSI" or parts[1] != "1":
        raise ValueError(f"unsupported CSI datagram: {text[:80]}")

    node_id = parts[2]
    sequence = int(parts[3])
    timestamp_ms = int(parts[4])
    rssi = int(parts[5])
    channel = int(parts[6])
    declared_length = int(parts[7])
    samples = decode_hex_i8(parts[8])

    if declared_length != len(samples):
        samples = samples[:declared_length]

    return CsiFrame(
        node_id=node_id,
        sequence=sequence,
        timestamp_ms=timestamp_ms,
        rssi=rssi,
        channel=channel,
        samples=samples,
    )


def decode_hex_i8(hex_text: str) -> list[int]:
    if len(hex_text) % 2 != 0:
        hex_text = hex_text[:-1]

    samples: list[int] = []

    for index in range(0, len(hex_text), 2):
        raw = int(hex_text[index : index + 2], 16)
        samples.append(raw - 256 if raw >= 128 else raw)

    return samples

