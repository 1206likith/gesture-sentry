# 20 Real Hardware+Software Demo Project Ideas — 4-Person Team, Few Weeks

Not BACSE324-paper-scoped. These are straightforward, real, working
hardware demos — roughly 50% hardware (wiring, sensors, actuators,
enclosure) and 50% software (firmware + a simple app/dashboard), sized so
a 4-person team can split the work simply and finish in a few weeks.

## How the split works (same simple pattern for every idea below)

For almost every idea here, the natural 4-way split is:

1. **Person A — Sensors & wiring**: gets every sensor reading correctly onto the microcontroller.
2. **Person B — Actuators & power**: gets every motor/relay/display/output working and wired safely.
3. **Person C — Firmware logic**: writes the microcontroller code that reads A's sensors and decides what B's actuators should do.
4. **Person D — App/dashboard/enclosure**: builds the phone app or web dashboard (or, for simpler builds, the physical enclosure/mounting) and handles final integration + demo day.

This keeps everyone's work physically separable — they can work in parallel for the first 1-2 weeks and only need to integrate in the last week.

---

## The 20 ideas

**1. Smart Parking Slot Indicator (IR/ultrasonic sensors + LED display board)**
4-6 parking slots, each with an ultrasonic/IR sensor. A central ESP32 shows free/occupied on an LED matrix or 7-segment board, and a simple web page shows the same live. Very well-scoped, cheap parts, clean demo.

**2. Gas Leak Detection + Auto-Ventilation System**
MQ-series gas sensor detects LPG/smoke, triggers a buzzer + relay-controlled exhaust fan, and sends an alert (buzzer + optional SMS/app notification via WiFi). Kitchen/industrial safety framing, physically satisfying demo (blow lighter gas near sensor, fan turns on).

**3. Smart Irrigation System (Soil Moisture + Auto Pump)**
Soil-moisture sensors across a few "plots," ESP32 decides when to run a relay-controlled water pump, app/dashboard shows moisture history and manual override. Classic, well-documented, parts are cheap and available everywhere.

**4. RFID-Based Attendance / Access Control System**
RFID reader logs card taps, ESP32 checks against a stored list, unlocks a servo-driven "door latch" or lights a green/red LED, and logs attendance to a simple app/spreadsheet-backed dashboard. Clean split: RFID+lock hardware / firmware logic / attendance app.

**5. Home Automation via App + Voice Trigger (Relay Bank)**
4-6 relays controlling lamps/fan/etc., controlled from a simple app (WiFi, ESP32) and optionally a basic voice command (using a phone's built-in speech-to-text feeding into the app, not embedded voice recognition — keeps it simple). Very demoable, everyone understands it instantly.

**6. Obstacle-Avoiding + Line-Following Robot (switchable modes)**
A small 2-motor robot chassis with IR line sensors and an ultrasonic sensor, switchable between line-following and obstacle-avoidance mode via a button or app command. Motor driver + chassis is one clean hardware chunk, sensor logic is another, mode-switching firmware is a third.

**7. Smart Dustbin (Auto-Lid + Fill-Level Monitoring)**
Ultrasonic sensor opens the lid via servo when a hand approaches, a second ultrasonic sensor (or the same one, timed) reports fill level to a small dashboard so "collection" can be scheduled. Simple, cheap, very demoable, good "smart city" framing.

**8. Weather Station with Live Dashboard**
DHT22/BMP280 (temp/humidity/pressure) + rain sensor + light sensor, ESP32 logs to a simple web dashboard with live graphs and historical data. Good if the team wants more "dashboard/app" work and less actuator/mechanical work.

**9. Smart Energy Meter / Appliance Monitor**
Current sensor (ACS712) + voltage sensing on a mains-safe low-voltage demo circuit (or simulate with a DC load bank for safety), computes power usage per appliance, shows live wattage + daily kWh estimate on a dashboard. Good "practical/useful" framing, moderate electrical-safety care needed.

**10. Fingerprint/RFID Dual-Factor Door Lock**
Combines an RFID tap AND a fingerprint sensor — both must match before a servo-driven lock opens. Splits cleanly: fingerprint module + firmware / RFID module + firmware / lock mechanism+servo / app-side access log.

**11. Automatic Plant Watering + Grow-Light System**
Like #3 but adds a light sensor (LDR) and a relay-controlled grow light that turns on/off based on ambient light + a schedule, alongside the moisture-based watering. Slightly more subsystems than #3 if the team wants more to split.

**12. Smart Traffic Light System (Mini Intersection Model)**
A physical 4-way mini traffic-light model (LEDs) with ultrasonic sensors detecting vehicle presence/density per lane, adjusting green-light duration dynamically instead of fixed timing. Very visual, easy to explain, good demo-day "wow."

**13. Elderly/Patient Fall-Detection + Alert Wearable**
MPU6050 accelerometer/gyro on a wearable strap detects a fall pattern (sudden acceleration spike + orientation change), triggers a buzzer locally and sends an alert via WiFi/app to a "caregiver" screen. Good medical-IoT framing, straightforward sensor-threshold logic.

**14. Smart Street-Light System (Ambient-Light + Motion Adaptive)**
LDR sensors control base brightness (dim at night, off in daylight), PIR motion sensors brighten lights further when someone approaches, simulating adaptive street lighting. Very clean 2-input, 1-output logic — good if the team wants a simpler firmware layer and more time on the physical mounting/enclosure of multiple "light poles."

**15. Voice-Controlled Robotic Arm (Basic Pick-and-Place)**
A simple 3-4 servo robotic arm, controlled either via a phone app with directional buttons or basic preset voice commands ("pick", "place", "home") processed on the phone side and sent over WiFi/Bluetooth as simple commands. Good hardware complexity (servo arm build) balanced by simple software (few discrete commands, not real robotics/inverse-kinematics).

**16. Smart Agriculture Scarecrow / Animal Intrusion Deterrent**
PIR/ultrasonic detects motion in a field zone, triggers a loud buzzer + flashing lights + optional simple "arm movement" via a servo, and logs intrusion events with timestamp to a dashboard. Fun, practical, and very easy to demo indoors with a small model field.

**17. Wireless Health Vitals Monitor (Pulse + Temperature Dashboard)**
Pulse sensor (e.g. MAX30100) + body-temperature sensor on a wearable, streaming live BPM/temp to a phone app or web dashboard over WiFi/BLE. Splits cleanly into sensor-wearable hardware, firmware data-streaming, and app-side live-graph display.

**18. Smart Bicycle/Vehicle Safety System (Turn Signals + Brake-Light + Alert)**
Handlebar-mounted buttons trigger LED turn signals and a brake-light triggered by an accelerometer detecting deceleration, plus a buzzer-based "too close" ultrasonic rear-proximity warning. Good real-world safety framing, moderate mechanical mounting work (good use of a 4th hardware-focused person).

**19. Automated Pet Feeder with App Scheduling**
A servo/auger-driven food dispenser triggered on a schedule set via a simple app, with a load cell or IR sensor confirming food actually dispensed (and alerting if the hopper is empty). Popular, well-documented, good balance of mechanical build (dispenser mechanism) and simple app work (schedule + status).

**20. Smart Mailbox Notifier**
A reed switch/IR sensor on a mailbox detects when it's opened/mail is inserted, sends a WiFi notification to an app, with an optional weight sensor to distinguish "empty" vs. "has mail." Low mechanical complexity, good "starter" option if the team is short on hardware-building time and wants to spend more effort polishing the app/dashboard side.

---

## Comparison table

| # | Idea | Core hardware | Core software | Weeks (realistic) | Best for |
|---|------|---------------|----------------|--------------------|----------|
| 1 | Smart parking indicator | Ultrasonic/IR x4-6, LED matrix | ESP32 logic + simple web page | 2-3 | Fast, cheap, very demoable |
| 2 | Gas leak + auto-vent | MQ gas sensor, relay, fan, buzzer | Threshold logic + alert | 2-3 | Strong safety story |
| 3 | Smart irrigation | Soil sensors, relay, pump | Threshold logic + dashboard | 2-3 | Classic, cheap, reliable parts |
| 4 | RFID attendance/lock | RFID reader, servo lock | Match logic + attendance app | 2-3 | Clean 4-way split |
| 5 | Home automation (app+voice) | Relay bank | App + phone speech-to-text | 2 | Fastest software-heavy option |
| 6 | Line-follow/obstacle robot | Chassis, motors, IR/ultrasonic | Mode-switch firmware | 3-4 | Best "robot" wow-factor |
| 7 | Smart dustbin | Servo lid, ultrasonic x1-2 | Fill-level + lid logic | 2 | Very simple, cheap |
| 8 | Weather station | Temp/humidity/pressure/rain/light sensors | Dashboard + graphs | 2-3 | Software/dashboard-heavy |
| 9 | Energy meter | Current/voltage sensor | Power calc + dashboard | 3 | Practical, moderate electrical care needed |
| 10 | Dual-factor door lock | Fingerprint + RFID + servo | Dual-match logic + log app | 3-4 | Most "security system" feel |
| 11 | Watering + grow-light | Moisture + LDR + 2 relays | Combined logic | 3 | More subsystems to split |
| 12 | Mini smart traffic light | LEDs x4, ultrasonic x4 | Adaptive timing logic | 3 | Best visual demo |
| 13 | Fall-detection wearable | MPU6050 wearable | Threshold + alert app | 2-3 | Medical-IoT framing |
| 14 | Adaptive street-light model | LDR + PIR, LEDs | Simple 2-input logic | 2 | Simplest firmware, more build/mounting work |
| 15 | Voice/app robotic arm | 3-4 servo arm | Command app + servo control | 3-4 | Good hardware build challenge |
| 16 | Smart scarecrow | PIR/ultrasonic, buzzer, servo | Motion-trigger logic + log | 2 | Fun, easy indoor demo |
| 17 | Health vitals monitor | Pulse + temp sensor wearable | Live-streaming app/dashboard | 2-3 | Clean 3-way split |
| 18 | Smart bike safety system | Buttons, LEDs, accelerometer, ultrasonic | Signal + brake-light logic | 3 | Good mechanical-mounting work |
| 19 | Automated pet feeder | Servo/auger dispenser, load cell/IR | Scheduling app + confirm logic | 3 | Popular, well-documented |
| 20 | Smart mailbox notifier | Reed switch/IR, optional weight sensor | WiFi notification app | 1-2 | Fastest/simplest overall |

## My recommendation for "simple, 4-way, few weeks"

If you want the safest bet — cheap parts, well-documented online, genuinely finishable in 2-3 weeks by 4 people without anyone becoming a bottleneck — pick from this short list:

- **#1 Smart Parking Indicator** or **#7 Smart Dustbin** — simplest overall, lowest risk, still a real demo.
- **#3 Smart Irrigation** or **#8 Weather Station** — most tutorials/parts availability online, very safe choice.
- **#12 Mini Smart Traffic Light** — if you want the best "wow" demo for the same low difficulty as the above.

Avoid starting with #10, #15, or #18 unless the team already has some hardware experience — they're still doable in a few weeks, but have more moving parts (dual sensors that must agree, a multi-servo arm, or safety-relevant mounting) and are more likely to eat extra time on mechanical fiddling rather than the core learning goal.
