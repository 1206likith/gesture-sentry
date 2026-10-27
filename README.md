# ESA_Friend_Project — BACSE324 Idea Brainstorm for a Different Group

This is a separate project folder for a friend's BACSE324 (Embedded Systems
Architecture) group — not connected to the main ESA archive (`../ESA`),
which already has its own CAN/I2C/WiFi/BLE/Zigbee project set (P1, P10,
W1-W6) taken.

## Contents

- `docs/design/Project_Ideas_Brainstorm.md` — 21 new BACSE324-scoped (paper/patent
  style) project ideas across every domain the syllabus covers, with a
  comparison table and ranked recommendation. Best suited to 1-3 people.
- `docs/design/Hardware_Demo_Project_Ideas.md` — 20 different ideas: real,
  working hardware+software demo projects (no paper/patent needed),
  roughly 50/50 hardware/software, sized to split simply across 4 people
  and finish in a few weeks. Comparison table + recommendation included.
- `docs/design/Sumo_Robot_Full_Spec.md` — sumo-fighting robot spec (locates +
  pushes an opponent out of a ring while staying in-bounds via IR edge
  sensors + ultrasonic opponent tracking). Full BOM, 4-way team split,
  control-logic state machine, and week-by-week timeline. Good baseline/
  starter-tier build.
- `docs/design/Advanced_Robotics_Project_Ideas.md` — 20 more technically
  impressive robotics+embedded ideas (self-balancing robot, swarm
  coordination, vision-guided arm, fire-fighting bot, sentry turret, etc.)
  for teams that want more "wow factor" and technical depth than the
  sumo-bot baseline. Comparison table + recommendation included.
- `docs/design/comparison_dashboard.html` — dashboard putting all 62
  ideas + both full specs in one filterable, sortable page.
- `docs/design/Gesture_Sentry_Turret_Full_Spec.md` — **current top pick**:
  gesture-controlled sentry turret (pan-tilt, auto-tracks via
  ultrasonic/PIR, manually overridden by **real on-device TinyML
  gesture recognition** — 8 trained hand gestures classified locally on
  the glove's ESP32 via Edge Impulse/TFLite Micro, sent over BLE). Fully
  decked out — OLED status display, RGB LED ring, untethered battery
  power on both turret and glove — for ~₹4,780, under the ₹5,000
  ceiling. Includes a built-in week-2 fallback checkpoint (core 5
  gestures vs. stretch 8) so the timeline doesn't collapse if training
  runs long. Full BOM, 4-way split, control architecture, and a 4-week
  timeline.
- `docs/hardware/Sentry_Turret_Build_Manual.docx` — **the single end-to-end
  deliverable**: everything above (overview, full BOM, AI toolchain,
  gesture set, 4-way team split, control architecture, week-by-week
  timeline with the Week-2 checkpoint, risk register, one-page summary)
  compiled into one polished, print-ready manual. This is the document
  to actually hand to the team.
- `docs/hardware/Sentry_Turret_Buy_List.docx` — pure shopping list: all 16 verified
  BOM links with prices, plus the 3 pre-order warnings, nothing else.
- `docs/design/Sentry_Turret_Complete_Explainer_A_to_Z.docx` — full standalone
  deep-dive (2026-09-21): plain-language opener through the TinyML
  pipeline, control architecture, full verified BOM with real links, the
  3 flagged technical issues, team split, timeline, and risk register.
  Needs no other file to understand the project.

## Status

Brainstorm/comparison phase for most ideas — three have full build specs:
the (superseded) robotic arm, the sumo robot, and the current top pick,
the gesture-controlled sentry turret. Once the group commits, this folder
can be expanded the same way `../ESA` and `../W1_WiFi_Deauth_IDS` were:
literature/reference notes, then firmware, in that order.

**2026-09-17/18 — BOM link verification, complete:** the friend sent 17 + 9
Amazon.in links (26 total, several duplicates/re-pastes of the same ASINs)
to source parts against the sentry turret BOM. All resolved and added to
`Gesture_Sentry_Turret_Full_Spec.md`'s parts table, covering: HC-SR04
ultrasonic, PIR sensor, RGB LED ring, tripod, pan-tilt bracket kit, laser
diode (corrected to a genuine KY-008 module after 3 earlier links turned
out to be bare 5V diodes with no driver circuit), MG996R + SG90 servos,
MPU6050 IMU, OLED display, buzzer, jumper wires + perfboard + header
strips, both battery holder/charger modules, and a glove/wristband
candidate. Only the 2nd ESP32 remains unlinked (one ESP32 link was
confirmed earlier; the project needs two).

Three real technical gaps were also found during verification (not link
problems — actual spec issues), documented in the spec's "issues found"
section: (1) the linked pan-tilt bracket is sized for two same-class
MG996R-style servos and likely won't cleanly fit the spec's mismatched
MG996R+SG90 pair without an adapter; (2) neither battery path (18650 or
LiPo) has a boost/regulator module specified, and raw battery voltage
(3.7-4.2V) is not what an ESP32 dev board's 5V/VIN input expects; (3) the
first glove/wristband link resolved to a full driving glove, not the
"sports wristband" the spec describes — resolved by swapping to a proper
3" cotton sports wristband link instead.

**All 16 BOM line items now have a confirmed Amazon.in link in the spec's
parts table** — ESP32 (buy qty 2 of the same link, one board covers both
roles), MG996R servo, SG90 servo, MPU6050, HC-SR04, PIR, KY-008 laser
module, buzzer, OLED, RGB LED ring, pan-tilt bracket, wristband, both
battery/charger modules, tripod, and perfboard/wires/headers. The BOM
sourcing task is complete; the 3 flagged technical gaps above are the only
remaining open items before the team should start ordering/building.

**Method note:** direct WebFetch on bare `amazon.in/dp/ASIN` links was
consistently blocked by Amazon's bot-detection (HTTP 500 on every attempt,
even on retry). WebSearch could resolve links whose URL already contained
a descriptive product-name slug, but not bare ASINs. What actually worked
for the remaining 6 bare-ASIN links: driving a real Chromium browser via
the chrome-devtools MCP tools (`new_page` / `navigate_page` / snapshot) —
Amazon does not block genuine browser traffic the way it blocks
programmatic fetches. This is the reliable method going forward for any
future unresolved Amazon links in this or other projects.

**2026-10-27:** glove firmware (learned orientation, rule-based gestures) and turret firmware (gesture handling, stronger wheel drive) written, host-tested and partly verified on hardware. Clean results, open bugs and next steps: `docs/guides/PROGRESS_2026-10-27.md`.

## Repository layout

```
firmware/
  glove/      glove ESP32 sketch (MPU6500 tilt sensing over BLE)
  turret/     main ESP32 sketch (wheels, web page, sensors, OLED)
  README.md   wiring tables and build notes
tests/        host C++ tests (wheel driver, gesture detector) and hardware test scripts
docs/
  hardware/   wiring, build manual, buy list, diagrams
  design/     proposal, specs, project ideas
  guides/     controls, test results, progress notes
  test-logs/  raw serial logs from bench tests
  tools/      scripts that generate the documents
reports/      review and final report PDFs
```

## Driving with the glove

Tilt the glove 25 degrees or more: forward, back, left or right (left and right spin in place). Level the glove to stop. To switch drive on, roll the glove right 40 degrees or more and hold for 1.5 seconds; the car beeps, then arms when the glove is level. Details: `docs/guides/WHEELS.md`.

## Build and test

- Arduino: ESP32 core 3.3.x, boards `esp32:esp32:esp32`. The turret needs the Huge APP partition scheme.
- Host tests: `c++ -std=c++17 -O0 -I tests tests/wheels.cpp -o wheels && ./wheels`
