# Wheel firmware verification — 2026-10-26

- Built with Arduino-ESP32 3.3.12; 1,150,902 bytes program storage; 43,492 bytes global RAM.
- Host checks passed: startup disabled, neutral arming, forward/reverse output, reversal delay, independent update watchdog, rearming, mixed-command speed limit, link loss and stop.
- Uploaded to ESP32-D0WD-V3, MAC d4:8a:fc:61:62:f8, /dev/cu.usbserial-0001. Esptool reported hash verification successful.
- Live serial reported mode=TURRET armed=0 at startup. Sending d switched to mode=DRIVE while armed stayed 0 with link=0. Sending s returned to mode=TURRET armed=0. Board was left stopped/disarmed.
- Motors/peripherals and glove BLE drive have not been physically validated. L298N is the proposed controller; actual controller is unconfirmed.

Firmware SHA-256: deba1d53aeb1e9dcbc8e384fde946d1d503a9e82b62ee4cdcf77d1b545f5e555

Updated diagnostic firmware subsequently compiled, flashed and verified. See [DEVICE_TEST_RESULTS.md](DEVICE_TEST_RESULTS.md) for the latest results and limitations.
