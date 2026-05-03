from pathlib import Path


class RuViewAdapter:
    """Small boundary around the local RuView checkout.

    The downloaded RuView repository contains documentation and archived data in
    this workspace, but not a ready importable CSI inference package. This
    adapter keeps the integration point explicit and camera-free so a future
    RuView model can replace the heuristic processor without changing the UDP
    contract to the drone.
    """

    def __init__(self, ruview_path: str) -> None:
        self.root = Path(ruview_path).resolve()

    @property
    def available(self) -> bool:
        return self.root.exists() and (self.root / "README.md").exists()

    def describe(self) -> str:
        if self.available:
            return f"RuView reference available at {self.root}"

        return f"RuView reference not found at {self.root}"

