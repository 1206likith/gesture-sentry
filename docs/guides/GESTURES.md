# Rule-based gestures (glove -> turret)

> **Superseded (2026-10-27):** turret mode and the motion gestures described here were removed. The glove now gives five tilt commands (forward, reverse, left, right, hold) and drive is switched on with a long roll to the right. See `WHEELS.md`.

Status 2026-10-27: flashed and tried on the real glove. Double flick, shake and punch worked on the real hand; sweeps did not yet (see PROGRESS_2026-10-27.md for the open bug and what is flashed). Flash the turret before the glove (the old turret rejects the longer packet).

| Gesture | Motion | Turret action |
|---|---|---|
| Tilt (roll/pitch) | slow wrist angle | pan/tilt in turret mode; steer/drive in drive mode (unchanged) |
| Double flick | two sharp wrist snaps 0.25-0.9 s apart | laser for 1 s (turret mode only, live link only) |
| Shake | 4+ fast side-to-side yaw swings | toggle drive / turret mode (one shake = one toggle, however long) |
| Still | level and quiet for 2 s | reported only (no action). It used to stop the car, but that clashed with the half-second level hold that arms the wheels; returning level already stops them |
| Punch | sharp thrust along the fingers (judged 120 ms after it starts; a big rotation vetoes it) | lock-on flash (red LED) + beep |
| Sweep left / right | one quick swing about the real vertical (works edge-on or flat) | pivot-turn step (0.35 s), drive mode only, then re-arm with a level hold |

## Setup needed on the glove
The detector uses the hand frame learned by `f` (flat) then `d` (fingers down) in the glove serial monitor, or `o` for one run. Without it, tilt works but gestures are off.

## Protocol
Glove packet: `seq,pan,tilt,evSeq,evCode` (evCode 1..6 as in the table order above: 1 double flick, 2 shake, 3 still, 4 punch, 5 sweep left, 6 sweep right). The turret still accepts the old `seq,pan,tilt`. It ignores any gesture that happened before it connected.

## Tests
`tests/test_gestures.cpp` (any C++17 compiler) feeds synthetic 100 Hz traces to `glove/Gestures.h`: deterministic cases, 300 randomised trials per gesture, and one hour of non-gesture motion. The last run passed: >97% per gesture, 0 false actions. This checks the logic only; real-wrist accuracy and thresholds (top of `Gestures.h`) must be tuned on the real glove.

## Hardware changes with this version
The glove gyro range is now +/-1000 deg/s with a 92-94 Hz filter (was +/-250 deg/s and 20 Hz, which clipped fast flicks). The sensor is read every 10 ms; angle packets still go out every 40 ms.
