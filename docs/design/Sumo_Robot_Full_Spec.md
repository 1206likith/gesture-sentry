# Autonomous Sumo-Fighting Robot — Full Build Spec

Replaces the robotic-arm idea. Autonomous robot: locates an opponent via
ultrasonic sensors, pushes it out of a ring, while IR edge-sensors stop it
from driving itself out of bounds.

## 1. Parts list & BOM (~₹3,000-4,000 total)

| Part | Qty | Approx cost (₹) | Notes |
|---|---|---|---|
| Arduino Uno/Nano (or MSP430 if syllabus requires it) | 1 | 300-400 | Arduino is easier to source/debug for a first build |
| L298N dual H-bridge motor driver | 1 | 150 | Drives both DC motors |
| DC gear motors (100-300 RPM, high torque) | 2 | 400 | Higher torque > higher speed for pushing power |
| Wheels (rubber-gripped, not smooth) | 2 | 150 | Grip matters more than speed in sumo — smooth wheels lose pushing contests |
| Ultrasonic sensor (HC-SR04) | 2-3 | 300 | Front + front-left + front-right for opponent detection arc |
| IR reflectance/edge sensor (e.g. TCRT5000 or IR proximity module) | 3-4 | 200 | Mounted facing down at the front edges — detects white ring boundary (sumo rings are black with a white border line) |
| Robot chassis (low, wide, wedge-front if possible) | 1 | 500-700 | Low ground clearance + wedge front = better pushing leverage, this matters more than code |
| Li-ion/LiPo battery pack (7.4V) or 4x AA holder | 1 | 300-400 | Needs enough current for 2 motors under load |
| Caster wheel (rear support) | 1 | 100 | Keeps chassis level |
| Jumper wires, screws, standoffs | - | 200 | |

**Total: ~₹2,600-3,450** → budget ₹3,500-4,000 with contingency.

**Weight/traction note**: in real sumo competitions, weight and grip matter as much as code — a heavier, low-slung robot with grippy wheels beats a "smarter" but lighter one. Keep this in mind during the mechanical build.

## 2. 4-way team split

- **Person A — Chassis & drivetrain**: build/assemble the chassis, mount motors + wheels + caster, ensure low center of gravity and wedge-front geometry, wire motors to the H-bridge. Deliverable: robot drives forward/back/turn via direct wired test (no sensors yet).
- **Person B — Sensor mounting & wiring**: mount and wire the 2-3 ultrasonic sensors (front arc) and 3-4 IR edge sensors (downward-facing at front corners/edges), test each sensor individually reads sane values in isolation. Deliverable: serial monitor prints live distance readings from all ultrasonics and live white/black readings from all IR sensors.
- **Person C — Control firmware/logic**: write the core decision loop — read all sensors, decide "opponent detected → charge toward it," "edge detected → reverse + turn away," "nothing detected → search pattern (spin/sweep)." Deliverable: firmware compiles, logic is testable on the bench with sensors handheld (no full robot needed yet).
- **Person D — Integration, tuning & competition testing**: combine A+B+C into the finished robot, tune IR edge-detection threshold and ultrasonic detection range/angle on the actual ring surface, run real practice matches, adjust motor speed/turn timing for reliable pushing behavior. Deliverable: robot reliably avoids the edge and reliably charges a target object placed in front of it, 5/5 trials.

Same parallel-then-converge pattern as before: A/B/C work independently weeks 1-2, D integrates and tunes in week 3.

## 3. Control logic (the actual algorithm — simple priority-based state machine)

```
loop:
  read IR_front_left, IR_front_right, IR_back (edge sensors)
  read US_front, US_left, US_right (ultrasonic, opponent detection)

  if ANY edge sensor detects white/edge:
      STOP, reverse briefly, turn away from that edge, resume search
      # edge-avoidance always overrides everything else — highest priority
  else if US_front detects object within range (e.g. < 40cm):
      drive straight forward at full power (push)
  else if US_left detects object closer than US_right:
      turn left, then re-check
  else if US_right detects object closer than US_left:
      turn right, then re-check
  else:
      # no opponent seen — search pattern
      rotate slowly in place (or sweep forward in an arc)
```

**Key design principle**: edge-avoidance must always have absolute priority over opponent-chasing — a robot that pushes hard toward an opponent but ignores the ring edge will drive itself out and lose instantly. Structure the code so the edge-check happens first, every loop iteration, unconditionally.

**Tuning notes for Person D**:
- IR edge threshold must be calibrated on the *actual* ring surface/lighting (ambient light affects IR reflectance sensors) — don't trust a value that worked on a different table.
- Ultrasonic sensors have a minimum reliable range (~2cm) and can give false readings on angled/soft surfaces — test against the actual opponent robot shape, not just a flat wall.
- Reverse-on-edge duration needs tuning: too short and the robot doesn't clear the edge before turning; too long and it wastes time / drives further into the ring than needed.

## 4. Week-by-week timeline

- **Week 1**: A builds chassis + mounts motors, gets basic forward/back/turn driving working via direct code (no sensors). B mounts all sensors and gets raw serial readings from each one individually. C writes the state-machine skeleton and tests the *logic* on the bench with sensors handheld (simulate readings without the full robot). *Milestone: robot drives on command; all sensors report plausible live values; control logic compiles and makes correct decisions given test inputs.*
- **Week 2**: A finalizes chassis geometry (weight distribution, wedge front) and wheel grip. B finalizes exact sensor mounting angles/positions on the real chassis. C integrates real sensor readings into the state machine (replacing the week-1 test/handheld values), tunes basic thresholds. *Milestone: robot avoids a hand-simulated table edge and turns toward a hand-held object, tethered/bench-tested.*
- **Week 3**: D integrates everything onto the finished, powered robot; runs it on an actual ring (real or improvised — a circle of tape on a table with a light-colored border works for early testing); tunes IR thresholds and ultrasonic angles against real conditions; runs repeated trials against a stationary "dummy" opponent object. *Milestone: 5/5 successful trials — robot stays in-ring and successfully pushes a target object.*
- **Week 4 (buffer/demo prep)**: practice matches against an actual second robot or a human-pushed obstacle, fix any last-minute edge-case failures (e.g. false edge-triggers from ring markings, opponent robot shape confusing ultrasonic readings), polish for demo day.

## Key risks to flag early

1. **Motor torque vs. speed tradeoff**: pick gear ratio for torque (pushing power), not top speed — sumo matches are won by pushing power and low center of gravity, not by being fast.
2. **IR calibration is surface/lighting-dependent**: must be tuned on the actual demo surface, not assumed from a datasheet value. Budget real time for this in week 3, don't treat it as a five-minute task.
3. **Ultrasonic sensors can miss angled/rounded opponents**: if possible, test against the actual second robot (or a same-shaped object) rather than only a flat cardboard box.
