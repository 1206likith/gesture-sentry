"""Explicit bench test: lift wheels first. Pulses each side for 200ms.

Requires the proposed L298N wiring in WHEELS.md. Output commands and PWM
configuration are observable over serial; physical rotation is not sensed.
"""
from pathlib import Path
import time
import serial

port=serial.Serial()
port.port='/dev/cu.usbserial-0001'
port.baudrate=115200
port.timeout=0.02
port.dtr=False
port.rts=False
lines=[]

def collect(seconds):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        raw=port.readline().decode('ascii',errors='ignore').strip()
        markers=('TEST WHEEL','I2C ','link=','Wheel PWM')
        offsets=[raw.find(x) for x in markers if x in raw]
        if offsets:
            line=raw[min(offsets):]
            lines.append(line)
            if not line.startswith('link='):
                print(line,flush=True)

try:
    port.open()
    port.reset_input_buffer()
    collect(4)
    port.write(b's\n0\n')
    collect(1)
    for command,side in ((b'u\n','LEFT'),(b'v\n','RIGHT')):
        print(f'Running {side}: 200ms at duty 80/255',flush=True)
        port.write(command)
        collect(0.04)
        port.write(b'i\n')
        collect(2)
    port.write(b's\n0\ni\n')
    collect(3)
finally:
    if port.is_open:
        port.write(b's\n0\n')
        port.flush()
        port.close()
    path=Path(__file__).with_name('wheel-test.log')
    path.write_text('\n'.join(lines)+'\n')
    print(f'Wheels commanded STOP. Log: {path}',flush=True)
