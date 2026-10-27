# Four-wheel drive wiring and operation

This firmware now supports a **dual-channel L298N module with ENA, IN1, IN2, IN3, IN4, ENB pins**. This is a proposed hardware layout because your controller has not been identified. It is not universal firmware for arbitrary four-channel shields or I2C motor controllers. Check the labels before making these connections.

## Proposed wiring to the main ESP32

All turret connections in README.md remain assigned to their original GPIOs.

| L298N signal | Main ESP32 |
|---|---|
| IN1 | GPIO14 |
| IN2 | GPIO16 |
| IN3 | GPIO17 |
| IN4 | GPIO26 |
| ENA | GPIO2 |
| ENB | GPIO15 |
| GND | ESP32 GND and motor-supply negative |

**Remove both ENA and ENB jumper caps** before connecting the ESP32: the firmware controls those enables with PWM. Add a **10kΩ pull-down to GND on each ENA/ENB signal** so motors stay disabled during reset. GPIO2 and GPIO15 are ESP32 boot-strapping pins; do not let the module pull them up to 5V. This mapping is for the detected original ESP32; GPIO16/17 may be unavailable on boards with PSRAM.

Connect the two left motors in parallel to OUT1/OUT2 and the two right motors in parallel to OUT3/OUT4 **only if each channel can handle the combined stall current of its two motors**. The number of wheels alone does not establish driver suitability. Use another adequately rated driver if the combined load exceeds the module's continuous/thermal capability. Confirm each side rotates forward together; mirror-mounted motors may need opposite wire polarity.

Motor supply positive goes to the module's motor supply terminal, often labeled `12V`, `VS`, or `+Vm`. Choose the supply for the actual motor ratings and driver requirements. The terminal label does not mean every motor should receive 12V. The L298N module also needs its specified logic supply; its **5V regulator jumper is separate from ENA/ENB**. Configure it according to your particular module's documentation. Do not connect an external logic 5V source against an enabled onboard regulator output.

Keep the ESP32 on USB initially and the servos on their external regulated 5V supply. Share grounds. **Do not send motor/servo current through the breadboard rails or ESP32 power pins**: wire power directly with suitable wire and terminals. The breadboard can carry the logic connections and pull-down resistors. Keep supply wiring short and add suitable motor-noise suppression and supply decoupling. Keep the laser disconnected during initial drive testing.

## Controls

Open main-board Serial Monitor at **115200 baud**:

| Key | Behavior |
|---|---|
| `d` | Enable glove drive from serial (the glove can also do it: roll right 40 degrees or more and hold 1.5 s; the turret beeps). The car arms when the glove is held flat (HOLD) |
| `s` or `h` | Stop and disable the wheels |
| `l` | One-second laser pulse, only with a live glove link |
| `0` | Laser off |

There is no turret mode. Tilt the glove at least 25 degrees: pitch forward = FORWARD, pitch back = REVERSE, roll right = RIGHT, roll left = LEFT (turns spin in place). All run at full speed. A command ends when the glove returns within 15 degrees of level (HOLD), which stops the car. If both axes are tilted, the larger one wins. If tilt is reversed for the way you mount your glove sensor, change the sign of the throttle expression in `WheelDrive.h`.

Mode selection currently uses USB serial because the diagram includes no mode button. The glove firmware already sends the angles required; it needs no update for wheel control. It must be flashed onto the second ESP32 and powered before drive mode can arm.

Wheels start disabled after every boot. Loss of fresh glove data stops and disarms them. A separate FreeRTOS task disables both enables if the main loop stops updating wheel commands for more than 250ms, even during a blocking BLE operation. Stale glove data is rejected after 300ms; return to neutral for 0.5 seconds before movement resumes. This software timeout coasts the motors; it does not provide mechanical braking or a physical emergency stop.

Initial duty is capped at **120/255**. This limits PWM duty, not measured speed or current. Some motors may not start at low duty. Adjust `MAX_PWM` only after testing the drivetrain with the wheels lifted. `INVERT_LEFT` and `INVERT_RIGHT` reverse the corresponding side without rewiring. The turret-mounted ultrasonic/PIR sensors remain advisory indicators; they do not provide collision avoidance for wheel movement.

## First test

1. Connect only the ESP32 logic and L298N, following the table and pull-down requirements. Raise the wheels off the surface before applying motor power.
2. Confirm the serial log says `Wheel PWM ready (L298N); wheels disarmed`.
3. Power the flashed glove, hold it flat, and wait for `link=1`.
4. Send `d`; hold neutral until `armed=1`, then apply a small pitch and check both sides.
5. Send `s` and confirm the wheels stop. Test loss of glove power and direction reversal before putting the chassis on the floor.

Implementation uses the [Espressif LEDC API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html) and the L298's separate enable/direction inputs described by [STMicroelectronics](https://www.st.com/en/motor-drivers/l298.html).

## Bounded wheel diagnostic

The latest source adds `u` for a left-side forward pulse and `v` for a right-side forward pulse. Each is capped at PWM 80/255 and 200ms, with expiry enforced by the wheel-output task independently of the main loop. Both commands disarm gesture driving, freeze the turret and turn the laser off. `s` cancels a pulse. These commands are installed on the board. The first upload attempt was blocked by boot mode 0xB; the subsequent attempt succeeded and the flash hash was verified. Both enables were confirmed at duty 0 after startup. A subsequent pulse test confirmed the left and right PWM outputs individually at 5000Hz and duty80, followed by final duty0/0. Physical motor rotation requires user observation; see wheel-test.log.

If the controller wiring prevents flashing, power off the motor supply and disconnect the GPIO2-to-ENA signal while uploading. Ensure ENA/ENB jumper caps are removed and the intended pull-downs are installed before reconnecting. GPIO2 must be low or floating at reset to enter download mode; do not connect it to a module's 5V jumper pin.

## Extended diagnostic pending upload

The latest source adds uppercase U/V/J/K for left forward/right forward/left reverse/right reverse, each capped at 180/255 duty and 1500ms with task-enforced expiry. Lowercase u/v retain the 80/255, 200ms test. Command p reports direction levels and PWM duty. test_wheels_extended.py checks two active snapshots per phase, stops after unexpected output, and checks that both enables return to zero after expiry. Host tests and the full ESP32 build passed. Upload of this extended version failed with boot mode 0xB after the controller was reconnected; it is NOT yet installed. The old bounded-test firmware remains on the board.

The extended version was subsequently uploaded successfully and its flash hash verified. U/V/J/K and p are now installed. Awaiting reconnection of GPIO2→ENA and motor power before the extended physical test.
