# Voice-Controlled Robotic Arm — Full Build Spec

## 1. Parts list & BOM (~₹3,500-4,500 total)

| Part | Qty | Approx cost (₹) | Notes |
|---|---|---|---|
| MG995/SG90 servo (4x arm joints: base, shoulder, elbow, wrist) | 3x MG995 + 1x SG90 | 900 | MG995 for base/shoulder/elbow (torque), SG90 for wrist/gripper |
| SG90 micro servo (gripper) | 1 | 120 | Claw open/close |
| Acrylic/3D-printed arm frame kit | 1 | 600-900 | Buy a "4-DOF robotic arm kit" — cheaper than fabricating |
| ESP32 dev board | 1 | 450 | WiFi/BLE for phone-app control |
| PCA9685 16-ch PWM servo driver | 1 | 250 | Do NOT drive 4 servos off ESP32 GPIO directly — current/timing issues |
| 5V 4A external power supply (for servos) | 1 | 300 | Servos draw too much for ESP32's onboard 3.3V rail |
| Jumper wires + breadboard/perfboard | - | 150 | |
| Mounting base/clamp | 1 | 150 | Stabilizes arm during pick-place |
| Small test objects (foam cubes, bottle caps) | - | 0 | For pick-and-place demo |

**Total: ~₹2,900-3,700** plus contingency → budget ₹4,000-4,500.

## 2. 4-way team split

- **Person A — Arm mechanical build**: assemble frame, mount all 4 servos, attach gripper, tune horn positions so each joint's mechanical range matches its expected software angle range (0-180°). Deliverable: arm physically moves through full range by hand with no binding.
- **Person B — Servo control firmware**: wire PCA9685 to ESP32 (I2C), write firmware functions `moveJoint(id, angle)` and `moveSequence(steps[])`, calibrate min/max pulse widths per servo. Deliverable: can command any single joint to any angle from serial monitor.
- **Person C — App + voice/command interface**: build simple phone app (or web page) with buttons for discrete commands (`HOME`, `PICK`, `PLACE`, `UP`, `DOWN`, `OPEN`, `CLOSE`) and optionally wire in phone's built-in speech-to-text to map spoken words to the same command set. Sends commands over WiFi (HTTP POST or WebSocket) to ESP32. Deliverable: app can send all 7 commands and ESP32 receives them (visible in serial monitor).
- **Person D — Integration, sequencing & demo**: combine B's joint control + C's command reception into full pick-and-place sequences (a `PICK` command = a scripted list of joint moves), test the full loop end-to-end, handle power/mounting for demo day, write the 1-page demo script. Deliverable: full "pick object from point X, place at point Y" cycle works reliably 5/5 times.

Work is parallel for weeks 1-2 (A/B/C independent), converges in week 3 (D integrates).

## 3. Software / control architecture

```
Phone (voice or button) --HTTP/WebSocket--> ESP32 --I2C--> PCA9685 --PWM--> 4 servos
```

- **Command set** (keep it small — 7 discrete commands, not continuous voice control): `HOME`, `PICK`, `PLACE`, `OPEN`, `CLOSE`, `UP`, `DOWN`.
- **Voice mapping**: use the phone's native speech-to-text (Android/iOS built-in, or a simple web page's `SpeechRecognition` API) to convert speech → text → match against the 7 known command words → send as an HTTP GET/POST to the ESP32's IP (e.g. `GET /cmd?action=pick`). Do **not** attempt on-device speech recognition on the ESP32 — too complex for the timeline, and the phone already does this for free.
- **Arm control — no inverse kinematics needed**: pre-record each named position (`HOME`, `PICK_POS`, `PLACE_POS`) as a fixed 4-angle tuple `{base, shoulder, elbow, wrist}` during calibration (Person B/D jog the arm manually via serial commands, note the angles that look right, hard-code them). A `PICK` command = move to `PICK_POS` → close gripper → move to `HOME`. A `PLACE` command = move to `PLACE_POS` → open gripper → move to `HOME`. This avoids any real kinematics math.
- **Motion smoothing**: move each servo in small angle increments (e.g. 2° steps with a short delay) rather than jumping instantly — prevents jerky motion and reduces current spikes.

## 4. Week-by-week timeline

- **Week 1**: A assembles frame + mounts servos. B wires PCA9685 + writes single-joint test code. C builds app UI + gets HTTP request reaching a test ESP32 endpoint. *Milestone: arm physically complete; ESP32 can move one servo on command; app can send one HTTP request.*
- **Week 2**: A finishes gripper + full range-of-motion tuning. B writes multi-joint `moveSequence()` + calibrates all 4 servos' angle ranges. C wires up the 7-command set + adds speech-to-text mapping. *Milestone: all 4 joints individually controllable; app sends all 7 commands; ESP32 logs each received command correctly.*
- **Week 3**: D integrates — defines `PICK_POS`/`PLACE_POS`/`HOME` angle tuples by manually jogging the real arm, wires command→sequence logic, runs full pick-and-place loop repeatedly, fixes timing/power issues. *Milestone: full pick-and-place cycle works 5/5 times via voice command.*
- **Week 4 (buffer/demo prep)**: polish app UI, rehearse demo script, fix any object-slippage/gripper-grip issues, prepare backup plan (manual button fallback if voice recognition misfires in a noisy demo room).

## Key risk to flag early
Voice recognition in a noisy demo hall is unreliable — **always keep the button-based app UI as the primary demo path**, with voice as a bonus/backup showing the concept works. Don't let demo-day success depend on speech-to-text accuracy.
