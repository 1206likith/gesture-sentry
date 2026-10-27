# BACSE324 Project Ideas — 20+ Options for Your Friend's Group

Course: Embedded Systems Architecture (BACSE324), 30 marks, 3 reviews, 2nd-year group of 3.

## The template (same one your existing 8 ideas already use)

Every idea below fits one repeatable shape, so the paper/patent/demo structure is proven and low-risk:

**[protocol/mechanism event] → causes a real-time task to miss its deadline or misbehave → an attacker weaponizes it → an embedded guard detects/mitigates it → you measure a before/after number.**

The build is not the deliverable — the **measured finding** is. That is what fills the paper, the demo, and the Review-2/3 slides.

## Syllabus mapping (why every idea below is "safe" to propose)

- **Module 5 — Embedded Networking Protocols**: CAN, I2C, SPI, UART, WiFi, BLE, Zigbee, ESP-NOW, Modbus/RS-485 — all fair game.
- **Module 4 — RTOS**: deadline-miss rate, task starvation, priority inversion, watchdog resets — Course Outcome 4 literally asks you to report these numbers.
- **Module 3 — Timers / Watchdog / Interrupts**: ISR storms, timer drift, watchdog bypass/abuse.
- **Module 2 — Peripherals / ADC / Power**: sensor spoofing, ADC glitching, sleep-mode/power-state attacks.

Your friend's group already has your CAN/I2C/WiFi/BLE/Zigbee ideas taken (P1, P10, W1–W6) — everything below is a **different** protocol, mechanism, or attack surface so there's no overlap if both groups are in the same class.

---

## The 20 ideas

### A. Wired-bus / peripheral-protocol attacks (Module 5 + Module 2)

**1. SPI Bus Eavesdrop + Injection → Sensor-Fusion Poisoning**
Two ESP32s share an SPI bus (e.g. IMU sensor read). Attacker board taps MISO/MOSI, replays or forges sensor frames. Victim's sensor-fusion task acts on poisoned data and/or stalls waiting on a corrupted transaction. Guard: CRC/sequence-counter check on every SPI frame, cross-checked against expected read cadence.
*Domain: robotics / drone flight-control sensors.*

**2. UART/RS-232 Command Injection → Actuator Hijack**
Attacker taps a UART line between an MCU and an actuator driver, injects spoofed command bytes during idle gaps. Victim's control task executes bogus commands or misses its next real command's deadline waiting for a corrupted framing byte. Guard: framing + checksum + rate-of-command sanity check.
*Domain: industrial actuator control.*

**3. Modbus RTU (RS-485) Injection → PLC-style Deadline Miss**
Same family as CAN/I2C but Modbus RTU over RS-485 — the dominant industrial-automation protocol, not yet covered by your 8. Attacker injects forged Modbus frames onto the multi-drop bus; victim "PLC" (ESP32) misses its scan-cycle deadline. Guard: slave-ID + CRC + timing-window anomaly detector.
*Domain: industrial/factory automation — strong "PLC" framing, examiners like this one.*

**4. I2C Address-Spoofing (not clock-stretch) → Sensor Impersonation**
Distinct from your existing I2C clock-stretch project: here the attacker answers to a legitimate sensor's I2C address on the same bus (bus contention / address collision) instead of starving the clock. Victim reads corrupted/attacker-controlled register values. Guard: response-timing fingerprint + value-plausibility check.
*Domain: consumer electronics multi-sensor boards.*

**5. SPI Flash Glitch / Fault-Injection → Firmware Read-out or Skip**
Attacker uses voltage-glitching (cheap: a MOSFET + timer) on the SPI flash chip-select/power line during boot to skip a security check or corrupt a read. Victim's boot sequence takes a wrong branch. Guard: redundant checksum + glitch-detection via brown-out/timing sensor.
*Domain: hardware security / secure boot — closest thing to a "real" hardware attack in this list, higher wow-factor for examiners.*

### B. RTOS / scheduler-internal attacks (Module 4) — no wireless/bus needed

**6. Priority Inversion Injection Attack**
Attacker-controlled low-priority task deliberately holds a shared mutex/resource that a high-priority real-time task needs (classic priority-inversion bug, weaponized on purpose). Victim's high-priority task misses deadlines despite no network/bus attack at all. Guard: priority-inheritance protocol (FreeRTOS supports this) turned on/off to show the before/after.
*Domain: pure RTOS internals — zero extra hardware, cheapest project on this list, very "textbook Module 4" which examiners will recognize instantly.*

**7. Interrupt-Storm Denial-of-Service via GPIO**
Attacker board toggles a shared GPIO line at high frequency, causing an ISR storm on the victim (e.g. an "external interrupt" pin misused). Victim's scheduler spends all its time in ISR context, starving the real-time task. Guard: interrupt-rate limiting / debounce-aware ISR that drops excess triggers past a budget.
*Domain: generic embedded control — very cheap (one wire + one GPIO), easy to demo live.*

**8. Timer/Tick Manipulation → Deadline Miscalculation**
If using a shared/external RTC or timer peripheral that a second "attacker" task can also write to, corrupting the tick source causes the RTOS's own deadline math to go wrong (not just miss deadlines — miscalculate them). Guard: redundant/independent tick source cross-check (e.g. compare hardware timer vs. software tick counter, alarm on divergence).
*Domain: safety-critical timing — a more RTOS-internals-only demo of a nuanced fault; the "detect a fault in your own clock" angle is not written up in your P1–W6 set.*

**9. Watchdog Timer Bypass / Abuse Attack**
Attacker task deliberately "pets" (resets) the watchdog on behalf of a stuck/compromised task, hiding a real fault from the safety mechanism designed to catch it. Guard: per-task heartbeat/liveness checks feeding the watchdog instead of a single global pet, so one hung task can't be hidden by another's activity.
*Domain: safety systems — directly Module 3, and a genuinely under-explored angle (most projects use the watchdog as the defense; this one attacks the watchdog itself).*

**10. Stack-Overflow Induced Task Corruption → Silent Deadline Drift**
A deliberately-crafted recursive/large-buffer attacker task overflows its stack into an adjacent task's TCB/stack region (FreeRTOS has stack-overflow hooks, but many student projects disable them). Show what happens with the hook off (silent corruption, hard-to-diagnose deadline drift) vs. on (clean detection + task restart). Guard: FreeRTOS's own `configCHECK_FOR_STACK_OVERFLOW` plus a stack-high-water-mark monitor task.
*Domain: firmware reliability engineering — strong for the "we found a real, reproducible failure mode" narrative reviewers like.*

### C. Power / physical-layer attacks (Module 2)

**11. ADC Glitching via Power-Rail Noise Injection**
Attacker injects switching noise onto the shared power rail (e.g. via a nearby motor driver or a deliberately noisy MOSFET switching circuit) to corrupt ADC sensor readings on the victim, causing a control loop to act on bad data or miss its sampling deadline waiting for a stable read. Guard: majority-vote oversampling + rail-noise detection via a reference channel.
*Domain: automotive/industrial sensor front-ends — cheap, very hands-on hardware.*

**12. Sleep/Wake Timing Attack on Low-Power Nodes**
Attacker forces spurious wake-up interrupts on a battery-powered sensor node (e.g. via a cheap IR/ultrasonic trigger or GPIO pulse), preventing it from reaching deep sleep and blowing its power budget — a "battery-drain DoS" rather than a deadline-miss DoS, which is a nice variation on the template. Guard: wake-reason classification + rate-limited wake acceptance.
*Domain: battery-powered IoT / wearables — a genuinely different "resource" being attacked (energy, not CPU time), which stands out from the other 7 wireless ideas.*

**13. Brown-Out / Voltage-Sag Induced Reset Attack**
Attacker deliberately loads the shared power supply (e.g. switching a resistive load) to induce brown-outs on the victim MCU during critical sections, causing unclean resets mid-task. Guard: brown-out detector + non-volatile "critical section in progress" flag that forces a safe-state recovery on reboot instead of resuming blindly.
*Domain: safety-critical embedded (automotive ECUs, medical devices) — a real, well-documented automotive-ECU failure class.*

### D. Real-time networked / mixed attacks (Module 4 + 5 combined, but not yet covered protocols)

**14. NTP/Time-Sync Poisoning → Distributed Deadline Skew**
If the project uses two+ ESP32s synchronized via SNTP (common for logging timestamps), an attacker spoofs NTP responses to skew the victim's clock, causing its scheduled/networked task to fire at the wrong wall-clock time relative to a peer — a distributed-systems angle none of your 8 touch (they're all single-victim). Guard: bounded-drift sanity check + fallback to local RTC on suspicious jumps.
*Domain: distributed sensor networks — good if the group wants a "multi-node" story instead of one attacker/one victim.*

**15. MQTT Broker Flooding → Publish/Subscribe Deadline Miss**
ESP32 victim publishes sensor data / subscribes to control topics via MQTT over WiFi. Attacker floods the broker (or the victim directly) with junk MQTT messages, so the victim's networking task spends all its time parsing/ACKing and misses its control-loop deadline. Guard: per-topic rate-limiting + priority QoS handling.
*Domain: cloud-connected IoT — very "real product" framing (this is literally how commercial IoT devices work), good resume/portfolio value.*

**16. DNS Spoofing → IoT Device Redirected to Malicious Server**
Victim ESP32 resolves a hostname (e.g. for OTA update or cloud check-in) via DNS; attacker on the same network spoofs the DNS response, redirecting the victim to a malicious server. Less about deadline-miss, more about a supply-chain-style integrity attack — pairs well as a "data integrity" project like your W4 BLE-spoofing one, but for WiFi/IP instead of BLE. Guard: DNS response validation (matching query ID rigor) + pinned-IP fallback.
*Domain: IoT security / OTA update safety — timely, maps to real recent IoT CVEs.*

### E. Novel defense-only / measurement-focused ideas (lower attack complexity, strong RTOS measurement)

**17. Multi-Core Task Migration Jitter (if using ESP32 dual-core)**
No "attacker" board at all — instead, deliberately unbalanced core-affinity assignment (both cores oversubscribed on purpose) causes task migration jitter and missed deadlines, which you then fix with explicit core pinning. A "self-inflicted" fault-injection project rather than an external-attacker one — lower build complexity, good if the group is short on time/budget.
*Domain: general embedded computing — cheapest and fastest to execute of all 20, since it needs only one board.*

**18. Cache/Flash-Wait-State Induced WCET Variance**
On ESP32, flash-cache misses (e.g. from executing code placed in external flash vs. IRAM) cause unpredictable execution-time variance for a "real-time" task. Deliberately place critical code in slow flash vs. IRAM and measure the deadline-miss-rate difference — a pure Worst-Case-Execution-Time (WCET) study, directly tied to Module 4's real-time scheduling theory (Liu & Layland). No attacker at all, purely a measurement/optimization project.
*Domain: real-time systems theory — the most "textbook academic" of all 20, safest if the group is nervous about anything resembling "hacking."*

**19. GPS/GNSS Spoofing → Navigation Task Deadline & Integrity Attack**
If a cheap GPS module is available, a software-defined-radio-free simplified spoofing demo (or, more feasibly, a simulated/replayed NMEA sentence injected over UART) feeds false position data to a navigation control loop, testing both timing (does the task stall waiting for a valid fix) and integrity (does it act on a false position). Guard: NMEA checksum + physically-implausible-jump detector (max velocity/acceleration sanity check).
*Domain: automotive/drone navigation — high novelty/wow-factor, but check GPS module cost and availability before committing (~₹400-600 extra part).*

**20. Bluetooth Classic (not BLE) SPP Flood → Legacy Serial-over-Bluetooth Starvation**
Distinct from your BLE ideas: classic Bluetooth Serial Port Profile (used by many legacy industrial/consumer devices) flooded with connection attempts, starving a victim's serial-bridge task. Less novel than BLE variants (classic BT DoS is old, well-known), but genuinely different protocol stack from your W2/W4. Guard: connection-attempt rate-limiting identical in spirit to W2's BLE guard.
*Domain: legacy industrial serial bridges — weakest/most "already been done" idea on this list; included for completeness, not recommended as a top pick.*

**21. CAN-FD Bit-Rate-Switch Exploitation (bonus, if CAN-FD transceivers are available)**
A CAN-FD-specific attack: exploiting the bit-rate-switch (BRS) flag to desynchronize arbitration timing between classic-CAN and CAN-FD nodes on a mixed bus, which is a newer/more novel angle than plain CAN (your P1). Needs CAN-FD-capable transceivers (not all cheap MCP2515 modules support this) — check part availability first.
*Domain: next-gen automotive networking — most novel/publishable if hardware is available, but the riskiest on parts/feasibility.*

---

## Comparison table

| # | Idea | Domain | Extra parts cost | Difficulty | Novelty vs. your 8 | Best for |
|---|------|--------|------------------|------------|---------------------|----------|
| 1 | SPI eavesdrop/injection | Robotics/drone sensors | ~₹0 (2 ESP32s) | Moderate | High | Sensor-fusion story |
| 2 | UART command injection | Industrial actuators | ~₹0 | Low-moderate | High | Fast, simple build |
| 3 | Modbus RTU injection | Industrial/PLC | ~₹200 (RS-485 module) | Moderate | High | Strong "PLC" framing |
| 4 | I2C address-spoofing | Consumer multi-sensor | ~₹0 | Moderate | Medium (I2C already used, different mechanism) | Differentiates from your I2C project |
| 5 | SPI flash glitching | Hardware/secure boot | ~₹150 (MOSFET/timer) | High | Very high | Wow-factor, riskier |
| 6 | Priority inversion | Pure RTOS | ₹0 | Low | Very high | Cheapest + fastest, textbook-safe |
| 7 | GPIO interrupt storm | Generic embedded | ~₹0 | Low | Very high | Easiest live demo |
| 8 | Timer/tick manipulation | Safety-critical timing | ~₹0 | Moderate | Very high | Nuanced RTOS-internals angle |
| 9 | Watchdog bypass/abuse | Safety systems | ₹0 | Moderate | Very high | Under-explored, examiner-friendly |
| 10 | Stack-overflow corruption | Firmware reliability | ₹0 | Moderate | Very high | Strong "found a real bug" narrative |
| 11 | ADC glitching via rail noise | Automotive/industrial sensors | ~₹150 | Moderate | High | Hands-on hardware feel |
| 12 | Sleep/wake timing attack | Battery IoT/wearables | ~₹100 (trigger source) | Moderate | Very high | Different resource (energy not CPU) |
| 13 | Brown-out induced reset | Safety-critical ECUs | ~₹100 (load switch) | Moderate | High | Real automotive failure class |
| 14 | NTP poisoning | Distributed sensor nets | ₹0 | Moderate | Very high | Multi-node distributed-systems story |
| 15 | MQTT broker flooding | Cloud IoT | ₹0 | Low-moderate | High | Real-product framing, portfolio value |
| 16 | DNS spoofing | IoT/OTA security | ₹0 | Moderate | High | Timely, maps to real CVEs |
| 17 | Multi-core task migration jitter | General computing | ₹0 (1 board only) | Low | High | Fastest/cheapest overall |
| 18 | Cache/flash WCET variance | RT systems theory | ₹0 (1 board only) | Low-moderate | High | Safest, most "textbook" |
| 19 | GPS/GNSS spoofing | Automotive/drone nav | ~₹500 (GPS module) | High | Very high | Highest wow-factor, check parts first |
| 20 | Bluetooth Classic SPP flood | Legacy industrial serial | ₹0 | Low | Low (well-trodden) | Weakest pick, included for completeness |
| 21 | CAN-FD bit-rate-switch | Next-gen automotive | ~₹300+ (if available) | High | Very high | Most novel, riskiest on hardware |

## My recommendation, ranked for a 2nd-year group of 3 on a tight budget/timeline

1. **#6 Priority Inversion** or **#7 GPIO Interrupt Storm** — if the group wants the safest, fastest, cheapest path to a clean, defensible Review-1. Zero extra hardware, zero wireless flakiness, and the RTOS-internals angle is exactly what Module 4 asks for.
2. **#3 Modbus RTU Injection** — if they want something with real industrial-automation credibility and a "different protocol from everyone else" story, at low extra cost.
3. **#9 Watchdog Bypass** — if they want the most conceptually interesting/under-explored angle (attacking the safety mechanism itself, not just deadline-missing a task).
4. **#1 SPI Eavesdrop/Injection** — if they want something closer to your CAN/I2C style (two-board attacker/victim, bus-level) but on a bus none of your 8 use.

Avoid **#20** (too well-trodden) and be cautious with **#19/#21** unless parts availability is confirmed early — both are high-novelty but carry real hardware-sourcing risk this late in feasibility planning.

## Explaining this to your friend (plain-language summary)

Tell your friend: every single one of these 20+ ideas follows the exact same story shape as your own two projects — something in the embedded system (a bus, a timer, a radio, a power rail, even the RTOS scheduler itself) gets disturbed or attacked, a real-time task somewhere misses its deadline or does the wrong thing as a result, and then a "guard" (a piece of code they write) catches it and proves it caught it with a measured graph. That's the whole formula the syllabus rewards — it's just about picking *which* piece of the system gets disturbed. The safest, fastest options for a first pass are #6, #7, #17, and #18, because they need **zero extra hardware** and no unreliable wireless attack — just one or two boards and clever RTOS-level fault injection, which means less time debugging flaky wireless demos and more time writing the paper and running clean experiments. If they want more "wow factor" for a demo day and don't mind a bit more build risk, #3 (Modbus/industrial) or #9 (watchdog abuse) are the best balance of novelty and feasibility.
