# Gesture Sentry

A four-wheel ESP32 car you drive by tilting a glove. A second ESP32 on the glove reads a wrist-mounted IMU and sends the tilt over Bluetooth Low Energy; the car turns that into five commands (forward, reverse, left, right, hold). The car also hosts its own WiFi page, so a phone or laptop can drive it when the glove isn't on.

The car carries an ultrasonic range sensor, a PIR motion sensor, an RGB status LED, a buzzer, an OLED and a pan/tilt servo mount.

## How it works

```
glove ESP32 + MPU6500  --BLE-->  car ESP32  -->  L298N  -->  4 DC motors
                                     |
                                     +-- WiFi access point -> control page (phone / laptop)
                                     +-- HC-SR04, HC-SR501, OLED, RGB LED, buzzer, servos
```

- **Glove** (`firmware/glove`): reads the IMU, filters roll and pitch, and publishes them over BLE about every 40 ms.
- **Car** (`firmware/turret`): connects to the glove, turns the angles into drive commands, and runs the wheels through the L298N. A watchdog stops the wheels if the control loop stalls or the link is lost.
- **Web control** (`firmware/turret/web_ui.h`): the car runs its own WiFi network and serves a drive page with touch pad, keyboard and a stop button.

## Driving with the glove

| Glove tilt (25 degrees or more) | Car |
|---|---|
| Pitch forward | Forward |
| Pitch back | Reverse |
| Roll right | Spin right |
| Roll left | Spin left |
| Back within 15 degrees of level | Hold (stop) |

All commands run at full speed. If both axes are tilted, the larger one wins. The two thresholds (25 to start, 15 to stop) stop a command from flickering at the boundary.

**Switching drive on:** the wheels are off after every boot. Roll the glove right 40 degrees or more and hold it for 1.5 seconds; the car beeps. It then arms the next time the glove is level, so enabling can't start motion by itself. Losing the glove link disarms the car at once. Serial command `s` stops and disables the wheels; `d` enables them.

## Web control

Join the car's WiFi network (`GestureSentry`) and open `http://192.168.4.1`. The WiFi password is set in `firmware/turret/turret.ino`; change it before leaving the car unattended.

## Build

- Arduino IDE or `arduino-cli`, ESP32 core 3.3.x, board `esp32:esp32:esp32`.
- The car firmware needs the **Huge APP** partition scheme; the glove builds with the default one.
- Libraries: ESP32Servo, Adafruit GFX, Adafruit SSD1306, MPU6050.
- Wiring tables and pin assignments: `firmware/README.md` and `docs/hardware/`.

## Tests

Host tests run on a PC with no hardware:

```
c++ -std=c++17 -O0 -I tests tests/wheels.cpp -o wheels && ./wheels
```

`tests/wheels.cpp` covers the command thresholds, arming, glove enable, reversal delay, the watchdog, link loss and web control. `tests/hardware/` has serial scripts for bench checks, and `docs/test-logs/` has raw logs from them.

## Repository layout

```
firmware/
  glove/      glove sketch (IMU and BLE)
  turret/     car sketch (wheels, web page, sensors, OLED)
  README.md   wiring tables and build notes
tests/        host tests and hardware test scripts
docs/
  hardware/   wiring, build manual, buy list, diagrams
  design/     proposal and full spec
  guides/     controls, test results, progress notes
  test-logs/  raw serial logs from bench tests
  tools/      scripts that generate the documents
reports/      review and final report PDFs
```

## Status

Bench-tested: wheel driver pins on all four channels, the BLE link, and the glove IMU. Written and covered by host tests, not yet confirmed on the car: the glove tilt commands and the roll-to-enable gesture. Known issues: the OLED sometimes goes dark (suspected supply dip), and `tests/test_gestures.cpp` has one failing sweep case in the old motion-gesture detector, which the car no longer uses.
