from __future__ import annotations

from dataclasses import dataclass, field
import math
import time


@dataclass
class CsiFrame:
    node_id: str
    sequence: int
    timestamp_ms: int
    rssi: int
    channel: int
    samples: list[int]


@dataclass
class NodeState:
    node_id: str
    direction: str
    last_seen: float = 0.0
    baseline_energy: float | None = None
    energy: float = 0.0
    motion_score: float = 0.0
    rssi: int = -127
    frame_count: int = 0


@dataclass
class CsiDecision:
    presence: bool
    motion: str
    direction: str

    def to_json_dict(self) -> dict[str, object]:
        return {
            "presence": self.presence,
            "motion": self.motion,
            "direction": self.direction,
        }


@dataclass
class CsiProcessorConfig:
    node_directions: dict[str, str]
    presence_energy_threshold: float = 18.0
    motion_low_threshold: float = 8.0
    motion_high_threshold: float = 22.0
    node_timeout_seconds: float = 2.0


@dataclass
class CsiProcessor:
    config: CsiProcessorConfig
    nodes: dict[str, NodeState] = field(default_factory=dict)

    def update(self, frame: CsiFrame) -> CsiDecision:
        now = time.monotonic()
        direction = self.config.node_directions.get(frame.node_id, "unknown")
        state = self.nodes.setdefault(frame.node_id, NodeState(frame.node_id, direction))

        energy = self._energy(frame.samples)

        if state.baseline_energy is None:
            state.baseline_energy = energy

        delta = abs(energy - state.baseline_energy)
        state.baseline_energy = (0.98 * state.baseline_energy) + (0.02 * energy)
        state.energy = energy
        state.motion_score = (0.75 * state.motion_score) + (0.25 * delta)
        state.rssi = frame.rssi
        state.last_seen = now
        state.frame_count += 1

        return self.decide()

    def decide(self) -> CsiDecision:
        now = time.monotonic()
        active_nodes = [
            node
            for node in self.nodes.values()
            if now - node.last_seen <= self.config.node_timeout_seconds
        ]

        if not active_nodes:
            return CsiDecision(False, "none", "unknown")

        strongest = max(active_nodes, key=lambda node: node.motion_score)
        max_motion = strongest.motion_score
        max_energy_delta = max(
            abs(node.energy - (node.baseline_energy or node.energy))
            for node in active_nodes
        )

        presence = (
            max_motion >= self.config.motion_low_threshold
            or max_energy_delta >= self.config.presence_energy_threshold
        )

        if max_motion >= self.config.motion_high_threshold:
            motion = "high"
        elif max_motion >= self.config.motion_low_threshold:
            motion = "low"
        else:
            motion = "none"

        direction = strongest.direction if presence else "unknown"

        return CsiDecision(presence, motion, direction)

    @staticmethod
    def _energy(samples: list[int]) -> float:
        if not samples:
            return 0.0

        mean_square = sum(sample * sample for sample in samples) / len(samples)
        return math.sqrt(mean_square)

