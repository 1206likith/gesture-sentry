#pragma once
#include <Arduino.h>

// Proposed L298N wiring for this original ESP32: remove ENA/ENB jumpers.
// Two motors per side, only when the driver's current rating allows it.
class WheelDrive {
 public:
  static constexpr int IN1=14, IN2=16, IN3=17, IN4=26, ENA=2, ENB=15;
  static constexpr int MIN_PWM=90;  // 0..255; these motors do not start below about this duty
  static constexpr int MAX_PWM=200; // 0..255; top speed limit
  static constexpr bool INVERT_LEFT=false, INVERT_RIGHT=false;

  // Glove commands, all at full speed. A strong tilt starts a command; it ends only
  // when the wrist comes back inside the exit band, so it does not chatter.
  enum Command : uint8_t { HOLD, FORWARD, REVERSE, LEFT, RIGHT };
  static constexpr int ENTER_DEG=25, EXIT_DEG=15;
  // Enable drive from the glove: roll right at least ENABLE_DEG and hold it for ENABLE_MS.
  static constexpr int ENABLE_DEG=40;
  static constexpr uint32_t ENABLE_MS=1500;

  static const char *name(Command c) {
    switch (c) {
      case FORWARD: return "FORWARD";
      case REVERSE: return "REVERSE";
      case LEFT: return "LEFT";
      case RIGHT: return "RIGHT";
      default: return "HOLD";
    }
  }

  // Glove angles in degrees from level: pan 90 = level roll, tilt 90 = level pitch.
  // Forward is tilt below 90; right is pan above 90.
  static Command classify(int pan,int tilt,Command current) {
    int fwd=90-tilt, right=pan-90;
    switch (current) {
      case FORWARD: if (fwd>=EXIT_DEG) return FORWARD; break;
      case REVERSE: if (fwd<=-EXIT_DEG) return REVERSE; break;
      case RIGHT: if (right>=EXIT_DEG) return RIGHT; break;
      case LEFT: if (right<=-EXIT_DEG) return LEFT; break;
      default: break;
    }
    if (abs(fwd)>=ENTER_DEG && abs(fwd)>=abs(right)) return fwd>0 ? FORWARD : REVERSE;
    if (abs(right)>=ENTER_DEG) return right>0 ? RIGHT : LEFT;
    return HOLD;
  }

  bool begin() {
    for (int pin : {IN1,IN2,IN3,IN4,ENA,ENB}) {
      pinMode(pin,OUTPUT); digitalWrite(pin,LOW);
    }
    // Reserve channels in a separate timer from the servo channels.
    bool a=ledcAttachChannel(ENA,5000,8,14);
    bool b=ledcAttachChannel(ENB,5000,8,15);
    ready=a && b;
    if (a) ledcWrite(ENA,0);
    if (b) ledcWrite(ENB,0);
    if (!ready) return false;
    ready = xTaskCreate(taskEntry,"wheel-output",3072,this,2,nullptr)==pdPASS;
    return ready;
  }

  void mode(bool on) {
    portENTER_CRITICAL(&mux);
    enabled=on && ready; armed=false; command=HOLD; jogging=false; enableSince=0;
    updatedAt=millis();
    portEXIT_CRITICAL(&mux);
  }

  void update(bool live,int pan,int tilt) {
    uint32_t now=millis();
    portENTER_CRITICAL(&mux);
    // Any update gap or link loss disarms. Arming needs a HOLD sample, so a wrist that
    // is already tilted when the link comes back cannot start the car.
    if (now-updatedAt>250 || !live || !enabled) armed=false;
    updatedAt=now;
    // Glove enable: a long, hard roll right turns drive on. The car stays unarmed until
    // the glove is brought back to HOLD, so enabling never starts motion by itself.
    if (!enabled && live && ready && pan-90>=ENABLE_DEG) {
      if (!enableSince) enableSince=now;
      else if (now-enableSince>=ENABLE_MS) { enabled=true; enableSince=0; }
    } else {
      enableSince=0;
    }
    if (enabled && live) {
      command=classify(pan,tilt,command);
      if (command==HOLD) armed=true;
    } else {
      command=HOLD;
    }
    portEXIT_CRITICAL(&mux);
  }

  Command current() {
    portENTER_CRITICAL(&mux); Command value=command; portEXIT_CRITICAL(&mux);
    return value;
  }

  bool isArmed() {
    portENTER_CRITICAL(&mux); bool value=armed; portEXIT_CRITICAL(&mux);
    return value;
  }

  bool isEnabled() {
    portENTER_CRITICAL(&mux); bool value=enabled; portEXIT_CRITICAL(&mux);
    return value;
  }

  // Web control: throttle and steer in -100..100 (same sign convention as the glove:
  // throttle + forward, steer + right). Output stops 400 ms after the last command.
  void setManual(int throttle,int steer) {
    int thr=percentToPwm(throttle), str=percentToPwm(steer);
    int l=thr+str, r=thr-str;
    int peak=max(abs(l),abs(r));
    if (peak>MAX_PWM) { l=l*MAX_PWM/peak; r=r*MAX_PWM/peak; }
    portENTER_CRITICAL(&mux);
    manualL=INVERT_LEFT ? -l:l; manualR=INVERT_RIGHT ? -r:r;
    manualAt=millis(); manualOn=true;
    portEXIT_CRITICAL(&mux);
  }

  void stopManual() {
    portENTER_CRITICAL(&mux);
    manualOn=false; manualL=manualR=0;
    portEXIT_CRITICAL(&mux);
  }

  bool testPulse(int l,int r,uint32_t duration=200) {
    if (!ready) return false;
    portENTER_CRITICAL(&mux);
    enabled=false; armed=false; command=HOLD;
    jogLeft=constrain(l,-180,180); jogRight=constrain(r,-180,180);
    if (INVERT_LEFT) jogLeft=-jogLeft;
    if (INVERT_RIGHT) jogRight=-jogRight;
    jogUntil=millis()+constrain(duration,(uint32_t)1,(uint32_t)1500); jogging=true;
    portEXIT_CRITICAL(&mux);
    return true;
  }

 private:
  portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
  bool ready=false, enabled=false, armed=false;
  Command command=HOLD;
  uint32_t updatedAt=0, enableSince=0;
  bool jogging=false;
  int jogLeft=0,jogRight=0;
  uint32_t jogUntil=0;
  bool manualOn=false;
  int manualL=0,manualR=0;
  uint32_t manualAt=0;
  struct Side { int sign=0; uint32_t offSince=0; bool powered=false; };
  Side leftState,rightState;

  // Full-speed wheel PWM for a glove command. Left and right turns spin in place.
  static void glovePwm(Command c,int &l,int &r) {
    switch (c) {
      case FORWARD: l=r=MAX_PWM; break;
      case REVERSE: l=r=-MAX_PWM; break;
      case LEFT: l=-MAX_PWM; r=MAX_PWM; break;
      case RIGHT: l=MAX_PWM; r=-MAX_PWM; break;
      default: l=r=0; break;
    }
    if (INVERT_LEFT) l=-l;
    if (INVERT_RIGHT) r=-r;
  }

  // Web percent (-100..100) to PWM, with the same start-up jump past the dead zone.
  static int percentToPwm(int percent) {
    int magnitude=abs(percent);
    if (magnitude<=5) return 0;
    int pwm=constrain(MIN_PWM+(magnitude-5)*(MAX_PWM-MIN_PWM)/95,MIN_PWM,MAX_PWM);
    return percent<0 ? -pwm:pwm;
  }

  static void output(Side &side,int pwm,int en,int a,int b) {
    uint32_t now=millis();
    int sign=(pwm>0)-(pwm<0);
    if (!sign || (side.sign && sign!=side.sign)) {
      ledcWrite(en,0); digitalWrite(a,LOW); digitalWrite(b,LOW);
      if (side.powered) { side.offSince=now; side.powered=false; }
      // Leave at least 150ms with the bridge disabled before reversing.
      if (!sign || now-side.offSince<150) return;
    }
    digitalWrite(a,sign>0); digitalWrite(b,sign<0);
    side.sign=sign; side.powered=true; ledcWrite(en,abs(pwm));
  }

  static void taskEntry(void *argument) {
    auto *self=static_cast<WheelDrive *>(argument);
    for (;;) {
      int l=0,r=0;
      portENTER_CRITICAL(&self->mux);
      if (millis()-self->updatedAt>250) {
        self->armed=false;
      }
      if (self->manualOn && millis()-self->manualAt<=400) { l=self->manualL; r=self->manualR; }
      else if (self->enabled && self->armed) glovePwm(self->command,l,r);
      if (self->jogging) {
        if ((int32_t)(millis()-self->jogUntil)>=0) self->jogging=false;
        else { l=self->jogLeft; r=self->jogRight; }
      }
      portEXIT_CRITICAL(&self->mux);
      output(self->leftState,l,ENA,IN1,IN2);
      output(self->rightState,r,ENB,IN3,IN4);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
};
