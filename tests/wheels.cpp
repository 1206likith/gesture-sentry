#include <cassert>
#include <iostream>
#include "../firmware/turret/WheelDrive.h"
using W = WheelDrive;
int main() {
  WheelDrive w;
  assert(w.begin()); tick();
  assert(duty[w.ENA]==0 && duty[w.ENB]==0);
  assert(w.testPulse(80,0)); tick();
  assert(duty[w.ENA]==80 && duty[w.ENB]==0);
  fakeNow+=201; tick();
  assert(duty[w.ENA]==0 && duty[w.ENB]==0 && !w.isArmed());
  assert(w.testPulse(0,80)); tick();
  assert(duty[w.ENA]==0 && duty[w.ENB]==80);
  w.mode(false); tick();
  assert(duty[w.ENA]==0 && duty[w.ENB]==0);

  // Classification: a command starts at 25 degrees and holds until 15 degrees.
  assert(W::classify(90,90,W::HOLD)==W::HOLD);
  assert(W::classify(90,65,W::HOLD)==W::FORWARD);
  assert(W::classify(90,66,W::HOLD)==W::HOLD);        // 24 degrees: not strong enough
  assert(W::classify(90,70,W::FORWARD)==W::FORWARD);  // 20 degrees keeps it going
  assert(W::classify(90,76,W::FORWARD)==W::HOLD);     // 14 degrees ends it
  assert(W::classify(90,115,W::HOLD)==W::REVERSE);
  assert(W::classify(115,90,W::HOLD)==W::RIGHT);
  assert(W::classify(65,90,W::HOLD)==W::LEFT);
  assert(W::classify(60,60,W::HOLD)==W::FORWARD);     // tie goes to forward
  assert(W::classify(120,70,W::HOLD)==W::RIGHT);      // side tilt beats a weak pitch

  // Glove drive. A strong tilt cannot arm; a HOLD pose arms.
  w.mode(true);
  for (int i=0;i<3;++i) { fakeNow+=100; w.update(true,90,60); tick(); }
  assert(!w.isArmed() && duty[w.ENA]==0);
  fakeNow+=100; w.update(true,90,90); tick();
  assert(w.isArmed() && w.current()==W::HOLD && duty[w.ENA]==0);
  fakeNow+=100; w.update(true,90,60); tick();
  assert(w.current()==W::FORWARD && duty[w.ENA]==w.MAX_PWM && duty[w.ENB]==w.MAX_PWM);
  assert(pins[w.IN1]==HIGH && pins[w.IN2]==LOW && pins[w.IN3]==HIGH && pins[w.IN4]==LOW);
  fakeNow+=100; w.update(true,90,70); tick();         // 20 degrees: still forward
  assert(w.current()==W::FORWARD && duty[w.ENA]==w.MAX_PWM);
  fakeNow+=100; w.update(true,90,76); tick();         // 14 degrees: hold
  assert(w.current()==W::HOLD && duty[w.ENA]==0 && duty[w.ENB]==0 && w.isArmed());
  fakeNow+=100; w.update(true,90,60); tick();         // forward again, then straight to reverse
  assert(w.current()==W::FORWARD && duty[w.ENA]==w.MAX_PWM);
  fakeNow+=10; w.update(true,90,120); tick();
  assert(w.current()==W::REVERSE && duty[w.ENA]==0 && duty[w.ENB]==0);  // reversal dead time
  fakeNow+=160; w.update(true,90,120); tick();
  assert(duty[w.ENA]==w.MAX_PWM && duty[w.ENB]==w.MAX_PWM);
  assert(pins[w.IN1]==LOW && pins[w.IN2]==HIGH && pins[w.IN3]==LOW && pins[w.IN4]==HIGH);
  fakeNow+=100; w.update(true,90,90); tick();
  fakeNow+=200; w.update(true,60,90); tick();         // left: spin in place
  assert(w.current()==W::LEFT && duty[w.ENA]==w.MAX_PWM && duty[w.ENB]==w.MAX_PWM);
  assert(pins[w.IN1]==LOW && pins[w.IN2]==HIGH && pins[w.IN3]==HIGH && pins[w.IN4]==LOW);
  fakeNow+=100; w.update(true,90,90); tick();
  fakeNow+=200; w.update(true,120,90); tick();        // right: spin in place
  assert(w.current()==W::RIGHT && duty[w.ENA]==w.MAX_PWM && duty[w.ENB]==w.MAX_PWM);
  assert(pins[w.IN1]==HIGH && pins[w.IN2]==LOW && pins[w.IN3]==LOW && pins[w.IN4]==HIGH);
  fakeNow+=251; tick();                               // no loop updates: watchdog stops wheels
  assert(!w.isArmed() && duty[w.ENA]==0 && duty[w.ENB]==0);
  fakeNow+=10; w.update(true,120,90); tick();
  assert(!w.isArmed() && duty[w.ENA]==0);             // tilted: cannot re-arm
  fakeNow+=10; w.update(true,90,90); tick();
  assert(w.isArmed());                                // HOLD re-arms at once
  fakeNow+=10; w.update(false,90,60); tick();         // link loss
  assert(!w.isArmed() && duty[w.ENA]==0 && duty[w.ENB]==0);
  w.mode(false); fakeNow+=10; w.update(true,90,60); tick();
  assert(duty[w.ENA]==0 && duty[w.ENB]==0);           // disabled: no drive

  // Enable from the glove: roll right 40+ degrees and hold 1.5 s while drive is off.
  for (int i=0;i<30;++i) { fakeNow+=100; w.update(true,125,90); tick(); }
  assert(!w.isEnabled());                             // 35 degrees is not enough
  for (int i=0;i<10;++i) { fakeNow+=100; w.update(true,130,90); tick(); }
  fakeNow+=100; w.update(true,90,90); tick();         // released early: timer resets
  for (int i=0;i<14;++i) { fakeNow+=100; w.update(true,130,90); tick(); }
  assert(!w.isEnabled());
  fakeNow+=200; w.update(true,130,90); tick();        // 1.5 s reached
  assert(w.isEnabled() && !w.isArmed() && duty[w.ENA]==0 && duty[w.ENB]==0);
  fakeNow+=100; w.update(true,90,90); tick();         // level arms; nothing moves
  assert(w.isArmed() && duty[w.ENA]==0);
  w.mode(false); tick();
  for (int i=0;i<20;++i) { fakeNow+=100; w.update(false,130,90); tick(); }
  assert(!w.isEnabled());                             // no link: cannot enable

  assert(w.testPulse(255,0,10000)); tick();
  assert(duty[w.ENA]==180 && duty[w.ENB]==0);
  fakeNow+=1499; tick();
  assert(duty[w.ENA]==180);
  fakeNow+=2; tick();
  assert(duty[w.ENA]==0 && duty[w.ENB]==0);
  assert(w.testPulse(0,-180,1500)); tick();
  assert(duty[w.ENB]==180);
  fakeNow+=160; tick();
  assert(duty[w.ENB]==180 && pins[w.IN3]==LOW && pins[w.IN4]==HIGH);
  fakeNow+=1341; tick();
  assert(duty[w.ENA]==0 && duty[w.ENB]==0);

  // Web control: percent commands run without arming and stop 400 ms after the last one.
  fakeNow+=200; w.setManual(100,0); tick();
  assert(duty[w.ENA]==w.MAX_PWM && duty[w.ENB]==w.MAX_PWM);
  assert(pins[w.IN1]==HIGH && pins[w.IN2]==LOW && pins[w.IN3]==HIGH && pins[w.IN4]==LOW);
  fakeNow+=200; w.setManual(0,100); tick();
  assert(duty[w.ENA]==w.MAX_PWM && duty[w.ENB]==0);   // right side reverses only after dead time
  fakeNow+=160; w.setManual(0,100); tick();
  assert(duty[w.ENA]==w.MAX_PWM && duty[w.ENB]==w.MAX_PWM && pins[w.IN3]==LOW && pins[w.IN4]==HIGH);
  fakeNow+=399; tick();
  assert(duty[w.ENA]==w.MAX_PWM);
  fakeNow+=2; tick();
  assert(duty[w.ENA]==0 && duty[w.ENB]==0);           // timed out without a new command
  fakeNow+=200; w.setManual(100,100); tick();
  assert(duty[w.ENA]==w.MAX_PWM && duty[w.ENB]==0);   // forward plus right steer: inside wheel stops
  w.stopManual(); tick();
  assert(duty[w.ENA]==0 && duty[w.ENB]==0);
  std::cout << "PASS: classify thresholds, arming, forward/reverse/left/right/hold, reversal delay, watchdog, link loss, disable, test pulses, web control\n";
}
