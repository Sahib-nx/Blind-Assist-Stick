# 🦯 Arduino Smart Blind Assist Stick

An Arduino UNO-based assistive device that helps visually impaired individuals detect obstacles in their path using an HC-SR04 ultrasonic sensor. The stick provides real-time audio (buzzer) and visual (LED) feedback with **four proximity zones** — the closer the obstacle, the more urgent the alert.

---

## 📌 Project Overview

This project converts a standard walking stick into an intelligent obstacle detection tool. Rather than a simple on/off alert, the system uses **zone-based proximity detection with hysteresis** to give the user a graduated warning — a slow beep for distant obstacles, a fast beep for near ones, and a continuous alarm for immediate danger. Hysteresis prevents the annoying flickering effect that occurs when a reading sits right on a threshold boundary.

---

## 🧠 How It Works

### The HC-SR04 Ultrasonic Sensor

The **HC-SR04** sends a burst of 40 kHz ultrasonic sound from its **Trig** pin. When the sound bounces off an object, the **Echo** pin goes HIGH for a duration equal to the round-trip travel time. The microcontroller converts that time to a distance:

```
distance (cm) = pulse duration (µs) × 0.034 / 2
```

- **Operating voltage:** 5V  
- **Detection range:** ~2 cm to 400 cm  
- **Accuracy:** ±3 mm  
- **Cone angle:** ~15°

### Averaged Sampling

Instead of using a single reading (which can be noisy), `getDistance()` takes **3 consecutive samples** and returns the average. Each sample has a small 10 ms gap between readings to allow the ultrasonic wave to fully dissipate before the next pulse. If a pulse times out (no echo within 30 ms), it is treated as `999 cm` — effectively "open path."

```
Final Distance = (Sample1 + Sample2 + Sample3) / 3
```

### Zone-Based Proximity Detection

The system divides the space in front of the stick into **4 zones**:

| Zone | Name | Distance | Buzzer | LED |
|------|------|----------|--------|-----|
| 0 | Safe | > 150 cm | Off | Off |
| 1 | Caution | 100 – 150 cm | Slow beep (500 ms interval, 800 Hz) | Slow blink |
| 2 | Warning | 50 – 100 cm | Fast beep (200 ms interval, 1500 Hz) | Fast blink |
| 3 | Danger | < 50 cm | Continuous alarm (2000 Hz) | Solid ON |

### Hysteresis — Preventing Alert Flickering

Without hysteresis, if an object sits right at a zone boundary (e.g., exactly 100 cm), the zone would flicker between Caution and Warning on every reading due to tiny sensor noise. This would cause the buzzer to rapidly switch tones, which is confusing and annoying for the user.

**Solution:** The zone only changes when the reading crosses a threshold by more than `HYSTERESIS = 5 cm` in the exit direction. For example:

- To **enter** Danger (zone 3): distance must drop **below 50 cm**
- To **exit** Danger back to Warning: distance must rise **above 55 cm** (50 + 5)

This creates a small "sticky" buffer around each boundary, making transitions smooth and intentional.

```
          ← Getting closer          Getting farther →

  Safe    Caution      Warning         Danger
  ──────|────────────|───────────────|──────────
       150          100              50       (enter thresholds)
           155          105              55   (exit thresholds)
```

---

## 🔧 Hardware Components

| Component | Purpose |
|---|---|
| Arduino UNO | Main microcontroller |
| HC-SR04 Ultrasonic Sensor | Measures distance to obstacles |
| Passive Buzzer | Audio alert (supports variable frequency tones) |
| LED | Visual alert indicator |
| Wooden Ply | Physical housing for the device |
| Jumper Wires + Mount | Assembly and sensor positioning |

> **Note:** Use a **passive buzzer**, not an active one. Active buzzers only produce a single fixed tone. A passive buzzer accepts frequency control via `tone()`, which is required for the multi-tone zone system.

---

## 📐 Pin Configuration

| Arduino Pin | Component | Role |
|---|---|---|
| 9 | HC-SR04 Trig | Send ultrasonic pulse |
| 10 | HC-SR04 Echo | Receive echo return |
| 5 | Passive Buzzer | Audio alert output |
| 6 | LED | Visual alert output |

---

## 🔊 Alert Behavior by Zone

### Zone 0 — Safe (> 150 cm)
Everything is off. No beep, no LED. The path is clear.

### Zone 1 — Caution (100–150 cm)
Slow intermittent beep at **800 Hz**, toggling every **500 ms**. LED blinks in sync. Indicates an obstacle is present but not immediately threatening. The user has time to adjust direction.

### Zone 2 — Warning (50–100 cm)
Faster intermittent beep at **1500 Hz**, toggling every **200 ms**. LED blinks faster. The obstacle is getting close. The user should slow down and prepare to navigate around it.

### Zone 3 — Danger (< 50 cm)
**Continuous tone at 2000 Hz.** LED stays solid ON. Immediate obstacle — the user must stop or turn right away.

---

## ⚙️ Key Functions

### `getDistance()`
Fires the HC-SR04 three times with a 10 ms gap between each pulse. Uses `pulseIn()` with a 30 ms timeout to avoid blocking if no echo is received. Returns the average of all 3 readings in centimeters.

### `getZone(long dist)`
Takes the latest distance reading and determines which proximity zone applies, using the current zone state to apply hysteresis. The zone only transitions when the distance crosses a threshold by more than `HYSTERESIS` cm in the correct direction. This prevents buzzer flickering at zone boundaries.

### `loop()`
Runs the main control cycle:
1. Gets averaged distance
2. Updates current zone with hysteresis
3. Logs to Serial Monitor for debugging
4. For Zone 0: silences everything
5. For Zone 3: activates continuous alarm
6. For Zones 1 & 2: manages non-blocking timed beeping using `millis()` (no `delay()` in the alert loop , keeps timing accurate)

---

## 🔄 Non-Blocking Beep Timing

The intermittent beeps for zones 1 and 2 are managed using `millis()` rather than `delay()`. This means the Arduino is never "frozen" waiting , it always responds to new distance readings even while managing the beep interval. The pattern:

```cpp
if (now - previousMillis >= beepInterval) {
    previousMillis = now;
    outputState = !outputState;  // toggle on/off
    // apply tone or silence accordingly
}
```

This is critical for a real-time assistive device where responsiveness matters.

---

## 🚀 Getting Started

### Prerequisites
- [Arduino IDE](https://www.arduino.cc/en/software) (1.8.x or 2.x)
- Arduino UNO board
- All hardware listed above

### Upload Steps
1. Clone or download this repository
2. Open `BlindPeopleStick.ino` in Arduino IDE
3. Select **Board:** Arduino UNO
4. Select the correct **Port**
5. Click **Upload**
6. Open **Serial Monitor** at **9600 baud** to view live distance and zone readings , very useful for testing and threshold tuning

---

## 🛠️ Tuning the Thresholds

All key values are defined as constants at the top of the sketch and can be easily adjusted:

```cpp
const int DANGER_DIST   = 50;   // cm — immediate stop threshold
const int WARNING_DIST  = 100;  // cm — fast beep threshold
const int CAUTION_DIST  = 150;  // cm — slow beep threshold
const int HYSTERESIS    = 5;    // cm — boundary buffer to prevent flickering

const int DANGER_FREQ   = 2000; // Hz — continuous alarm tone
const int WARNING_FREQ  = 1500; // Hz — fast beep tone
const int CAUTION_FREQ  = 800;  // Hz — slow beep tone
```

> Adjust `DANGER_DIST`, `WARNING_DIST`, and `CAUTION_DIST` based on the user's walking speed and reaction time. A faster walker may need larger distances. Adjust `HYSTERESIS` if zone flickering is still noticeable on your surface conditions.

---

## 📊 Comparison: Simple vs Zone-Based System

| Feature | Simple On/Off | This Project |
|---|---|---|
| Alert type | Single beep or silence | 4 graduated zones |
| Frequency variation | No | Yes — 800 / 1500 / 2000 Hz |
| Beep pattern | Fixed | Slow → Fast → Continuous |
| Sensor noise handling | None | 3-sample averaging |
| Zone flickering prevention | None | Hysteresis on all boundaries |
| Timing method | `delay()` — blocks CPU | `millis()` — non-blocking |
| Serial debugging | No | Yes |

---

## ⚠️ Known Limitations

- The HC-SR04 has a ~15° detection cone , very thin objects like chair legs or poles may not reflect enough signal to be reliably detected.
- The sensor is directional — it only detects what is directly in front of it. Obstacles to the side or overhead are not detected.
- Very soft or angled surfaces (like thick fabric or diagonal walls) may absorb or deflect the ultrasonic pulse, causing missed readings (returned as 999 cm / safe).
- Battery voltage drop over time can slightly affect sensor accuracy. Use a regulated 5V supply for best results.

---

## 🔮 Possible Future Upgrades

- Add a **vibration motor** for silent haptic feedback (useful in noisy environments)
- Add a **second sensor** pointing downward to detect steps and curbs
- Use a **rechargeable LiPo battery** with USB charging for convenience
- Add a **Bluetooth module** to send distance data to a smartphone app

---

## 📜 License

Open source — free to use, modify, and build upon for educational and assistive technology purposes.

---

## 🙋 Author

Built as part of a Software Engineering hardware project focused on assistive technology.
