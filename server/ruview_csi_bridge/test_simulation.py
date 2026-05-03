import urllib.request
import json
import time

def simulate():
    url = "http://localhost:5010/ruview"

    print("Starting RuView simulation... Press Ctrl+C to stop.")

    # Simulate someone walking towards the drone
    states = [
        {"presence": False, "motion": "none", "direction": "unknown"},
        {"presence": True, "motion": "low", "direction": "front"},
        {"presence": True, "motion": "high", "direction": "front"},
        {"presence": True, "motion": "none", "direction": "front"},
    ]

    try:
        idx = 0
        while True:
            state = states[idx % len(states)]
            payload = json.dumps(state).encode("utf-8")

            req = urllib.request.Request(url, data=payload, headers={'Content-Type': 'application/json'})

            try:
                with urllib.request.urlopen(req, timeout=1.0) as response:
                    print(f"Sent {state} -> status {response.getcode()}")
            except urllib.error.URLError as e:
                print(f"Could not connect to {url}: {e}")

            time.sleep(1.0)
            idx += 1

    except KeyboardInterrupt:
        print("\nSimulation stopped.")

if __name__ == "__main__":
    simulate()
