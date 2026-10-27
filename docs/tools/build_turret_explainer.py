# -*- coding: utf-8 -*-
from docx import Document
from docx.shared import Pt, RGBColor, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

OUT_PATH = r"L:\Likith\Coding_Projects\ESA_Friend_Project\docs\Sentry_Turret_Complete_Explainer_A_to_Z.docx"


def set_cell_shading(cell, color_hex):
    tcPr = cell._tc.get_or_add_tcPr()
    shd = OxmlElement('w:shd')
    shd.set(qn('w:val'), 'clear')
    shd.set(qn('w:color'), 'auto')
    shd.set(qn('w:fill'), color_hex)
    tcPr.append(shd)


def add_table(doc, headers, rows, col_widths=None):
    table = doc.add_table(rows=1, cols=len(headers))
    table.style = 'Light Grid Accent 1'
    hdr_cells = table.rows[0].cells
    for i, h in enumerate(headers):
        hdr_cells[i].text = h
        for p in hdr_cells[i].paragraphs:
            for r in p.runs:
                r.bold = True
                r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
        set_cell_shading(hdr_cells[i], "2F5496")
    for row in rows:
        cells = table.add_row().cells
        for i, val in enumerate(row):
            cells[i].text = str(val)
    if col_widths:
        for i, w in enumerate(col_widths):
            for cell in table.columns[i].cells:
                cell.width = w
    return table


def add_hyperlink(paragraph, url, text, color="0563C1", underline=True):
    part = paragraph.part
    r_id = part.relate_to(
        url,
        "http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink",
        is_external=True,
    )
    hyperlink = OxmlElement('w:hyperlink')
    hyperlink.set(qn('r:id'), r_id)
    new_run = OxmlElement('w:r')
    rPr = OxmlElement('w:rPr')
    color_elem = OxmlElement('w:color')
    color_elem.set(qn('w:val'), color)
    rPr.append(color_elem)
    if underline:
        u = OxmlElement('w:u')
        u.set(qn('w:val'), 'single')
        rPr.append(u)
    new_run.append(rPr)
    t = OxmlElement('w:t')
    t.text = text
    new_run.append(t)
    hyperlink.append(new_run)
    paragraph._p.append(hyperlink)
    return hyperlink


def add_bullets(doc, items, style='List Bullet'):
    for it in items:
        doc.add_paragraph(it, style=style)


doc = Document()

# ============================================================
# TITLE PAGE / OPENER
# ============================================================
doc.add_heading('The Gesture-Controlled Sentry Turret', level=0)
sub = doc.add_paragraph()
sub.add_run('A complete, start-to-finish explanation of the project \u2014 what it is, why it is being built this way, and everything needed to build it.').italic = True
meta = doc.add_paragraph()
meta.add_run('Course: ').bold = True
meta.add_run('BACSE324 \u2014 Embedded Systems Architecture (friend\u2019s group)    ')
meta.add_run('Status as of: ').bold = True
meta.add_run('2026-09-18 \u2014 full spec + 16/16 verified parts links complete, build not yet started')

doc.add_paragraph()
doc.add_heading('Part 1 \u2014 The Plain-Language Explanation', level=1)

doc.add_heading('What is this, in one paragraph?', level=2)
doc.add_paragraph(
    'Imagine a small robotic turret that sits on a desk or tripod and can do two things. Left alone, '
    'it quietly watches the room: if something moves in front of it, it notices and turns to face it, '
    'the way a security camera might. But there is a second, more interesting mode: a person can put '
    'on a wristband, and by moving their hand \u2014 tilting it left, tilting it right, giving it a sharp '
    'flick, waving it, or tapping twice \u2014 they can take direct control of the turret and steer it '
    'themselves, entirely through gesture. When they flick their hand sharply forward, the turret '
    '"locks on": a small laser dot lights up, a buzzer beeps, and a ring of coloured lights around the '
    'base turns red. No remote control, no app, no buttons needed on the wristband itself \u2014 just hand '
    'movement.'
)

doc.add_heading('Why is this impressive for a student project?', level=2)
doc.add_paragraph(
    'The interesting part is not the turret moving \u2014 lots of student projects make a motor turn '
    'toward a sensor reading. What makes this project a genuine step up is that the wristband does not '
    'just measure "which way is the hand tilted" and forward that number. It runs a small piece of '
    'artificial intelligence directly on its own tiny computer chip, trained in advance on real '
    'recordings of each hand gesture, so that it can tell the difference between eight different '
    'motions \u2014 not by simple thresholds, but by genuinely recognizing the shape of the movement. '
    'This is the same broad idea behind how a smartwatch tells a walk from a run, or how some phones '
    'recognize a "shake to undo" gesture \u2014 done here from scratch, on cheap hardware, by a student '
    'team.'
)

doc.add_heading('The two modes, simply', level=2)
p = doc.add_paragraph()
p.add_run('AUTO mode: ').bold = True
p.add_run(
    'the turret works on its own. A cheap motion sensor (PIR) notices when someone or something enters '
    'the room, which wakes the turret up; a distance sensor (ultrasonic, the same kind of technology '
    'bats and dolphins use) then tells it roughly where the target is, and the turret turns to track it.'
)
p = doc.add_paragraph()
p.add_run('MANUAL mode: ').bold = True
p.add_run(
    'a person wears the glove/wristband. Its built-in AI model watches the hand\u2019s motion continuously, '
    'and whenever it confidently recognizes one of the trained gestures, it sends that command '
    'wirelessly (over Bluetooth) to the turret, which reacts immediately \u2014 panning left or right, '
    'resetting to auto mode, switching modes, or "locking on" with the laser and buzzer.'
)
doc.add_paragraph(
    'Switching between the two modes is itself just another gesture (a quick double-tap motion of the '
    'wrist) \u2014 though the team is keeping a physical button on the turret as a backup, in case that '
    'particular gesture gets misread during a live demo.'
)

doc.add_page_break()

# ============================================================
# PART 2 — TECHNICAL DEEP DIVE
# ============================================================
doc.add_heading('Part 2 \u2014 Technical Deep Dive', level=1)

doc.add_heading('2.1 System architecture', level=2)
doc.add_paragraph(
    'Two ESP32 microcontrollers form the system, communicating over Bluetooth Low Energy (BLE):'
)
add_bullets(doc, [
    'Turret ESP32: drives the two servos (pan and tilt), reads the ultrasonic and PIR sensors for '
    'auto-track mode, receives recognized gesture labels over BLE and maps each to a physical action, '
    'and drives the OLED display, the RGB LED ring, and the buzzer to reflect current state.',
    'Glove ESP32: continuously reads its onboard IMU (accelerometer + gyroscope), runs a trained '
    'machine-learning model directly on-chip against a rolling window of that motion data, and \u2014 only '
    'when the model is confident enough \u2014 transmits a single recognized gesture label over BLE.',
])
doc.add_paragraph(
    'Control-flow diagram, as specified:'
)
diagram = doc.add_paragraph()
diagram_run = diagram.add_run(
    "              AUTO MODE                       MANUAL MODE (gesture-AI)\n"
    "PIR/Ultrasonic --> ESP32(turret) --> servos   Glove IMU --> TFLite Micro model\n"
    "     |                  |                       (on glove ESP32, on-device)\n"
    "  detects motion    tracks + centers                    |\n"
    "                         |                        gesture label\n"
    "                         |                              |\n"
    "                    OLED + LED ring + buzzer  --BLE-->  ESP32(turret) --> servo/laser/buzzer action\n"
    "                    reflect state                            |\n"
    "                                                    OLED + LED ring + buzzer\n"
    "                                                    reflect state"
)
diagram_run.font.name = 'Consolas'
diagram_run.font.size = Pt(8)

doc.add_heading('2.2 The on-device AI: how gesture recognition actually works', level=2)
doc.add_paragraph(
    'The glove\u2019s ESP32 samples its accelerometer and gyroscope into a fixed-length window (roughly '
    '1\u20132 seconds of motion data at 50\u2013100Hz), and runs a small trained model against that window. '
    'The model only outputs a recognized gesture label once its confidence crosses a set threshold '
    '(around 80%) \u2014 this deliberately avoids the system spamming false triggers on ordinary, '
    'unintentional hand movement between real gestures. This on-device classification, not the '
    'tilt-mapping that a simpler project would use, is the project\u2019s actual technical contribution.'
)

doc.add_heading('2.3 The AI toolchain', level=2)
doc.add_paragraph('Every tool in this pipeline is free and does not require building anything from scratch:')
add_bullets(doc, [
    'Edge Impulse (free tier, web-based) \u2014 the standard, well-documented path for '
    '\u201ccollect IMU data \u2192 train a model \u2192 export TFLite Micro \u2192 run on ESP32.\u201d It has an '
    'official ESP32 tutorial and handles data collection, labeling, training, and C++ export in one flow.',
    'Model type \u2014 Edge Impulse\u2019s default \u201cMotion Classification\u201d project template: a small '
    '1D-CNN or fully-connected network over the fixed-length accelerometer+gyro window. The team should '
    'use this template rather than designing a custom architecture.',
    'Data collection \u2014 each gesture needs roughly 40\u201360 recorded samples per team member (about '
    '150\u2013250 samples per gesture in total across the team) so the model generalizes beyond any one '
    'person\u2019s specific motion style. This is explicitly the single biggest time cost in the whole '
    'project, not the training step itself, which only takes minutes once the data exists.',
])

doc.add_heading('2.4 The 8 gestures', level=2)
doc.add_paragraph(
    'The gesture set splits into a core 5 (the fallback set \u2014 alone, these already form a complete, '
    'demoable system) and 3 stretch gestures, attempted only once the core 5 are working reliably.'
)
add_table(doc, ['#', 'Gesture', 'Command', 'Tier'], [
    ['1', 'Tilt left / hold', 'Pan left (continuous while held)', 'Core'],
    ['2', 'Tilt right / hold', 'Pan right (continuous while held)', 'Core'],
    ['3', 'Sharp forward flick', 'LOCK (laser + buzzer + red ring)', 'Core'],
    ['4', 'Wave (side-to-side shake)', 'Reset to idle / auto-track mode', 'Core'],
    ['5', 'Quick double-tap motion', 'Switch AUTO \u2194 MANUAL mode', 'Core'],
    ['6', 'Tilt up / hold', 'Tilt servo up (continuous)', 'Stretch'],
    ['7', 'Tilt down / hold', 'Tilt servo down (continuous)', 'Stretch'],
    ['8', 'Circular wrist rotation', '\u201cScan\u201d \u2014 turret does a full sweep', 'Stretch'],
])
doc.add_paragraph()
doc.add_paragraph(
    'Gesture 3 (LOCK) is the demo\u2019s punchline moment \u2014 it fires the laser, buzzer, and turns the '
    'LED ring red simultaneously \u2014 so the team should deliberately collect extra training samples for '
    'this specific gesture to make it the single most reliable one in the whole set.'
)

doc.add_heading('2.5 State feedback design', level=2)
add_bullets(doc, [
    'OLED display shows: current mode (AUTO/MANUAL), the last recognized gesture label plus its '
    'confidence percentage, a live distance reading (in auto mode), and a status word '
    '(SCANNING / TRACKING / LOCKED).',
    'RGB LED ring: green = idle/scanning, amber = tracking or a gesture just recognized, red = locked-on.',
    'Buzzer: a short beep on any recognized gesture, and a distinct triple-beep specifically on lock.',
])

doc.add_page_break()

# ============================================================
# PART 3 — HARDWARE / BOM
# ============================================================
doc.add_heading('Part 3 \u2014 Full Verified Hardware List', level=1)
doc.add_paragraph(
    'Every part below was independently verified against a real Amazon.in product page \u2014 not assumed '
    'from a title alone \u2014 confirming the correct voltage, module type, and specification before being '
    'listed. Several early candidate links for the laser diode were rejected during this verification '
    'because they turned out to be bare, unprotected diode components rather than the safe module '
    'actually needed (see the warnings at the end of this section).'
)

headers = ['#', 'Part', 'Qty', 'Est. \u20b9', 'Buy link']
table = doc.add_table(rows=1, cols=len(headers))
table.style = 'Light Grid Accent 1'
hdr_cells = table.rows[0].cells
for i, h in enumerate(headers):
    hdr_cells[i].text = h
    for p in hdr_cells[i].paragraphs:
        for r in p.runs:
            r.bold = True
            r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
    set_cell_shading(hdr_cells[i], "2F5496")

bom_items = [
    ("ESP32 dev board (turret + glove, buy 2x)", "2", "900",
     [("Robocraze ESP32 NodeMCU 30-pin (CP2102)", "https://www.amazon.in/dp/B0D6GDV77P")],
     "One link, order quantity 2 \u2014 one board per role."),
    ("MG996R metal-gear servo (pan axis)", "1", "350",
     [("Robocraze MG996R TowerPro servo", "https://www.amazon.in/dp/B08F79X9CN")],
     None),
    ("SG90 micro servo (tilt axis)", "1", "198",
     [("Tishvi SG90 Micro Servo, 4.8\u20136V, 180\u00b0", "https://www.amazon.in/dp/B0GM6XHJB5")],
     "2-pack at \u20b9310 costs barely more \u2014 worth grabbing a spare."),
    ("MPU6050 IMU (glove)", "1", "150",
     [("Robocraze GY-521 MPU-6050", "https://www.amazon.in/dp/B07H3XH1TB")],
     None),
    ("HC-SR04 ultrasonic sensor (auto-track)", "1", "130",
     [("Robocraze HC-SR04, 5V", "https://www.amazon.in/Robocraze-Ultrasonic-Non-Contact-Resolution-Automation/dp/B01I1ZTPJC")],
     "Confirmed genuine HC-SR04, standard pinout."),
    ("PIR motion sensor (wake/detect)", "1", "120",
     [("Robocraze HC-SR501 PIR (2-pack)", "https://www.amazon.in/dp/B0D94LDD77")],
     "2-pack \u2014 only 1 needed, extra is a spare."),
    ("Laser diode module (KY-008, the \u201clock\u201d tag)", "1", "90",
     [("REES52 KY-008 module", "https://www.amazon.in/REES52-KY-008-Transmitter-compatible-arduino/dp/B01I1HHJZW"),
      ("or Xcluma KY-008 module", "https://www.amazon.in/xcluma-KY-008-Sensor-Module-Arduino/dp/B074QTP1RC")],
     "MUST be a KY-008 module (onboard resistor) \u2014 NOT a bare diode. Buy only ONE."),
    ("Small buzzer", "1", "40",
     [("Universal Hub 5V Passive Buzzer (2-pack)", "https://www.amazon.in/dp/B0H1WXTSVP")],
     None),
    ("0.96\" OLED display (I2C, SSD1306)", "1", "250",
     [("Tishvi 0.96\" OLED, I2C, 128x64", "https://www.amazon.in/dp/B0GWL3728B")],
     None),
    ("NeoPixel/WS2812 RGB LED ring", "1", "220",
     [("Xcluma WS2812 12-bit RGB LED ring", "https://www.amazon.in/dp/B0974Y8NMR")],
     None),
    ("Pan-tilt bracket kit", "1", "500",
     [("Pro3D pan & tilt bracket kit (for MG995/MG996R)", "https://www.amazon.in/dp/B0CV9RSL99")],
     "See bracket/servo-size warning below before ordering."),
    ("Wristband/glove base", "1", "140",
     [("Cotton Wristband, Pair of 2, 3\" wide", "https://www.amazon.in/Cotton-Wristband-Men-Women-Pair/dp/B0H6M16CGY")],
     "2-pack, only 1 needed. Add Velcro/cable ties separately \u2014 fabric alone won\u2019t hold the board steady."),
    ("18650 battery + holder (turret power)", "1", "350",
     [("Universal Hub 4pcs 3-slot 18650 holder", "https://www.amazon.in/dp/B0DNFLDKH4")],
     "Holder only, no charger/boost \u2014 see power warning below."),
    ("LiPo pouch cell + TP4056 (glove power)", "1", "300",
     [("Universal Hub TP4056 USB-C charging module", "https://www.amazon.in/dp/B0FVLYQBKC")],
     "Charging only, no boost regulator \u2014 see power warning below."),
    ("Perfboard, headers, wires", "\u2014", "250",
     [("60pc jumper wire pack", "https://www.amazon.in/IEIDidacticsTM-Jumper-Female-breadboard-jumper/dp/B099R8S3RZ"),
      ("AVS zero-PCB perfboard 15x10cm (2-pack)", "https://www.amazon.in/AVS-COMPONENTS-Printed-Circuit-Board/dp/B08KRZLPNL"),
      ("AVS female header strip", "https://www.amazon.in/AVS-COMPONENTS-Header-Stirp-Female/dp/B08HPHJJ8F")],
     "Still need heat-shrink + screws separately (generic hardware-store items)."),
    ("Small tripod / base mount", "1", "200",
     [("POLAM-FOTO mini tripod, 360\u00b0 ball head", "https://www.amazon.in/dp/B0G4LXV2SD")],
     None),
    ("Enclosure spray paint / stickers", "\u2014", "150",
     [], "Cosmetic finishing \u2014 generic craft-store item, no specific link needed."),
]

for idx, (part, qty, price, links, note) in enumerate(bom_items, 1):
    row = table.add_row().cells
    row[0].text = str(idx)
    row[1].text = part
    row[2].text = qty
    row[3].text = price
    link_cell = row[4]
    link_cell.text = ""
    p = link_cell.paragraphs[0]
    if links:
        for i, (text, url) in enumerate(links):
            if i > 0:
                p.add_run(" ")
            add_hyperlink(p, url, text)
    else:
        p.add_run("(no specific link \u2014 generic item)").italic = True
    if note:
        note_p = link_cell.add_paragraph()
        note_run = note_p.add_run(note)
        note_run.italic = True
        note_run.font.size = Pt(8)
        note_run.font.color.rgb = RGBColor(0x66, 0x66, 0x66)

doc.add_paragraph()
total_p = doc.add_paragraph()
total_p.add_run('Estimated total: ~\u20b94,780').bold = True
total_p.add_run(' \u2014 right under the \u20b95,000 ceiling. No extra hardware is needed for the AI upgrade '
                 '\u2014 the trained model runs entirely on the ESP32 already listed above; the real cost of '
                 '\u201cAI\u201d here is time (data collection and training), not parts.')

doc.add_heading('3.1 Three issues found during verification \u2014 read before ordering', level=2)

w1 = doc.add_paragraph()
w1.add_run('1. Pan-tilt bracket / servo size mismatch.').bold = True
doc.add_paragraph(
    'The linked bracket kit is explicitly sized \u201cfor MG995/MG996R\u201d \u2014 built for two same-class '
    'large servos. This BOM pairs it with one MG996R (pan) and one much smaller SG90 (tilt). The kit\u2019s '
    'tilt-axis mounting tabs and screw spacing likely will NOT fit an SG90 without an adapter plate or '
    'minor drilling. Before ordering, pick one: (a) find a bracket kit specifically listed as mixed '
    'MG996R+SG90 compatible, (b) budget for a simple adapter/shim plate, or (c) switch tilt to a second '
    'MG996R (~+\u20b9350, but re-check the top-plate weight against the pan servo\u2019s torque rating '
    'afterward).'
)
w2 = doc.add_paragraph()
w2.add_run('2. The laser must be a real KY-008 module, not a bare diode.').bold = True
doc.add_paragraph(
    'Several similarly-worded Amazon listings for \u201c3V/5V 650nm laser diode\u201d turned out to be bare '
    'diode heads with leads and no onboard resistor \u2014 wiring one straight to a GPIO risks damaging '
    'the diode or the pin. Both links given in the buy list are confirmed genuine KY-008 modules with '
    'the resistor already on the board. Buy only one.'
)
w3 = doc.add_paragraph()
w3.add_run('3. Neither power path has voltage/boost regulation specified.').bold = True
doc.add_paragraph(
    'ESP32 dev boards expect a regulated 5V input (via USB/VIN) or a clean 3.3V feed \u2014 not a raw '
    'battery voltage. A raw 18650 cell outputs 3.7\u20134.2V and a raw LiPo pouch outputs about 3.7V; '
    'neither is a stable 5V. The TP4056 module is a charge controller only \u2014 it does not boost or '
    'regulate output for powering a load. Add a small boost converter module (e.g. MT3608, ~\u20b930\u201350) '
    'to EACH power path \u2014 turret and glove \u2014 between the battery and the ESP32\u2019s 5V/VIN pin, '
    'unless the specific holder module is confirmed to already include one.'
)

doc.add_page_break()

# ============================================================
# PART 4 — TEAM SPLIT, TIMELINE, RISKS
# ============================================================
doc.add_heading('Part 4 \u2014 Team Split, Timeline, and Risks', level=1)

doc.add_heading('4.1 The 4-way team split', level=2)
roles = [
    ("Person A \u2014 Turret mechanical build",
     "Assemble the pan-tilt bracket, mount both servos plus the ultrasonic, PIR, laser, and LED ring, "
     "mount on the tripod/base, and own the enclosure/cosmetic pass. Deliverable: turret physically "
     "complete, all sensors/actuators wired to header pins, moving smoothly through its full pan/tilt "
     "range by hand."),
    ("Person B \u2014 Turret firmware (the brain)",
     "ESP32 code that reads ultrasonic + PIR for auto-track mode, receives recognized gesture commands "
     "over BLE, maps each to a servo/laser/buzzer action, and drives the OLED + RGB ring + buzzer based "
     "on current state. Deliverable: turret auto-tracks a moving hand/object, and correctly executes "
     "each incoming gesture-command label with OLED and LED ring reflecting state."),
    ("Person C \u2014 Gesture-AI: data collection, training, glove firmware",
     "The project\u2019s critical path. Assembles the wristband, runs the Edge Impulse data-collection "
     "flow across multiple team members, trains and validates the model, exports the TFLite Micro model "
     "into the glove\u2019s sketch, and sends the classified gesture label over BLE. Deliverable: glove "
     "classifies gestures locally and sends a clean label string over BLE \u2014 no laptop needed at demo "
     "time."),
    ("Person D \u2014 Integration, dataset support, and demo",
     "Helps C with data collection (recording is faster with two people), wires the recognized-gesture "
     "pipeline into the turret\u2019s action-mapping, tests full-system range/latency/accuracy, handles "
     "battery-life testing, writes and rehearses the demo script, and owns cosmetic finishing. "
     "Deliverable: full gesture-to-action pipeline works reliably; system runs untethered for a full "
     "10+ minute demo without dying."),
]
for name, desc in roles:
    p = doc.add_paragraph()
    p.add_run(name).bold = True
    doc.add_paragraph(desc)

doc.add_paragraph(
    'Person C\u2019s role is the critical path \u2014 B and D cannot fully test their integration until a '
    'trained model exists, so data collection must be front-loaded into week 1, not week 2.'
)

doc.add_heading('4.2 Week-by-week timeline (4 weeks)', level=2)

wk1 = doc.add_paragraph()
wk1.add_run('Week 1 \u2014 Hardware bring-up + data collection starts immediately').bold = True
add_bullets(doc, [
    'A: source/assemble the pan-tilt bracket kit, mount both servos, confirm full mechanical range of '
    'motion by hand (no electronics yet).',
    'B: get the ESP32 talking to both servos via basic test code (sweep pan/tilt through full range '
    'under code control); get ultrasonic + PIR reading raw values to serial monitor.',
    'C: assemble the glove (ESP32 + MPU6050 + battery on wristband); set up the Edge Impulse project; '
    'begin recording gesture samples for the core 5 gestures immediately \u2014 this is the longest-lead-'
    'time task in the whole project.',
    'D: order any missing parts immediately (contingency buffer); help C record gesture samples.',
])
doc.add_paragraph('Milestone: turret moves through full range under code; glove streams IMU data to Edge Impulse; at least 30 samples per core-5 gesture collected, from more than one team member.', style='List Bullet')

wk2 = doc.add_paragraph()
wk2.add_run('Week 2 \u2014 Model training + core logic \u2014 FALLBACK CHECKPOINT at end of this week').bold = True
add_bullets(doc, [
    'A: mount ultrasonic/PIR/laser/LED ring onto the finished bracket in final positions; begin '
    'enclosure/cosmetic work.',
    'B: write auto-track logic; wire up OLED + RGB ring + buzzer with placeholder/test states; write '
    'the BLE receiver that maps gesture labels to actions.',
    'C: finish data collection for the core 5 (target 150\u2013250 samples/gesture total), train the first '
    'model, check the confusion matrix, export to TFLite Micro, flash to the glove, confirm labels '
    'print to serial.',
    'D: QA the data collection/labeling; stress-test the trained model with people who did NOT provide '
    'training data.',
])
checkpoint = doc.add_paragraph()
checkpoint.add_run('CHECKPOINT (end of week 2): ').bold = True
checkpoint.add_run(
    'is the core-5 model hitting ~85%+ accuracy on someone new? If yes \u2192 proceed to stretch gestures '
    '6\u20138 in week 3 alongside integration. If no \u2192 do not attempt gestures 6\u20138; spend the start of '
    'week 3 collecting more core-5 data instead, and simplify the gesture set further if needed. This '
    'checkpoint exists specifically so the team makes this call deliberately, not by running out of '
    'time in week 4.'
)
doc.add_paragraph('Milestone: turret auto-tracks a hand in front of it (bench test); glove correctly classifies core-5 gestures locally and prints labels to serial (BLE transmission not required yet).', style='List Bullet')

wk3 = doc.add_paragraph()
wk3.add_run('Week 3 \u2014 Integration (scope depends on week 2\u2019s checkpoint result)').bold = True
add_bullets(doc, [
    'D leads: wire the glove\u2019s classified gesture labels over BLE into the turret\u2019s action-mapping, '
    'getting the full manual-mode pipeline working end-to-end.',
    'A: finish cosmetic pass \u2014 paint, labels, cable management, tripod/base mounting, battery '
    'placement inside an enclosure.',
    'B: polish the OLED/LED-ring state machine so all states look clean, with no flicker/lag.',
    'C: if week 2 checkpoint passed \u2014 collect data for stretch gestures 6\u20138, retrain/expand the '
    'model, re-validate with new people. If it did not pass \u2014 stay on core-5, tighten confidence '
    'thresholds and reduce false-positive triggers instead.',
])
doc.add_paragraph('Milestone: full system works untethered \u2014 auto-track, gesture-driven manual override (at least core 5), mode-switch, and lock-trigger all work reliably; all state feedback correct.', style='List Bullet')

wk4 = doc.add_paragraph()
wk4.add_run('Week 4 \u2014 Hardening + demo rehearsal (buffer week, use it fully)').bold = True
add_bullets(doc, [
    'Full team: run the complete demo sequence start-to-finish at least 10 times, fixing whatever '
    'breaks (BLE dropout, misclassified gestures under demo-room lighting/nerves, battery dying '
    'mid-demo, PIR false-triggers from the audience\u2019s own movement).',
    'Re-test gesture recognition specifically with whoever will actually perform it live on demo day.',
    'Time a full battery-life test on both turret and glove; recharge fully right before the demo.',
    'Write and rehearse a 60\u201390 second demo script, practicing the timing so it looks slick.',
])
fallback = doc.add_paragraph()
fallback.add_run('Layered fallback plan: ').bold = True
fallback.add_run(
    'if the on-device model misbehaves live, fall back to demonstrating auto-track mode alone (still a '
    'complete, impressive demo). If the full 8-gesture set is shaky, demo only the core 5 \u2014 this is '
    'not a failure, it is the plan from week 2\u2019s checkpoint working as intended. Live gesture-AI '
    'accuracy should never be a single point of failure for the whole demo.'
)

doc.add_heading('4.3 Key risks to flag early', level=2)
add_bullets(doc, [
    'Data collection is the real bottleneck, not model training. Edge Impulse\u2019s training step takes '
    'minutes; collecting 150\u2013250 clean, correctly-labeled samples per gesture across multiple people '
    'takes real hours. Start in week 1.',
    'A model trained on one person\u2019s motion style may not generalize \u2014 always validate with someone '
    'who did NOT contribute training data before trusting an accuracy number. This is the single most '
    'common TinyML mistake student teams make.',
    'BLE range/latency: test glove-to-turret BLE range and responsiveness in week 2; if laggy, budget '
    'time to optimize or fall back to WiFi/ESP-NOW.',
    'Servo torque under full sensor-rig weight: MG996R on pan should be plenty, but weigh the '
    'fully-loaded top plate early \u2014 if too heavy, the servo strains and jitters.',
    'PIR false triggers: can false-trigger on the demo audience\u2019s own movement, not just the intended '
    'target. Position/aim it narrowly and test in the actual demo room if possible.',
    'Battery life on both sides: an untethered demo that dies mid-presentation is worse than a tethered '
    'one that works \u2014 test real runtime in week 4 and always start the demo on a full charge.',
    'Laser safety: use only a genuinely low-power visible laser diode (Class 1/2, never anything '
    'marketed for burning or cutting) and never aim it at eyes/faces during testing or the demo \u2014 a '
    'strict rule, not a suggestion.',
])

doc.add_page_break()

# ============================================================
# PART 5 — STATUS
# ============================================================
doc.add_heading('Part 5 \u2014 Current Status', level=1)

done = doc.add_paragraph()
done.add_run('Done: ').bold = True
done.add_run(
    'the full build spec is written and finalized, including the control architecture, the gesture set, '
    'the 4-way team split, and the week-by-week timeline with its built-in fallback checkpoint. All 16 '
    'BOM line items have a verified, working Amazon.in purchase link \u2014 every link was checked against '
    'the real product page, not assumed from a title, and three genuine technical risks were caught and '
    'documented before any money was spent (the bracket/servo mismatch, the laser diode requirement, '
    'and the missing power regulation).'
)

not_started = doc.add_paragraph()
not_started.add_run('Not started yet: ').bold = True
not_started.add_run(
    'no parts have been purchased and no build has begun. This document, together with the buy list, '
    'represents the complete pre-procurement state \u2014 the team\u2019s next concrete step is to actually '
    'order the parts, resolve the three flagged issues (particularly the bracket/servo decision, which '
    'affects what else needs to be ordered), and begin Week 1 of the timeline above.'
)

doc.save(OUT_PATH)
print("Saved:", OUT_PATH)
print("Paragraphs:", len(doc.paragraphs))
print("Tables:", len(doc.tables))
