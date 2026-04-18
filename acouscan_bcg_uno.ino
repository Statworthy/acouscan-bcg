/*
 * AcouScan BCG — Uno R3 + HC-SR04 chest-motion streamer
 * ─────────────────────────────────────────────────────
 * Streams distance samples over USB serial for the AcouScan
 * browser app. Line format: "<distance_mm>,<millis>\n"
 *
 * WIRING
 *   HC-SR04   Uno R3
 *   ──────    ──────
 *   VCC   →   5V
 *   GND   →   GND
 *   TRIG  →   D9
 *   ECHO  →   D10
 *
 * PHYSICAL SETUP
 *   - Hold sensor 3–5 cm from the skin of the chest, aimed at the
 *     left parasternal area (same spot you'd use for AcouScan acoustic).
 *   - Keep the sensor mechanically steady — hand tremor dominates
 *     the signal if you're not braced against something.
 *   - Best results with a rigid mount (a small tripod, a clip, or a
 *     chest strap holding the breakout board).
 *
 * USAGE
 *   1. Upload this sketch from Arduino IDE.
 *   2. CLOSE the Arduino Serial Monitor (it holds the port).
 *   3. In Chrome/Edge desktop, open AcouScan.
 *   4. Click "Connect Arduino (Web Serial)" and select the Uno.
 *
 * NOTES
 *   - Samples at 50 Hz (20 ms period). HC-SR04 max timeout caps us
 *     around 55 Hz; 50 Hz leaves margin.
 *   - Each reading is a 3-pulse median to knock out single spurious
 *     echoes from cloth folds / skin reflections.
 *   - Distance is reported in mm as a float. Browser handles filtering.
 */

const uint8_t TRIG_PIN = 9;
const uint8_t ECHO_PIN = 10;

// HC-SR04 echo timeout: ~23 ms covers ~4 m, plenty for chest work
const unsigned long ECHO_TIMEOUT_US = 25000UL;

// Target sample period (ms). 20 ms = 50 Hz.
const unsigned long SAMPLE_PERIOD_MS = 20;

// Speed of sound in air @ ~20 °C, mm per µs (round trip factor applied below)
const float MM_PER_US = 0.343f;

unsigned long nextSampleAt = 0;

// ── Single echo measurement, returns distance in mm, or -1.0 on timeout ──
float singlePing() {
  // Clear trigger line
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  // 10 µs trigger pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure echo pulse width (µs). 0 means timeout.
  unsigned long dur_us = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (dur_us == 0) return -1.0f;

  // Round trip: distance = (dur_us * speed_of_sound) / 2
  return (float)dur_us * MM_PER_US * 0.5f;
}

// ── Median of 3 pings — rejects one-off bad echoes ──
float medianPing() {
  float a = singlePing();
  delayMicroseconds(500);   // let echoes settle
  float b = singlePing();
  delayMicroseconds(500);
  float c = singlePing();

  // If any timed out, return best of the rest
  if (a < 0 && b < 0 && c < 0) return -1.0f;
  if (a < 0) a = (b < 0) ? c : b;
  if (b < 0) b = (a < 0) ? c : a;
  if (c < 0) c = (a < 0) ? b : a;

  // 3-element median
  if (a > b) { float t = a; a = b; b = t; }
  if (b > c) { float t = b; b = c; c = t; }
  if (a > b) { float t = a; a = b; b = t; }
  return b;
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  Serial.begin(115200);
  // Startup banner — AcouScan parser ignores non-numeric lines
  Serial.println(F("# AcouScan BCG streamer — 50 Hz, mm,ms"));
  nextSampleAt = millis();
}

void loop() {
  unsigned long now = millis();
  if ((long)(now - nextSampleAt) < 0) return;
  nextSampleAt += SAMPLE_PERIOD_MS;

  float mm = medianPing();
  if (mm > 10.0f && mm < 1000.0f) {   // sanity: 1cm–1m
    // Two decimal places gives ~0.01 mm precision on paper
    // (real HC-SR04 precision is ~3 mm, but the decimals help the filter).
    Serial.print(mm, 2);
    Serial.print(',');
    Serial.println(now);
  }
  // If out of range, emit nothing — browser just skips that time slot.
}
