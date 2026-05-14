// --- Pin Definitions ---
const int TRIG_PIN  = 9;
const int ECHO_PIN  = 10;
const int BUZZER    = 5;
const int LED       = 6;

// --- Distance Thresholds (cm) ---
const int DANGER_DIST  = 50;
const int WARNING_DIST = 100;
const int CAUTION_DIST = 150;

// --- Hysteresis buffer (prevents flickering at zone edges) ---
const int HYSTERESIS = 5;

// --- Beep frequencies (Hz) ---
const int DANGER_FREQ  = 2000;
const int WARNING_FREQ = 1500;
const int CAUTION_FREQ = 800;

// --- State tracking ---
unsigned long previousMillis = 0;
bool          outputState     = false;
int           currentZone     = 0; // 0=safe, 1=caution, 2=warning, 3=danger

// ============================================================
//  Get averaged distance over N readings
// ============================================================
long getDistance() {
  const int SAMPLES = 3;
  long total = 0;

  for (int i = 0; i < SAMPLES; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    total += (duration == 0) ? 999 : (duration * 0.034 / 2);

    delay(10); // small gap between samples
  }

  return total / SAMPLES;
}

// ============================================================
//  Determine zone with hysteresis
//  Zone only changes if reading crosses threshold by HYSTERESIS
// ============================================================
int getZone(long dist) {
  // Use slightly expanded thresholds to exit a zone (hysteresis)
  if (currentZone == 3) { // currently danger
    if (dist > DANGER_DIST + HYSTERESIS)  return 2;
    return 3;
  }
  if (currentZone == 2) { // currently warning
    if (dist > WARNING_DIST + HYSTERESIS) return 1;
    if (dist < DANGER_DIST  - HYSTERESIS) return 3;
    return 2;
  }
  if (currentZone == 1) { // currently caution
    if (dist > CAUTION_DIST + HYSTERESIS) return 0;
    if (dist < WARNING_DIST - HYSTERESIS) return 2;
    return 1;
  }
  // currently safe (zone 0)
  if (dist < CAUTION_DIST) return 1;
  return 0;
}

// ============================================================
void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER,   OUTPUT);
  pinMode(LED,      OUTPUT);
  Serial.begin(9600); // optional: for debugging in Serial Monitor
}

// ============================================================
void loop() {
  long dist = getDistance();
  currentZone = getZone(dist);

  Serial.print("Distance: ");
  Serial.print(dist);
  Serial.print(" cm | Zone: ");
  Serial.println(currentZone);

  unsigned long now = millis();

  // --- Zone 0: Safe — everything off ---
  if (currentZone == 0) {
    noTone(BUZZER);
    digitalWrite(LED, LOW);
    outputState = false;
    previousMillis = now; // reset timer cleanly
    return;
  }

  // --- Zone 3: Danger — continuous alarm ---
  if (currentZone == 3) {
    tone(BUZZER, DANGER_FREQ);
    digitalWrite(LED, HIGH);
    outputState = true;
    previousMillis = now; // keep timer reset while in danger
    return;
  }

  // --- Zone 1 & 2: Intermittent beep ---
  int beepInterval = (currentZone == 2) ? 200 : 500;
  int freq         = (currentZone == 2) ? WARNING_FREQ : CAUTION_FREQ;

  if (now - previousMillis >= beepInterval) {
    previousMillis = now;
    outputState = !outputState;

    if (outputState) {
      tone(BUZZER, freq);
      digitalWrite(LED, HIGH);
    } else {
      noTone(BUZZER);
      digitalWrite(LED, LOW);
    }
  }
}