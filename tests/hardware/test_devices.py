"""Run the bounded peripheral diagnostic; leaves wheels disarmed and laser off.

Use .venv/bin/python test_devices.py with the main ESP32 connected over USB.
Watch the hardware: serial confirms output commands, not physical actuation.
"""
import datetime
from pathlib import Path
import time
import serial

root = Path(__file__).resolve().parent
lines = []
port = serial.Serial()
port.port = "/dev/cu.usbserial-0001"
port.baudrate = 115200
port.timeout = 0.2
port.dtr = False
port.rts = False

def collect(seconds):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        raw = port.readline().decode("ascii", errors="ignore").strip()
        # Discard startup UART noise while preserving diagnostic/status text.
        markers = ("link=", "TEST ", "I2C ", "ACK/configuration", "Wheel PWM")
        offsets = [raw.find(marker) for marker in markers if marker in raw]
        if offsets:
            line = raw[min(offsets):]
            lines.append(line)
            print(line, flush=True)

try:
    port.open()
    port.reset_input_buffer()
    port.write(b"s\n0\n")
    collect(3)
    port.write(b"t\n")
    collect(15)
    port.write(b"i\n")
    collect(3)
finally:
    if port.is_open:
        port.write(b"s\n0\n")
        port.flush()
        port.close()
    log = root / "device-test.log"
    log.write_text(datetime.datetime.now().isoformat() + "\n" + "\n".join(lines) + "\n")
    print(f"Log saved: {log}")
