# Latest glove status — 2026-10-26

**Sensor is now working.** After the wiring was corrected, I2C responded at 0x68 with WHO_AM_I=0x70, consistent with MPU6500. Added a separate MPU6500 initialization/read path in glove/Mpu6500.h while retaining MPU6050 support.

Updated firmware compiled with warnings enabled (1,126,295 bytes program; 42,916 bytes global RAM), uploaded at 115200 baud, and flash hashes verified on glove ESP32 MAC 20:9b:a9:99:28:5c.

Live startup detected MPU6500 at 0x68, calibrated from 100 valid samples, and reported increasing packet sequence numbers with changing roll/pitch and pan/tilt commands. BLE address: 20:9b:a9:99:28:5e. BLE status remained ADVERTISING throughout the observation: connection to the main board is not yet verified. Tilt orientation uses gravity from the accelerometer; the driver also reads gyro data, but gyro fusion/yaw tracking is not implemented.

Source: glove/glove.ino and glove/Mpu6500.h. Raw evidence: glove-test.log. Send c at 115200 baud to recalibrate while the glove is still; restarting also calibrates.

Firmware SHA-256: 63366b90e798665127521b6b67b966b6d25aab5e81e9272abd4419c0fc1adf37

---

Earlier diagnostic history:

# Glove firmware verification — 2026-10-26

Connected ESP32-D0WD-V3 MAC: 20:9b:a9:99:28:5c (different from main board d4:8a:fc:61:62:f8).

Compiled with Arduino-ESP32 3.3.12 and warnings enabled. Program storage 1,124,999 bytes; global RAM 42,908 bytes.

Initial 921600-baud upload lost serial communication during verification. Repeated upload at 115200 succeeded, including application flash hash verification. Firmware is installed on the glove board.

Live startup reports MPU6050 initialization failure at 0x68 and 0x69. The firmware does not start BLE or update gesture packets until a sensor initializes. This does not distinguish absent wiring from incompatible chip identity; check VCC 3.3V, GND, SDA GPIO21, SCL GPIO22 and the sensor model, then reset with glove still. No successful gyro reading or BLE connection has been verified. Raw evidence: glove-test.log.

Firmware SHA-256: 51c457e2fc8f655101cab3eef2a682922b3c7d27c6005ff66738db81cf0b50ac

## Follow-up after wiring confirmation

Uploaded and hash-verified updated glove firmware with full I2C address scan and automatic sensor initialization retry. Repeated runtime scans found zero responding I2C devices on SDA GPIO21/SCL GPIO22; both lines sampled HIGH (idle). No chip ID can be read without a responding device. BLE control remains unavailable. Check physical wiring/power and identify the module; the firmware will retry every three seconds. Raw evidence: glove-test.log.

## Main powered: BLE verification

With the main ESP32 powered, glove serial status changed to BLE=CONNECTED and remained connected across successive increasing sequence numbers during the observation. Sensor readings remained valid. This confirms a BLE client connected; the main-board logs and physical actuator response were not observed in this check. Main-board wheel drive still requires explicit d mode selection and neutral arming; g selects turret mode.
