# Gesture Sentry Turret — Full Wiring, Matched to What Was Actually Bought

Built strictly from the 3 receipts (28-09-2026, totals ₹110 + ₹2,703 + ₹649) — not
the spec's ideal BOM. Differences from the spec are called out where they matter.

**Parts on the receipts:**
Breadboard 400-point, Breadboard 840-point, 0.96" OLED (4-pin I2C), Female Berg
strip 40x1 (x2), 18650 battery holder (4-cell), Buzzer 5V, Two-cell charger (for
18650/16340/14500), MPU6050 GY-521, HC-SR04 ultrasonic, HC-SR501 PIR, Laser module,
SG90 micro servo, Tower Pro MG995 servo, 6x4" general-purpose PCB (perfboard),
18650 rechargeable battery (x4), ESP32 dev board (x2), RGB LED module (x2),
Jumpers (x16), 4WD acrylic chassis kit.

**Not on any receipt, but required by the spec — build will not run untethered
without these:**
- Boost converter module (e.g. MT3608) — raw 18650 voltage (3.7-4.2V) cannot feed
  an ESP32's 5V/VIN pin directly. Needed on the turret power path.
- No LiPo/TP4056 was bought — the glove side has **no battery at all** right now
  (see Glove section below).
- No voltage divider resistors (for HC-SR04 ECHO) were itemized — use 2 resistors
  from the jumper/perfboard stock or buy a 1kΩ+2kΩ pair separately.

Two ESP32 boards: **Turret ESP32** and **Glove ESP32**. All logic is 3.3V. Every
module's GND ties to the same ground rail as the ESP32's GND.

---

## TURRET ESP32

### Servos — MG995 (pan) + SG90 (tilt)
MG995 and MG996R share the same 3-wire pinout, so the spec's servo wiring applies
unchanged.

| Servo wire | Connects to |
|---|---|
| MG995 (pan) signal (orange) | GPIO13 |
| MG995 (pan) V+ (red) | External 5V rail (from the boost converter output — MG995 stall current is too high to run off the ESP32's own 5V/USB) |
| MG995 (pan) GND (brown) | Common GND rail |
| SG90 (tilt) signal (orange) | GPIO27 (GPIO12 avoided — it's a boot-strapping pin on some ESP32 boards) |
| SG90 (tilt) V+ (red) | Same external 5V rail as pan servo |
| SG90 (tilt) GND (brown) | Common GND rail |

### HC-SR04 ultrasonic
| Pin | Connects to |
|---|---|
| VCC | 5V rail |
| TRIG | GPIO5 |
| ECHO | GPIO18 — through a voltage divider (1kΩ/2kΩ), ECHO outputs 5V and the GPIO is 3.3V-only |
| GND | Common GND rail |

### PIR motion sensor (HC-SR501)
| Pin | Connects to |
|---|---|
| VCC | 5V rail |
| OUT | GPIO19 (3.3V logic out, no divider needed) |
| GND | Common GND rail |

### Laser module
Receipt says "Laser Module," not confirmed KY-008. If it has 3 pins (S/+/−) with an
onboard resistor, wire as below. If it's a bare 2-pin diode with no resistor, do
not wire it directly to a GPIO or 3V3 — add a series resistor (~100-150Ω for 3.3V)
first, or it risks the diode or the pin.

| Pin | Connects to |
|---|---|
| S (signal, if 3-pin) | GPIO23 |
| + / VCC | 3V3 |
| − / GND | Common GND rail |

### Buzzer (5V)
| Pin | Connects to |
|---|---|
| + | GPIO25 |
| − | Common GND rail |

### 0.96" OLED (I2C, 4-pin)
| Pin | Connects to |
|---|---|
| VCC | 3V3 |
| GND | Common GND rail |
| SCL | GPIO22 |
| SDA | GPIO21 |

### RGB LED module (x2 bought — not a WS2812 ring)
The receipt lists two individual "RGB LED Module" units, not the spec's 12-LED
ring. Treat each as a 4-pin common-cathode (or common-anode) RGB LED breakout with
separate R/G/B signal pins — the far simpler and more likely part for this price.
One module is enough for the status indicator the spec calls for (green/amber/red);
the second is a spare/backup, same as the spec intended an extra SG90.

| Pin (common-cathode module) | Connects to |
|---|---|
| R | GPIO32 (through onboard or added series resistor — most modules already have one) |
| G | GPIO33 |
| B | GPIO4 |
| GND (common pin) | Common GND rail |

If your module is common-anode instead, wire the common pin to 3V3 instead of GND,
and drive R/G/B LOW to turn each color on (inverted logic) in firmware.

### Mode-switch button
Not on the receipt. If a spare button exists, wire one leg to GPIO26 and the other
to GND (`INPUT_PULLUP` in firmware). If none was bought, the mode-switch gesture
(double-tap) is the only way to flip AUTO/MANUAL — fine for bench testing, but
the spec recommends a physical fallback before the live demo.

### Turret power path — gap vs. spec
4x 18650 cells → the 4-cell **holder** gives you a pack, but the receipt's "two-cell
charger" only charges 2 cells at a time outside the circuit — it is not inline
power regulation and does not belong in the live wiring path. For running the
turret:
1. Decide the holder's wiring (series for ~14.8V nominal, or parallel for ~3.7V
   at higher capacity — check how the 4-cell holder is internally wired; most
   cheap 4-slot holders are 2S2P or straight 4S, confirm with a multimeter before
   connecting anything to the ESP32).
2. **A boost converter alone is not enough if the pack is wired in series (2S/4S)
   — that output (7.4V-14.8V+) would need a buck (step-down) converter instead,
   not a boost.** Confirm the holder's actual output voltage with a multimeter
   first; this determines whether you need a buck or boost module.
3. Regulated output → ESP32 VIN/5V pin + the 5V rail feeding the servos,
   ultrasonic, PIR, buzzer, laser.
Until a buck/boost module (matched to the measured pack voltage) is bought, run
the turret on USB power for all bring-up and testing.

### Turret ESP32 pin summary
| GPIO | Function |
|---|---|
| 4 | RGB LED Blue |
| 5 | HC-SR04 TRIG |
| 13 | MG995 pan signal |
| 18 | HC-SR04 ECHO (via divider) |
| 19 | PIR OUT |
| 21 | OLED SDA |
| 22 | OLED SCL |
| 23 | Laser signal |
| 25 | Buzzer |
| 26 | Mode-switch button (if present) |
| 27 | SG90 tilt signal |
| 32 | RGB LED Red |
| 33 | RGB LED Green |
| BLE (internal radio) | receives gesture labels from Glove ESP32 |

---

## GLOVE ESP32

### MPU6050 IMU (I2C)
| Pin | Connects to |
|---|---|
| VCC | 3V3 |
| GND | Common GND rail |
| SCL | GPIO22 |
| SDA | GPIO21 |
| AD0 | GND (sets I2C address 0x68) |
| INT | not used |

### Glove power path — gap vs. spec
No LiPo cell or TP4056 is on any receipt. The glove currently has **no portable
power source**. Until one is bought, the glove ESP32 runs tethered via USB cable
to a laptop/power bank — this defeats the "untethered glove" part of the demo but
does not block firmware/gesture-training work, which can all be done over USB.
To go wireless: buy a small LiPo pouch cell + TP4056 charge module + a boost
converter (3 items, none currently owned), same power chain as the spec's glove
section.

### Glove ESP32 pin summary
| GPIO | Function |
|---|---|
| 21 | MPU6050 SDA |
| 22 | MPU6050 SCL |
| BLE (internal radio) | transmits classified gesture label to Turret ESP32 |

---

## Cross-board link

No wired connection between the two ESP32s — BLE only (Glove = peripheral
broadcasting gesture labels, Turret = central receiving them).

---

## Prototyping-stage parts (bought, used before final perfboard build)

- **Breadboard 400-point + 840-point** — use for bench-testing every module above
  before soldering to the 6x4" perfboard. Put the Turret ESP32's modules on the
  840-point (more room for servos/sensors/OLED/LEDs together), MPU6050 alone fits
  the 400-point for the glove side.
- **Female Berg strip (x2)** — solder onto the perfboard as socket headers so the
  ESP32s and modules can be plugged/unplugged from the final board rather than
  soldered directly — standard practice, do this before final assembly.
- **Jumpers (x16)** — enough for breadboard bring-up of one subsystem at a time,
  but tight for the full turret wired simultaneously (servos + ultrasonic + PIR +
  laser + buzzer + OLED + RGB LED is ~20+ individual wires). Budget for more
  jumpers, or move to perfboard sooner rather than keeping everything on jumpers.

## 4WD acrylic chassis kit (₹649) — unrelated to this build

Not part of the sentry turret. This is a wheeled-robot chassis (motors + wheels +
acrylic deck) matching a different brainstormed idea (line-follower/obstacle-avoider
or the sumo robot), not the turret spec. No motor driver (L298N or similar) was
bought to go with it — it cannot drive anything yet. Keep it aside unless the team
is also building a second, separate robot.
