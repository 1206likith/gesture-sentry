"""Explicit lifted-wheel bench test; 1.5s per side/direction at duty180.

Requires updated firmware and L298N mapping in WHEELS.md. Serial snapshots
verify configured GPIO levels/PWM only, not voltages at the driver or motion.
"""
from pathlib import Path
import re
import time
import serial

port=serial.Serial()
port.port='/dev/cu.usbserial-0001'
port.baudrate=115200
port.timeout=0.02
port.dtr=False
port.rts=False
lines=[]
stop_sent=False

def collect(seconds):
    captured=[]
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        raw=port.readline().decode('ascii',errors='ignore').strip()
        offsets=[raw.find(x) for x in ('TEST WHEEL','WHEEL pins','link=','Wheel PWM') if x in raw]
        if offsets:
            line=raw[min(offsets):]
            lines.append(line);captured.append(line)
            if not line.startswith('link='): print(line,flush=True)
    return captured

def snapshot():
    port.write(b'p\n')
    return collect(1.3)

try:
    port.open();port.reset_input_buffer()
    collect(4)
    port.write(b's\n0\n');collect(1)
    checks=[]
    phases=((b'U\n','LEFT FORWARD',(1,0,0,0,180,0)),
            (b'V\n','RIGHT FORWARD',(0,0,1,0,0,180)),
            (b'J\n','LEFT REVERSE',(0,1,0,0,180,0)),
            (b'K\n','RIGHT REVERSE',(0,0,0,1,0,180)))
    for command,label,expected in phases:
        print(f'RUN {label}: 1500ms, duty180/255',flush=True)
        port.write(command)
        acknowledged=False
        deadline=time.monotonic()+4
        while time.monotonic()<deadline:
            if any('TEST WHEEL ' in line and 'accepted=1' in line for line in collect(0.05)):
                acknowledged=True
                break
        if not acknowledged:
            print('No accepted-command acknowledgement; aborting.',flush=True)
            checks.append(False)
            break
        started=time.monotonic()
        captured=[]
        # Poll through the whole pulse. BLE may delay serial processing, so
        # zero snapshots at the edges are allowed; nonzero wrong states fail.
        while time.monotonic()-started<1.7:
            port.write(b'p\n')
            captured+=collect(0.18)
        samples=[]
        for line in captured:
            match=re.search(r'IN1=(\d) IN2=(\d) IN3=(\d) IN4=(\d) ENA=(\d+) ENB=(\d+)',line)
            if match: samples.append(tuple(map(int,match.groups())))
        active=[sample for sample in samples if sample[4] or sample[5]]
        valid=bool(active) and all(sample==expected for sample in active)
        print(f'GPIO/PWM check {label}: {"PASS" if valid else "FAIL/NO ACTIVE SAMPLE"}',flush=True)
        checks.append(valid)
        collect(0.4)
        stopped=snapshot()
        valid=any('ENA=0 ENB=0' in line for line in stopped)
        print(f'Expiry check: {"PASS" if valid else "FAIL"}',flush=True)
        checks.append(valid)
        if not all(checks):
            print('Aborting remaining phases after unexpected status.',flush=True)
            break
        collect(0.3)
    port.write(b's\n0\n');collect(0.4)
    snapshot()
    print('ALL OUTPUT CHECKS PASSED' if checks and all(checks) else 'OUTPUT CHECKS INCOMPLETE/FAILED',flush=True)
finally:
    if port.is_open:
        port.write(b's\n0\n');port.flush();stop_sent=True;port.close()
    log=Path(__file__).with_name('wheel-test-extended.log')
    if lines:
        log.write_text('\n'.join(lines)+'\n')
    print(f'Stop and laser-off commands sent. Log: {log}' if stop_sent else 'Port could not be opened; no test or stop commands were sent.',flush=True)
