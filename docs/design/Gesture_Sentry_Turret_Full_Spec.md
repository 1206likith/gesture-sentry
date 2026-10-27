# Gesture-Controlled Sentry Turret — Full Build Spec (TinyML Edition)

A pan-tilt sentry turret that auto-tracks a moving target, but can be
overridden by **real gesture recognition** — an on-device TinyML model
running on the glove's own ESP32 classifies 8 distinct hand motions
(not just tilt-follows-hand) and sends the recognized command over BLE.
No chassis, no drivetrain, no wheels to fight with — and now a genuine
embedded-AI component, which is a real step up from simple sensor-fusion
logic.

**Read this first — the honest risk call**: training 8 gestures to run
reliably on-device in 4 weeks, with a team likely new to ML, is
ambitious. This spec includes a built-in **fallback checkpoint at the
end of week 2** — if the 8-gesture model isn't hitting ~85%+ accuracy by
then, the team drops to a **core set of 5 gestures** (still genuinely
"AI," still impressive, just less training data needed) rather than
risking a broken demo. Both paths use identical hardware and firmware
structure — only the model/dataset size changes. This isn't a failure
mode, it's the plan.

## 1. Full parts list & BOM (~₹4,800 — decked out, under ₹5k)

| Part | Qty | ₹ | Notes | Verified link |
|---|---|---|---|---|
| ESP32 dev board (turret controller) | 1 | 450 | Handles servo control + BLE receive | [Robocraze ESP32 NodeMCU 30-pin (CP2102) — Amazon.in](https://www.amazon.in/dp/B0D6GDV77P) — confirmed, buy qty 2 (one per board) |
| ESP32 dev board (glove controller) | 1 | 450 | Reads IMU, sends over BLE — cheaper/easier than one board doing both | Same link as above, qty 2 total |
| MG996R metal-gear servo (pan) | 1 | 350 | Bigger torque servo for pan axis — carries tilt servo + sensor rig on top, needs the strength. **See bracket-compatibility warning below.** | [Robocraze MG996R TowerPro servo — Amazon.in](https://www.amazon.in/dp/B08F79X9CN) — confirmed match |
| SG90 micro servo (tilt) | 1 | 130 | Lighter axis, SG90 is plenty. **See bracket-compatibility warning below.** | [Tishvi SG90 Micro Servo, 4.8-6V, 180°, pack of 2 — Amazon.in](https://www.amazon.in/dp/B0GM6XHJB5) — confirmed, ₹198 (1-pack, but 2-pack at ₹310 is nearly the same price, worth grabbing a spare) |
| MPU6050 IMU (glove) | 1 | 150 | Wrist tilt → pan/tilt angle mapping | [Robocraze GY-521 MPU-6050 — Amazon.in](https://www.amazon.in/dp/B07H3XH1TB) — confirmed match |
| HC-SR04 ultrasonic (auto-track mode) | 1 | 130 | Front-facing distance/presence sensing for auto mode | [Robocraze HC-SR04, 5V — Amazon.in](https://www.amazon.in/Robocraze-Ultrasonic-Non-Contact-Resolution-Automation/dp/B01I1ZTPJC) — confirmed genuine HC-SR04, standard pinout |
| PIR motion sensor (wake/detect mode) | 1 | 120 | Cheap "something's here" trigger to kick off auto-tracking | [Robocraze HC-SR501 PIR (2-pack) — Amazon.in](https://www.amazon.in/dp/B0D94LDD77) — only 1 needed, extra is a spare |
| Laser diode module (3V, KY-008 style) | 1 | 90 | The "tag" — safe, visible laser dot, NOT a class 3B+/harmful laser. **Must be a KY-008 module (has onboard PCB + current-limiting resistor), not a bare diode head with leads** — a bare 5V diode without a resistor risks damage to the diode or the ESP32 GPIO pin. | [REES52 KY-008 module — Amazon.in](https://www.amazon.in/REES52-KY-008-Transmitter-compatible-arduino/dp/B01I1HHJZW) or [Xcluma KY-008 module — Amazon.in](https://www.amazon.in/xcluma-KY-008-Sensor-Module-Arduino/dp/B074QTP1RC) — both confirmed genuine KY-008 modules (onboard resistor included, works on ESP32's 3.3V logic). Buy only ONE. |
| Small buzzer | 1 | 40 | Audible "target locked" confirmation — free wow-factor | [Universal Hub 5V Passive Buzzer, pack of 2 — Amazon.in](https://www.amazon.in/dp/B0H1WXTSVP) — confirmed match |
| 0.96" OLED display (I2C, SSD1306) | 1 | 250 | Shows mode (AUTO/MANUAL), target distance, "LOCKED" status — this is what makes it feel "decked out" | [Tishvi 0.96" OLED, I2C, 128x64 — Amazon.in](https://www.amazon.in/dp/B0GWL3728B) — confirmed match |
| NeoPixel/WS2812 RGB LED ring (8-12 LED) | 1 | 220 | Status ring around the turret base — green=idle, amber=tracking, red=locked. Big visual upgrade for very little cost | [Xcluma WS2812 12-bit RGB LED ring — Amazon.in](https://www.amazon.in/dp/B0974Y8NMR) — confirmed match |
| 3D-printed or laser-cut pan-tilt bracket kit | 1 | 500 | Buy a cheap generic "pan-tilt camera mount" kit rather than building from scratch — huge time saver. **See bracket-compatibility warning below.** | [Pro3D pan & tilt bracket kit (for MG995/MG996R) — Amazon.in](https://www.amazon.in/dp/B0CV9RSL99) — confirmed match, but see warning |
| Wristband/glove base (cheap sports wristband + Velcro) | 1 | 100 | Mount the glove-side ESP32 + MPU6050. Wide 3" cotton band gives more surface area to strap the board+IMU onto than a thin band would. | [Cotton Wristband, Pair of 2, 3" wide — Amazon.in](https://www.amazon.in/Cotton-Wristband-Men-Women-Pair/dp/B0H6M16CGY) — ₹140 for 2 (only 1 needed, spare included). Fabric is stretchy/elastic, not rigid — still add separate Velcro strips or small cable ties on top to secure the ESP32+MPU6050 stack without it sliding during gesture motion. |
| 18650 battery + holder + charger module (turret power) | 1 | 350 | Untethered turret is much more impressive than one trailing a USB cable. **Needs a boost regulator — see power warning below.** | [Universal Hub 4pcs 3-slot 18650 battery holder — Amazon.in](https://www.amazon.in/dp/B0DNFLDKH4) — confirmed match (holder only, no charger/boost included — see power warning) |
| LiPo 3.7V small pouch cell + TP4056 (glove power) | 1 | 300 | Glove must be wireless/untethered to sell the "gesture control" illusion. **Needs a boost regulator — see power warning below.** | [Universal Hub TP4056 USB-C charging module — Amazon.in](https://www.amazon.in/dp/B0FVLYQBKC) — confirmed match (charging only, no boost regulator included — see power warning) |
| Perfboard, headers, wires, heat-shrink, screws | - | 250 | | [60pc jumper wire pack](https://www.amazon.in/IEIDidacticsTM-Jumper-Female-breadboard-jumper/dp/B099R8S3RZ), [AVS zero-PCB perfboard 15x10cm (2-pack)](https://www.amazon.in/AVS-COMPONENTS-Printed-Circuit-Board/dp/B08KRZLPNL), [AVS female header strip](https://www.amazon.in/AVS-COMPONENTS-Header-Stirp-Female/dp/B08HPHJJ8F) — all 3 confirmed, still need heat-shrink + screws separately |
| Small tripod or base mount for turret | 1 | 200 | Height matters for demo visibility | [POLAM-FOTO mini tripod, 360° ball head — Amazon.in](https://www.amazon.in/dp/B0G4LXV2SD) — confirmed match |
| Enclosure spray paint / stickers ("SENTRY" label, warning stripe) | - | 150 | Cheap cosmetic finishing — disproportionate wow-per-rupee | — |

**Total: ~₹4,780** — right under the ₹5,000 ceiling, no corners cut. No extra hardware is needed for the AI upgrade — TinyML runs entirely on the same ESP32 already in the BOM; the cost is time (data collection + training), not parts.

### ⚠️ Three issues found during BOM verification (2026-09-17) — read before ordering

1. **Pan-tilt bracket/servo size mismatch.** The linked bracket kit above is explicitly sized "for MG995/MG996R" — i.e., built for two same-class large servos. This BOM pairs it with one MG996R (pan) + one much smaller SG90 (tilt). A generic MG996R-sized kit's tilt-axis mounting tabs/screw spacing likely will NOT fit an SG90 without an adapter plate or minor drilling/modification. Before ordering: either (a) find a bracket kit specifically listed as mixed MG996R+SG90 compatible, (b) budget for a simple adapter/shim plate, or (c) switch tilt to a second MG996R (~+₹350, but re-check total top-plate weight against the pan servo's torque rating).
2. **Laser needs to be an actual KY-008 module, not a bare diode.** Confirmed via verification: several similarly-worded Amazon listings for "3V/5V 650nm laser diode" are bare diode heads with leads and NO onboard resistor — wiring one straight to a GPIO risks damaging the diode or the pin. Use one of the two verified KY-008 module links above; both include the resistor on-board.
3. **No voltage/boost regulation specified for either power system.** ESP32 dev boards expect a regulated 5V input (via USB/VIN, regulated onboard to 3.3V) or a clean 3.3V direct feed — NOT a raw battery voltage. A raw 18650 cell outputs 3.7-4.2V and a raw LiPo pouch outputs ~3.7V; neither is a stable 5V, and the TP4056 module is a charge controller only (it does not boost or regulate the output for powering a load). Add a small boost converter module (e.g., MT3608, ~₹30-50) to EACH power path (turret and glove) between the battery and the ESP32's 5V/VIN pin, or confirm your specific 18650/LiPo holder module already includes one before assuming raw battery voltage is safe to feed into the board.

**What makes this "decked out"**: the OLED status display, RGB LED ring, buzzer confirmation, untethered battery power on both turret and glove, cosmetic finishing, **and now real on-device gesture classification instead of simple tilt-mapping**. This is where the budget and effort should go — not a fancier microcontroller.

## 1a. The AI toolchain (what you'll actually use, free/open tools only)

- **Edge Impulse** (free tier, web-based): the standard, well-documented path for "collect IMU data → train a model → export TFLite Micro → run on ESP32." Strongly recommended over building the pipeline from scratch in Arduino IDE — it handles data collection, labeling, training, and C++ export in one flow, and has an official ESP32 tutorial.
- **Model type**: a small 1D-CNN or fully-connected network over a fixed-length window of accelerometer+gyro samples (Edge Impulse's default "Motion Classification" project template does this out of the box — don't design your own architecture, use the template).
- **Data collection**: each gesture needs ~40-60 recorded samples per person doing it (so ~150-250 samples per gesture across the team) to get a model that generalizes beyond one person's exact motion style. This data collection step is the single biggest time cost in the whole project — budget real hours for it in week 1-2, not an afternoon.

## 2. The 8 gestures (core 5 + 3 stretch)

| # | Gesture | Command | Tier |
|---|---|---|---|
| 1 | Tilt left/hold | Pan left (continuous while held) | Core |
| 2 | Tilt right/hold | Pan right (continuous while held) | Core |
| 3 | Sharp forward flick | **LOCK** (laser+buzzer+red ring) | Core |
| 4 | Wave (side-to-side shake) | Reset to idle / auto-track mode | Core |
| 5 | Quick double-tap motion | Switch AUTO ↔ MANUAL mode | Core |
| 6 | Tilt up/hold | Tilt servo up (continuous) | Stretch |
| 7 | Tilt down/hold | Tilt servo down (continuous) | Stretch |
| 8 | Circular wrist rotation | "Scan" — turret does a full sweep | Stretch |

**The core 5 are the fallback set** — genuinely useful on their own (pan left/right, lock, reset, mode-switch covers a full demo). Gestures 6-8 are additive polish, not required for a complete working demo. Treat them as a stretch goal you attempt only after the core 5 are solid.

## 3. 4-way team split

- **Person A — Turret mechanical build**: assemble the pan-tilt bracket, mount both servos + ultrasonic + PIR + laser on top, mount the LED ring around the base, 3D-print or hand-finish the enclosure/paint job, mount on tripod/base. Deliverable: turret physically complete, all sensors/actuators mounted and wired to header pins, moves smoothly through full pan/tilt range by hand.
- **Person B — Turret firmware (the brain)**: ESP32 code that (1) reads ultrasonic + PIR for auto-track mode, (2) receives recognized gesture commands over BLE and maps each to a turret action, (3) drives both servos smoothly, (4) drives the OLED display + RGB ring + buzzer based on current state. Deliverable: turret auto-tracks a moving hand/object in front of it, and correctly executes each incoming gesture-command label with OLED + LED ring reflecting state.
- **Person C — Gesture-AI: data collection, training, and glove firmware**: this is now the project's core technical role. Assembles the wristband (MPU6050 + ESP32 + battery), runs the Edge Impulse data-collection flow to record all 8 gestures across multiple team members, trains and validates the model, exports the TFLite Micro model into the glove's Arduino sketch, and sends the classified gesture label over BLE. Deliverable: glove classifies gestures locally and sends a clean label string over BLE with no laptop involved at demo time.
- **Person D — Integration, mode-switching, dataset support & demo**: helps C with data collection (recording gestures is faster with 2 people — one performing, one operating the recorder), wires the recognized-gesture pipeline into the turret's action-mapping, tests full system range/latency/accuracy, handles battery life testing, writes the demo script, and owns cosmetic finishing. Deliverable: full gesture-to-action pipeline works reliably, system runs untethered for a full 10+ minute demo without dying.

Same parallel-then-converge shape as before, but note: **Person C's role is now the critical path** — B and D can't fully test their integration until a trained model exists. Front-load data collection in week 1, not week 2.

## 4. Control architecture

```
              AUTO MODE                       MANUAL MODE (gesture-AI)
PIR/Ultrasonic --> ESP32(turret) --> servos   Glove IMU --> TFLite Micro model
     |                  |                       (on glove ESP32, on-device)
  detects motion    tracks + centers                    |
                         |                        gesture label
                         |                              |
                    OLED + LED ring + buzzer  --BLE-->  ESP32(turret) --> servo/laser/buzzer action
                    reflect state                            |
                                                    OLED + LED ring + buzzer
                                                    reflect state
```

- **Two states, one clean switch**: `AUTO` (turret scans/tracks on its own using PIR to wake + ultrasonic to estimate direction/distance) and `MANUAL` (glove's on-device model classifies gestures, turret executes the recognized command). Switched by the `MODE_SWITCH` gesture (double-tap motion) itself, so no physical button is strictly needed — though keep a physical button as a manual fallback in case the mode-switch gesture misclassifies during a demo.
- **Gesture recognition, not simple tilt-mapping**: the glove's ESP32 continuously samples the IMU into a fixed-length window (Edge Impulse's default is ~1-2 second windows at 50-100Hz), runs the trained TFLite Micro model on that window, and outputs a gesture label (`PAN_LEFT`, `LOCK`, `RESET`, etc.) only when classification confidence crosses a threshold (e.g. >80%) — this avoids spamming false triggers on random hand movement between intentional gestures. This on-device classification is the actual "AI" component and the main new technical content vs. the tilt-mapping version.
- **The "lock" trigger**: the `LOCK` gesture label (sharp forward flick) fires the laser + buzzer + turns the LED ring red + OLED shows "LOCKED". This is the demo's punchline moment — collect extra training samples for this specific gesture since it needs to be the most reliable one.
- **State feedback (cheap to build, big visual payoff)**:
  - OLED shows: current mode (AUTO/MANUAL), last recognized gesture label + confidence %, live distance reading (auto mode), and a status word (SCANNING / TRACKING / LOCKED).
  - RGB ring: green = idle/scanning, amber = tracking/gesture recognized, red = locked-on.
  - Buzzer: short beep on any recognized gesture, distinct triple-beep on lock.

## 5. Week-by-week timeline (4 weeks)

**Week 1 — Hardware bring-up + data collection starts immediately**
- A: source/assemble pan-tilt bracket kit, mount both servos, confirm full mechanical range of motion by hand (no electronics yet).
- B: get ESP32 talking to both servos via basic test code (sweep pan/tilt through full range under code control); get ultrasonic + PIR reading raw values to serial monitor.
- C: assemble glove (ESP32 + MPU6050 + battery on wristband); set up Edge Impulse project; get raw IMU data streaming into Edge Impulse's data-collection tool. **Begin recording gesture samples for the core 5 gestures immediately** — this cannot wait, it's the longest-lead-time task in the whole project.
- D: order any missing parts immediately (contingency buffer); help C record gesture samples (2 people recording is much faster than 1 — one wears the glove, one operates the laptop).
- *Milestone*: turret moves through full range under code; glove streams IMU data to Edge Impulse; **at least 30 samples per core-5 gesture collected**, from more than one team member.

**Week 2 — Model training + core logic — FALLBACK CHECKPOINT at end of this week**
- A: mount ultrasonic/PIR/laser/LED ring onto the finished bracket in final positions; begin enclosure/cosmetic work.
- B: write auto-track logic (PIR wakes system, ultrasonic-based sweep-and-center logic); wire up OLED + RGB ring + buzzer with placeholder/test states; write the BLE receiver that accepts a gesture-label string and maps it to an action.
- C: finish data collection for core 5 (target 150-250 samples/gesture total across the team), train the first model in Edge Impulse, check the confusion matrix — if two gestures are frequently confused, either collect more contrasting samples or drop one from the demo set. Export to TFLite Micro, flash onto the glove, confirm it prints a recognized label to serial monitor when you perform each gesture.
- D: support C on data collection/labeling QA, then help stress-test the trained model with people who did NOT provide training data (this is the real accuracy test — a model that only recognizes the exact people who trained it isn't validated).
- **CHECKPOINT (end of week 2)**: is the core-5 model hitting ~85%+ accuracy on someone new? **If yes** → proceed to stretch gestures 6-8 in week 3 alongside integration. **If no** → do not attempt gestures 6-8; spend the start of week 3 collecting more core-5 data instead, and simplify the gesture set further if needed (e.g. drop to 3-4 most-distinct gestures). This checkpoint exists specifically so the team makes this call deliberately, not by running out of time in week 4.
- *Milestone*: turret auto-tracks a hand in front of it (bench test); glove correctly classifies core-5 gestures locally and prints labels to serial (BLE transmission not required yet).

**Week 3 — Integration (scope depends on week 2's checkpoint result)**
- D leads: wire glove's classified gesture labels over BLE into the turret's action-mapping (gesture → servo move / lock / mode-switch), get the full manual-mode pipeline working end-to-end.
- A: finish cosmetic pass — paint, labels, cable management, tripod/base mounting, battery placement inside an enclosure (not dangling wires).
- B: polish OLED/LED-ring state machine so all states (idle/scanning/tracking/manual/locked, plus showing the live recognized-gesture label) look clean, no flicker/lag.
- C: **if week 2 checkpoint passed**: collect data for stretch gestures 6-8, retrain/expand the model, re-validate accuracy with new people. **If week 2 checkpoint did not pass**: stay on core-5, spend this time instead tightening confidence thresholds and reducing false-positive triggers between intentional gestures.
- *Milestone*: full system works untethered — auto-track works, gesture-driven manual override works for at least the core 5, mode-switch works, lock-trigger works reliably, all state feedback correct.

**Week 4 — Hardening + demo rehearsal (buffer week, use it fully)**
- Full team: run the complete demo sequence start-to-finish at least 10 times, fix whatever breaks (BLE dropout, misclassified gestures under demo-room lighting/nerves, battery dying mid-demo, PIR false-triggers from the audience's own movement).
- Re-test gesture recognition specifically with whoever will actually perform it live on demo day — accuracy for the live performer matters more than the aggregate validation accuracy.
- Time a full battery-life test on both turret and glove — know exactly how long you can run before recharging, and recharge fully right before the actual demo.
- Write and rehearse a 60-90 second demo script: "auto mode finds and tracks me walking by → I put on the glove → recognized gestures pan/tilt the turret live → I flick to lock and fire the laser." Practice the timing so it looks slick, not fumbling.
- **Fallback plan, layered**: if the on-device model misbehaves live, fall back to demonstrating auto-track mode alone (still a complete, impressive demo). If the full 8-gesture set is shaky, demo only the core 5 — this is not a failure, it's the plan from week 2's checkpoint working as intended. Never let live gesture-AI accuracy be a single point of failure for the whole demo.

## Key risks to flag early (so weeks aren't lost re-discovering them)

1. **Data collection is the real bottleneck, not the model training itself.** Edge Impulse's training step takes minutes; collecting 150-250 clean, correctly-labeled samples per gesture across multiple people takes real hours. Start in week 1, not "when we get to it."
2. **A model trained on one person's motion style may not generalize.** Always validate with someone who did NOT contribute training data before trusting the accuracy number — this is the single most common TinyML mistake student teams make.
3. **BLE range/latency**: test glove-to-turret BLE range and responsiveness in week 2 — if laggy, budget time to optimize or fall back to WiFi/ESP-NOW.
4. **Servo torque under full sensor-rig weight**: MG996R on pan should be plenty, but weigh the fully-loaded top plate early — if too heavy, the servo strains and jitters.
5. **PIR false triggers**: can false-trigger on the demo audience's own movement, not just the intended target. Position/aim it narrowly and test in the actual demo room if possible.
6. **Battery life on both sides**: an untethered demo that dies mid-presentation is worse than a tethered one that works — test real runtime in week 4 and always start the demo on a full charge.
7. **Laser safety**: use only a genuinely low-power visible laser diode (Class 1/2, not anything marketed for "burning" or cutting) and never aim at eyes/faces during testing or demo — a strict rule, not a suggestion.
