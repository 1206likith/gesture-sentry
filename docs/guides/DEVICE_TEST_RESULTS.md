# Device tests — 2026-10-26

| Device | Evidence | Result / remaining check |
|---|---|---|
| Main ESP32 | Compiled updated diagnostic firmware with warnings enabled; flashed and verified hashes; live serial status | Running |
| OLED | I2C ACK at 0x3C in repeated scans; buffer allocated; hardware-test text sent | Bus response verified; visible display needs observation |
| Pan MG995 | Servo attached; 50Hz configured; commanded 80/100/90 degrees | Signal configuration verified; physical movement not sensed |
| Tilt SG90 | Servo attached; 50Hz configured; commanded 80/100/90 degrees | Signal configuration verified; physical movement not sensed |
| RGB | Red/green/blue output sequence executed | Needs visual confirmation; second RGB has no independent wiring assignment |
| Buzzer | 150ms active-buzzer pulse executed | Needs audible confirmation |
| HC-SR04 | Mostly timeout/no echo during test; a later sample returned 58.4cm | Intermittent response; check wiring/power and test against a fixed target |
| PIR | Raw output LOW in diagnostic scan; warmup flag active after reflash | Motion detection not yet verified; allow 60 seconds after reset then walk across field of view |
| Wheel driver/motors | PWM initialized; duty 0/0 and disarmed confirmed; host watchdog/arming tests pass | Physical wheel test not run; awaiting L298N identity/wiring and wheels-lifted confirmation |
| Laser | OFF command sent; no activation in diagnostic | Optical test not run; awaiting safe beam-path confirmation |
| Glove ESP32 / MPU6050 | BLE link=0 | Not reachable from main board; flash/connect second ESP32 |

The main board is left in hold mode, wheels disarmed, laser off. Serial angles are commands, not measured positions. Source includes i (bus/status scan), t (bounded RGB/OLED/buzzer/servo diagnostic), s (stop/abort). See device-test.log for raw serial evidence. The initial frequency values in that log preceded a corrected printf format; the corrected final diagnostic reports both servo signals at 50Hz.

Firmware SHA-256: eb9d34e053787e76f8ba15888cc4f10c12d7137d7e98a368606f5c2caa3a07d1

## Subsequent wheel pulse test

After the timed-test firmware was uploaded and the user confirmed reconnection/readiness, each side received one 200ms pulse at duty 80/255. Live diagnostic snapshots confirmed left 5000Hz, duty 80/0; right 5000Hz, duty 0/80; and final duty 0/0 with both outputs inactive. The script sent stop and laser-off before closing the port. Physical wheel rotation still requires user observation; the drivetrain has no encoder feedback. Raw log: wheel-test.log.

## Extended wheel test

The 1500ms, duty180/255 test ran left forward, right forward, left reverse and right reverse after the user confirmed readiness. Each phase produced multiple matching GPIO/PWM snapshots and returned both enables/direction outputs to zero after expiry. The test runner now synchronizes to accepted-command acknowledgements so BLE scan delays do not cause false timing failures. Final stop/laser-off commands were sent. This verifies ESP32 configuration/readback, not driver-terminal voltages, motor power or physical rotation. Raw evidence: wheel-test-extended.log.
