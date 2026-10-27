# -*- coding: utf-8 -*-
"""Draws the full turret + glove wiring diagram as a PNG using only Pillow."""
from PIL import Image, ImageDraw, ImageFont

W, H = 2200, 2500
BG = (255, 255, 255)
INK = (20, 20, 20)
BOX = (245, 247, 250)
BOX_EDGE = (60, 60, 60)
WIRE_SIG = (0, 90, 200)      # signal/data wires
WIRE_PWR = (200, 40, 40)     # power (5V/3V3)
WIRE_GND = (40, 40, 40)      # ground
WIRE_I2C = (0, 140, 90)      # I2C bus
WIRE_BLE = (150, 60, 200)    # wireless link
TITLE = (10, 10, 10)

img = Image.new("RGB", (W, H), BG)
d = ImageDraw.Draw(img)

def font(size, bold=False):
    names = ["arialbd.ttf", "Arial Bold.ttf"] if bold else ["arial.ttf", "Arial.ttf"]
    for n in names:
        try:
            return ImageFont.truetype(n, size)
        except Exception:
            continue
    return ImageFont.load_default()

f_title = font(40, bold=True)
f_sub = font(22)
f_box = font(20, bold=True)
f_pin = font(16)
f_legend = font(18)
f_note = font(16)

def box(x, y, w, h, label, fill=BOX, edge=BOX_EDGE, lw=3):
    d.rectangle([x, y, x + w, y + h], fill=fill, outline=edge, width=lw)
    d.text((x + 10, y + 8), label, fill=INK, font=f_box)

def pin_label(x, y, text, color=INK, anchor="la"):
    d.text((x, y), text, fill=color, font=f_pin, anchor=anchor)

def wire(x1, y1, x2, y2, color, width=3, dash=False):
    if not dash:
        d.line([x1, y1, x2, y2], fill=color, width=width)
    else:
        import math
        total = math.hypot(x2 - x1, y2 - y1)
        steps = max(int(total // 14), 1)
        for i in range(0, steps, 2):
            t0, t1 = i / steps, min((i + 1) / steps, 1)
            d.line([x1 + (x2 - x1) * t0, y1 + (y2 - y1) * t0,
                    x1 + (x2 - x1) * t1, y1 + (y2 - y1) * t1], fill=color, width=width)

# ---------------- Title ----------------
d.text((40, 30), "Gesture Sentry Turret \u2014 Full Wiring (as bought)", fill=TITLE, font=f_title)
d.text((40, 85), "Matched to the 3 receipts \u2014 ESP32 \u00d72, MG995, SG90, MPU6050, HC-SR04, PIR,",
       fill=(70, 70, 70), font=f_sub)
d.text((40, 112), "laser module, buzzer, OLED, RGB LED module \u00d72, 4x 18650 + holder, perfboard, breadboards",
       fill=(70, 70, 70), font=f_sub)

# ====================================================================
# LEFT COLUMN: Turret ESP32 + its peripherals
# ====================================================================
TX, TY, TW, TH = 60, 220, 300, 760
box(TX, TY, TW, TH, "TURRET ESP32", fill=(230, 238, 250))
turret_pins = [
    ("3V3", 40), ("GND", 72), ("GPIO4", 120), ("GPIO5", 152), ("GPIO13", 184),
    ("GPIO18", 216), ("GPIO19", 248), ("GPIO21", 280), ("GPIO22", 312),
    ("GPIO23", 344), ("GPIO25", 376), ("GPIO26", 408), ("GPIO27", 440),
    ("GPIO32", 472), ("GPIO33", 504), ("VIN/5V", 560),
]
pin_y = {}
for label, dy in turret_pins:
    yy = TY + dy
    d.ellipse([TX + TW - 10, yy - 5, TX + TW, yy + 5], fill=INK)
    pin_label(TX + TW + 12, yy - 10, label, anchor="la")
    pin_y[label] = yy

periph_x = TX + TW + 190
bx_w, bx_h = 760, 90
peripherals = [
    ("MG995 servo (pan)", "signal \u2192 GPIO13  |  V+ \u2192 5V rail  |  GND \u2192 GND", "GPIO13"),
    ("SG90 servo (tilt)", "signal \u2192 GPIO27  |  V+ \u2192 5V rail  |  GND \u2192 GND", "GPIO27"),
    ("HC-SR04 ultrasonic", "TRIG \u2192 GPIO5  |  ECHO \u2192 GPIO18 (via 1k/2k divider)  |  VCC \u2192 5V  |  GND \u2192 GND", "GPIO5"),
    ("PIR HC-SR501", "OUT \u2192 GPIO19  |  VCC \u2192 5V  |  GND \u2192 GND", "GPIO19"),
    ("Laser module", "S \u2192 GPIO23  |  VCC \u2192 3V3  |  GND \u2192 GND", "GPIO23"),
    ("Buzzer 5V", "+ \u2192 GPIO25  |  \u2212 \u2192 GND", "GPIO25"),
    ("OLED 0.96\" I2C", "SDA \u2192 GPIO21  |  SCL \u2192 GPIO22  |  VCC \u2192 3V3  |  GND \u2192 GND", "GPIO21"),
    ("RGB LED module", "R \u2192 GPIO32  |  G \u2192 GPIO33  |  B \u2192 GPIO4  |  GND \u2192 GND", "GPIO32"),
]
py = TY
for label, pins, src_pin in peripherals:
    box(periph_x, py, bx_w, bx_h, label, fill=(248, 248, 240))
    d.text((periph_x + 12, py + 46), pins, fill=(60, 60, 60), font=f_note)
    src_y = pin_y.get(src_pin, py + bx_h / 2)
    wire(TX + TW, src_y, periph_x, py + bx_h / 2, WIRE_SIG, 2)
    py += bx_h + 18

bottom_of_peripherals = py + 10

# ====================================================================
# RIGHT COLUMN: Glove ESP32 + MPU6050
# ====================================================================
GX, GY, GW, GH = 1870, 220, 270, 300
box(GX, GY, GW, GH, "GLOVE ESP32", fill=(250, 238, 230))
glove_pins = [("3V3", 50), ("GND", 90), ("GPIO21", 150), ("GPIO22", 190), ("VIN/5V", 250)]
gpin_y = {}
for label, dy in glove_pins:
    yy = GY + dy
    d.ellipse([GX - 10, yy - 5, GX, yy + 5], fill=INK)
    pin_label(GX - 170, yy - 10, label, anchor="la")
    gpin_y[label] = yy

# MPU6050 below glove ESP32, same column, no overlap
MX, MY, MW, MH = 1870, 620, 270, 160
box(MX, MY, MW, MH, "MPU6050 (GY-521)", fill=(250, 238, 230))
d.text((MX + 10, MY + 46), "VCC  SCL  SDA  AD0  GND", fill=(60, 60, 60), font=f_note)

wire(gpin_y and (GX + 60), GY + GH, MX + 60, MY, WIRE_I2C, 3)
wire(GX + 90, GY + GH, MX + 90, MY, WIRE_I2C, 3)
d.text((GX + 20, GY + GH + 8), "I2C (GPIO21/22)", fill=WIRE_I2C, font=f_note)

# Glove power-gap note, below MPU6050, inside the right column (no overlap)
PNX, PNY, PNW, PNH = 1870, 820, 270, 160
box(PNX, PNY, PNW, PNH, "Glove power \u2014 GAP", fill=(255, 235, 235))
d.text((PNX + 12, PNY + 44), "No LiPo/TP4056", fill=(170, 0, 0), font=f_note)
d.text((PNX + 12, PNY + 66), "bought. Glove runs on", fill=(170, 0, 0), font=f_note)
d.text((PNX + 12, PNY + 88), "USB power only until a", fill=(170, 0, 0), font=f_note)
d.text((PNX + 12, PNY + 110), "battery path is added.", fill=(170, 0, 0), font=f_note)

# BLE link between the two boards (drawn above both columns, clear of peripherals)
ble_y = 175
wire(TX + 80, ble_y, GX + 60, ble_y, WIRE_BLE, 4, dash=True)
d.ellipse([TX + 75, ble_y - 5, TX + 85, ble_y + 5], fill=WIRE_BLE)
d.ellipse([GX + 55, ble_y - 5, GX + 65, ble_y + 5], fill=WIRE_BLE)
d.text((TX + 80, ble_y - 32), "BLE (wireless, no physical wire)", fill=WIRE_BLE, font=f_note)
wire(TX + 80, ble_y, TX + 80, TY, WIRE_BLE, 2, dash=True)
wire(GX + 60, ble_y, GX + 60, GY, WIRE_BLE, 2, dash=True)

# ====================================================================
# Lower sections, all below both columns, stacked with real spacing
# ====================================================================
section_y = max(bottom_of_peripherals, PNY + PNH) + 60

# Power path box
PX, PY, PW, PH = 60, section_y, 2080, 260
box(PX, PY, PW, PH, "TURRET POWER PATH (as bought)", fill=(255, 245, 230))
lines = [
    "4x 18650 cells \u2192 4-cell holder (confirm series/parallel wiring with a multimeter \u2014 determines",
    "buck vs. boost needed) \u2192 [MISSING: buck/boost converter, not on any receipt] \u2192 regulated 5V",
    "\u2192 ESP32 VIN/5V pin + 5V rail feeding servos / ultrasonic / PIR / buzzer.",
    "",
    "\"Two-cell charger\" on the receipt only charges cells outside the circuit \u2014",
    "it is NOT inline power regulation and is not part of this live wiring path.",
    "",
    "Until a buck/boost module is bought and the pack voltage confirmed: run the turret on USB power.",
]
yy = PY + 50
for i, line in enumerate(lines):
    color = (170, 0, 0) if ("MISSING" in line or "Until a buck" in line) else (120, 50, 0)
    d.text((PX + 20, yy), line, fill=color, font=f_note)
    yy += 26

section_y = PY + PH + 40

# 4WD chassis + prototyping stock, side by side
CX, CY, CW, CH = 60, section_y, 1000, 170
box(CX, CY, CW, CH, "4WD acrylic chassis kit (\u20b9649) \u2014 NOT part of this build", fill=(240, 240, 240))
d.text((CX + 20, CY + 50), "Wheeled-robot chassis from a different brainstormed idea.", fill=(90, 90, 90), font=f_note)
d.text((CX + 20, CY + 76), "No L298N motor driver bought \u2014 cannot drive anything yet.", fill=(90, 90, 90), font=f_note)
d.text((CX + 20, CY + 102), "Keep aside unless building a second, separate robot.", fill=(90, 90, 90), font=f_note)

BPX, BPY, BPW, BPH = 1100, section_y, 1040, 170
box(BPX, BPY, BPW, BPH, "Prototyping stock (bought)", fill=(240, 240, 240))
d.text((BPX + 20, BPY + 50), "Breadboards (400pt + 840pt) for bench bring-up before soldering.", fill=(90, 90, 90), font=f_note)
d.text((BPX + 20, BPY + 76), "6x4\" perfboard + 2x female Berg strip \u2192 solder as socket headers for final build.", fill=(90, 90, 90), font=f_note)
d.text((BPX + 20, BPY + 102), "16 jumpers is tight for the full turret wired at once \u2014 budget for more.", fill=(90, 90, 90), font=f_note)

section_y = CY + CH + 50

# Legend
LX, LY = 60, section_y
d.text((LX, LY), "Legend", fill=TITLE, font=f_box)
legend_items = [
    (WIRE_SIG, "Signal / GPIO data line"),
    (WIRE_PWR, "Power (5V / 3V3)"),
    (WIRE_GND, "Ground"),
    (WIRE_I2C, "I2C bus (SDA/SCL)"),
    (WIRE_BLE, "BLE wireless link (no physical wire)"),
]
for i, (color, text) in enumerate(legend_items):
    yy = LY + 40 + i * 32
    d.line([LX, yy + 10, LX + 50, yy + 10], fill=color, width=5)
    d.text((LX + 60, yy), text, fill=INK, font=f_legend)

section_y = LY + 40 + len(legend_items) * 32 + 30

# Known gaps box
GPX, GPY, GPW, GPH = 60, section_y, 2080, 220
box(GPX, GPY, GPW, GPH, "Known gaps vs. a complete build (not on any receipt)", fill=(255, 235, 235))
gaps = [
    "1. Buck/boost converter for the turret's 18650 pack \u2014 required before battery power is safe to use.",
    "2. LiPo cell + TP4056 charger + boost converter for the glove \u2014 glove has zero portable power right now.",
    "3. Voltage-divider resistors (1k\u03a9 + 2k\u03a9) for the HC-SR04 ECHO line \u2014 not itemized on any receipt.",
    "4. Physical mode-switch button (spec's recommended fallback for gesture misclassification) \u2014 not bought.",
    "5. L298N motor driver \u2014 only needed if the 4WD chassis is used on a separate build.",
]
for i, g in enumerate(gaps):
    d.text((GPX + 20, GPY + 46 + i * 30), g, fill=(140, 0, 0), font=f_note)

final_bottom = GPY + GPH + 40

# Footer
d.text((40, final_bottom), "Generated from Full_Wiring_Connections.md \u2014 BACSE324 friend group, gesture sentry turret",
       fill=(130, 130, 130), font=f_note)

OUT = r"L:\Likith\Coding_Projects\ESA_Friend_Project\docs\wiring_diagram.png"
img = img.crop((0, 0, W, min(final_bottom + 60, H)))
img.save(OUT)
print("Saved:", OUT, img.size)
