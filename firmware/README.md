# Gesture sentry: two ESP32 sketches

**Wheel support added:** see [WHEELS.md](../docs/guides/WHEELS.md) for the proposed L298N wiring and drive-mode controls.

Target: two original ESP32/WROOM-32 development boards, Arduino IDE, Arduino-ESP32 **3.x**. Boards such as ESP32-C3/S3 have different pins; this wiring is not interchangeable.

## Install and upload

Install **esp32 by Espressif Systems** in Boards Manager. Install these Library Manager libraries and accept their dependencies:

- ESP32Servo
- Adafruit MPU6050
- Adafruit Unified Sensor
- Adafruit SSD1306
- Adafruit GFX Library

BLE and Wire come with the ESP32 board package. Select **ESP32 Dev Module** and the appropriate USB port. Open `glove/glove.ino` and upload to the glove board. Open `turret/turret.ino` and upload to the turret board. Open Serial Monitor at **115200 baud**. Both sketches were compiled successfully on 2026-10-26 using Arduino-ESP32 3.3.12. The turret sketch was uploaded to ESP32-D0WD-V3 (MAC d4:8a:fc:61:62:f8) at /dev/cu.usbserial-0001; the uploader verified the flash hashes. Runtime serial output confirmed the main loop is running. The glove sketch was subsequently compiled and uploaded to the second ESP32 (MAC 20:9b:a9:99:28:5c) at 115200 baud with flash hash verification. Runtime reports MPU6050 initialization failure at both 0x68/0x69; BLE startup is held until the sensor initializes. Full peripheral operation is not yet verified.

## Wiring

Turret connections:

| Device | Connection |
|---|---|
| MG995 pan | Signal GPIO13; power external regulated 5V; GND common |
| SG90 tilt | Signal GPIO27; power external regulated 5V; GND common |
| HC-SR04 | TRIG GPIO5; ECHO through divider to GPIO18; VCC 5V; GND common |
| HC-SR501 PIR | OUT GPIO19; VCC 5V; GND common |
| OLED SSD1306 128x64 I2C | SDA GPIO21; SCL GPIO22; VCC 3.3V; GND common |
| RGB module | R GPIO32; G GPIO33; B GPIO4; GND common, assuming common cathode and onboard resistors |
| Laser module | Logic input S GPIO23 only if rated for 3.3V logic; supply per module rating; GND common |
| 5V buzzer | GPIO25 to transistor driver input; buzzer powered from 5V through driver |

HC-SR04 divider: **ECHO → 1kΩ → GPIO18 junction → 2kΩ → GND**. Never connect its 5V ECHO directly to GPIO18.

For an active buzzer, use an NPN transistor: GPIO25 → 1kΩ → base, emitter → GND, collector → buzzer negative, buzzer positive → 5V. Add a 10kΩ base-to-ground resistor. For a magnetic buzzer, add the appropriate flyback diode across it, cathode to +5V. The default code assumes an active buzzer; set `PASSIVE_BUZZER=true` for a passive one using a suitable driver.

If the laser module has only two wires, GPIO23 must control a transistor/MOSFET driver instead of powering it directly. Its exact supply voltage and switching arrangement depend on the actual module. Add a pull-down on the driver input so it remains off during boot. Use only a low-power visible indicator module and keep the beam away from eyes and reflective surfaces.

The diagram's second RGB module is not separately assigned any GPIOs. This implementation controls the one mapped module. An additional independently controlled RGB needs three additional pins and its actual module wiring.

Glove MPU6050/GY-521: **VCC→3.3V, GND→GND, SDA→GPIO21, SCL→GPIO22, AD0→GND**. INT is unused. Power the glove board by USB initially.

Use an external regulated **5V supply sized for both servos' combined peak/stall current**, with its ground connected to turret ESP32 GND. Power the turret ESP32 over USB for initial testing. Do not draw servo power from the ESP32 3V3 pin or through its USB/board regulator. Do not connect an unknown four-cell holder directly to VIN; determine its topology and use an appropriate regulator and protected battery arrangement first. Do not connect USB 5V and an external 5V rail together unless your particular board provides suitable power isolation. Software cannot fix the missing battery regulator/charging hardware.

## Operation

1. Initially disconnect servo horns/linkages. Boot the glove flat and still: it calibrates for one second.
2. Boot the turret. BLE connects automatically; the servos start at 90°. Fit horns only after confirming center. Start with the laser disconnected.
3. Roll the glove left/right to pan; pitch forward/back to tilt. This uses gravity-based tilt, not compass heading or drift-free yaw. Reverse the signs in the glove mapping if movement is backwards.
4. The code restricts pan to 30–150° and tilt to 45–135°, using conservative 1000–2000µs servo pulses. Reduce travel further if the mechanism binds. Servo models vary; calibrate these limits for your build.
5. OLED shows link, angles, range, PIR and laser state. Green = connected, blue = link unavailable, red = PIR motion or an object closer than 30cm. Alerts sound a short beep each second. PIR is ignored for the first 60 seconds while it settles. No ultrasonic echo is reported as unavailable, not clear space.
6. Stale glove data for 300ms or disconnection holds the servos and turns the laser off. BLE scanning/reconnection is synchronous and can pause sensing/display updates. Sensors only trigger indicators; they do not aim the mechanism or activate the laser.

Turret Serial Monitor commands:

| Key | Action |
|---|---|
| `g` | Resume gesture control |
| `h` | Hold current servo position; laser off |
| `c` | Move gradually to center and stay in hold mode |
| `l` | Manually activate laser for one second, only with a fresh glove link and gesture mode |
| `0` | Laser off immediately when processed |

Glove Serial Monitor: `c` recalibrates neutral while the glove is still. Recalibration interrupts updates, so the turret may briefly report a lost link.

The BLE link is a simple unauthenticated demonstration. It does not implement pairing or restrict connections to a particular glove. The manual hold command is not a physical emergency stop. L298N wheel control has now been added on a proposed pin mapping; see WHEELS.md. The actual motor-controller model has not been confirmed.

## Troubleshooting

- OLED blank: check whether your actual display uses address 0x3D rather than 0x3C, resolution 128x32 rather than 128x64, or an SH1106 rather than SSD1306 controller; change the configuration/library accordingly.
- MPU missing: verify power, shared ground, SDA/SCL and AD0. The glove code tries addresses 0x68 and 0x69; AD0 grounded selects 0x68. Initialization also checks the sensor identity, so another sensor model needs a matching library.
- Resets or servo jitter: check the external supply and common ground, and add appropriate bulk capacitance near the servo rail.
- RGB inverted: set `RGB_COMMON_ANODE=true` and connect the module common to its specified supply. Bare LEDs need individual current-limiting resistors; 5V RGB boards may need drivers.
- No buzzer sound: verify active/passive type and driver wiring.

API references: [Espressif BLE](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ble.html), [ESP32Servo](https://github.com/madhephaestus/ESP32Servo), [Adafruit MPU6050](https://adafruit.github.io/Adafruit_MPU6050/html/class_adafruit___m_p_u6050.html).

## Latest device check (2026-10-26)

The running main board reported `link=0 pan=90 tilt=90 cm=-1.0 PIR=0`: glove unavailable, servos commanded to center, no ultrasonic echo, PIR inactive. These are software status reports, not confirmation of mechanical movement or sensor wiring. The initial check preceded wheel support; see WHEELS.md for the subsequently added proposed L298N layout. Built binaries are in `build/turret/` and `build/glove/`.

## Peripheral diagnostics

Serial command `i` scans the main board I2C bus, reports servo PWM frequency/attachment and wheel PWM duty, and shows raw PIR level and warmup status. Zero wheel frequency while duty is zero is expected with this Arduino core.

Serial command `t` disarms the wheels, keeps the laser off, cycles RGB red/green/blue, displays HARDWARE TEST, pulses the active buzzer for 150ms, and sweeps each servo through 80°, 100°, and 90°. It ends in hold mode. Ensure the mechanism can move through that small range. `s` aborts the sequence. Output devices need visual/audible confirmation; reported angles are commands, not feedback.

The latest run is saved in `device-test.log`. To repeat using the local Python environment, run `.venv/bin/python test_devices.py`. The glove and its MPU6050 are on the other ESP32 and cannot be checked through the main board when BLE is unavailable.

## Latest glove correction

The connected sensor responds at I2C 0x68 with chip ID 0x70 (MPU6500). Firmware now supports this device as well as MPU6050 through separate initialization paths. The corrected glove firmware was flashed and hash-verified; 100-sample calibration and live changing angle packets were observed. BLE is advertising; a main-board connection has not yet been confirmed. See GLOVE_VERIFICATION.md for current evidence.

## Glove orientation learning (added after 2026-10-26)

The glove sketch no longer needs the MPU to sit in a particular direction. Serial command `o` (115200 baud) asks you to hold the hand flat, then tip the fingers down about 45 degrees, and learns the board's axes from gravity; the result is saved in flash and reloaded at boot. `r` clears it (back to the original axes), `c` recalibrates neutral. The math was checked against a simulated sensor in arbitrary and upside-down mounts; the compiled sketch is not yet flashed or tested on the real glove.

Gesture detection (double flick, shake, still, punch, sweeps): see [GESTURES.md](../docs/guides/GESTURES.md).
