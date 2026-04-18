# AcouScan × Arduino Uno — Ultrasonic BCG Add-On

A weekend integration that adds an **independent heart-rate channel** to AcouScan using an HC-SR04 ultrasonic rangefinder on an Arduino Uno R3. Measures chest-wall displacement (ballistocardiogram-style) at ~50 Hz and streams it to the browser over USB serial.

## What this is (and isn't)

- **Is:** a second, physically-independent HR estimator that cross-checks the acoustic reading. Useful when the piezo/earbud signal is noisy.
- **Isn't:** a phonocardiogram. The HC-SR04 cannot capture S1/S2 sounds — those need the acoustic pickup (already in AcouScan).

## Hardware

| Part       | Qty | Notes                            |
|------------|-----|----------------------------------|
| Arduino Uno R3 | 1   | Any ATmega328P clone works   |
| HC-SR04        | 1   | The standard 4-pin module    |
| Jumper wires   | 4   | F/F or F/M depending on setup |
| USB-B cable    | 1   | Uno ↔ computer               |

### Wiring

```
HC-SR04        Uno R3
──────         ──────
VCC      →     5V
GND      →     GND
TRIG     →     D9
ECHO     →     D10
```

That's it. No resistors, no breadboard required — HC-SR04 is 5V-native so it mates directly with the Uno.

## Installation

### 1. Upload the sketch

Open `acouscan_bcg_uno.ino` in Arduino IDE → select **Board: Arduino Uno** → select the correct **Port** → **Upload**.

Verify it works: open Serial Monitor at **115200 baud**. You should see lines like:

```
# AcouScan BCG streamer — 50 Hz, mm,ms
47.23,143
47.19,163
47.31,183
...
```

**Close the Serial Monitor before the next step** — only one app can hold the serial port at a time.

### 2. Connect from AcouScan

1. Open AcouScan in **Chrome or Edge on desktop** (Web Serial is not in Safari or iOS yet).
2. Scroll to the new **"Ultrasonic BCG · Arduino"** panel.
3. Click **Connect Arduino (Web Serial)** → pick the Uno from the picker.

You should see the status flip to "Connected / Streaming," the waveform should start drawing, and after ~5 seconds of good signal the HR metric will populate.

## How to position the sensor

Geometry matters more than you'd expect at 3 mm precision.

- **Distance:** 3–5 cm from bare skin. Closer than ~2 cm causes invalid/saturated readings; farther than ~8 cm loses the <1 mm heartbeat displacement in noise.
- **Aim:** left parasternal area (same spot as acoustic pickup).
- **Bracing:** hand tremor at 3–8 Hz will dominate the signal if you freehand it. Use one of:
  - A small tripod or gooseneck clamp holding the breakout board.
  - A chest strap with the board fixed to a rigid plate.
  - Resting your forearm against a table while holding the board.
- **Breath hold:** for the cleanest reading, hold your breath for 10–15 seconds after starting. Breathing motion (5–10 mm) dwarfs the heartbeat (<1 mm), and while the 0.7 Hz high-pass filter rejects most of it, a still chest gives obviously cleaner traces.

## How it works — signal chain

```
HC-SR04 ping (50 Hz)
      │
      ▼
Median of 3 pings (reject spurious echoes)
      │
      ▼
Serial: "distance_mm,millis\n"  @ 115200 baud
      │
      ▼ ─── USB ─── Browser (Web Serial API)
      │
      ▼
HPF 0.7 Hz (IIR, single-pole)  ← kills breathing + DC drift
      │
      ▼
LPF 3.0 Hz (IIR, single-pole)  ← smooths HC-SR04 quantization
      │
      ▼
Adaptive threshold = 1.5 × running RMS (1 s window)
      │
      ▼
Peak detect + 350 ms refractory (max 170 bpm ceiling)
      │
      ▼
Median RR of last 8 beats → HR in bpm
```

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| "Web Serial not supported" | Using Safari / Firefox / iOS | Use Chrome or Edge on desktop |
| "Could not open serial port" | Arduino IDE Serial Monitor still open | Close it, try again |
| Waveform is flat | Sensor pointing at air, or too close | Reposition 3–5 cm from chest |
| HR reads 120–170 (implausibly high) | Picking up tremor harmonics | Brace the sensor, retry |
| HR fluctuates wildly | Breathing motion dominating | Hold breath for 10 s test |
| Sample rate shows <40 Hz | USB latency / slow echo returns | Check wiring; shorter wires help |

## Limits of this approach

Being honest about the ceiling: HC-SR04's ~3 mm timing resolution means you're seeing heartbeat displacement *near the sensor's noise floor*. Expect:

- HR accuracy: **±5 bpm** at rest with good bracing. Not clinical grade.
- Works only on bare skin or very thin fabric. Winter jacket → nothing.
- Won't resolve anything more subtle than HR (no HRV, no morphology).

For better BCG, the upgrade path is an **mmWave radar** (e.g. Infineon BGT60 or Acconeer A121, both ~$30 breakouts) — same serial protocol would work, swap only the Arduino sketch.

## Files

- `index.html` — AcouScan app with Ultrasonic BCG panel integrated
- `acouscan_bcg_uno.ino` — Arduino sketch for the Uno R3
- `README.md` — this file
