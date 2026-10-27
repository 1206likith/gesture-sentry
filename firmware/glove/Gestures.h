#pragma once
#include <stdint.h>
#include <math.h>

// Rule-based wrist gestures. Inputs are in the hand frame learned by the 'o'
// command: index 0 = toward the fingers, 1 = left, 2 = up.
// Accelerations in m/s^2, gyro in rad/s. Call update() at about 100 Hz.
// Thresholds are starting points; tests/test_gestures.cpp exercises them with
// synthetic traces and they still need tuning on the real glove.
class GestureDetector {
 public:
  enum Event { NONE=0, DOUBLE_FLICK=1, SHAKE=2, STILL=3, PUNCH=4, SWEEP_LEFT=5, SWEEP_RIGHT=6 };

  static constexpr float LP_ALPHA=0.08f;        // gravity low-pass (removes tilt from accel)
  static constexpr float FLICK_RATE=7.0f;       // rad/s about the left axis (wrist snap)
  static constexpr float FLICK_REARM=3.0f;
  static constexpr uint32_t FLICK_GAP_MIN=250, FLICK_GAP_MAX=900;  // ms between the two snaps
  static constexpr float LP_FAST=0.25f;         // faster low-pass used for punches
  static constexpr float PUNCH_ACC=2.8f;        // m/s^2 through the fast high-pass (real jabs 3.1-4.7, hard tilting <=2.2)
  static constexpr float PUNCH_MAX_ROT=6.0f;    // rad/s; a punch is mostly a translation (real jabs <=4.7, swats 10+)
  static constexpr uint32_t PUNCH_WINDOW=120;   // ms to watch for rotation before accepting a punch
  static constexpr uint32_t PUNCH_REFRACT=600;
  static constexpr float SWING_RATE=3.0f;       // rad/s yaw counted as one swing
  static constexpr float SWEEP_RATE=4.5f;       // rad/s yaw peak needed for a sweep
  static constexpr uint32_t SWEEP_SETTLE=250;   // ms with no opposite swing
  static constexpr uint32_t SWING_GAP_MAX=400;  // ms between swings of one shake
  static constexpr int SHAKE_SWINGS=4;
  static constexpr uint32_t SHAKE_RELEASE=500;  // ms of calm before another shake counts
  static constexpr float STILL_GYRO=0.35f, STILL_DYN=0.8f, STILL_TILT=12.0f;
  static constexpr uint32_t STILL_MS=2000;
  static constexpr uint32_t COOLDOWN=400;       // ms after any action gesture

  int update(uint32_t now, const float a[3], const float g[3], float rollDeg, float pitchDeg) {
    if (!init) { for (int i=0;i<3;++i) lp[i]=a[i]; init=true; }
    for (int i=0;i<3;++i) lp[i]+=LP_ALPHA*(a[i]-lp[i]);
    float dyn[3]={a[0]-lp[0],a[1]-lp[1],a[2]-lp[2]};
    float dynMag=sqrtf(dyn[0]*dyn[0]+dyn[1]*dyn[1]+dyn[2]*dyn[2]);
    // Faster high-pass for punches: only sharp impulses pass, slow tilting does not.
    if (!init2) { lp2f=a[0]; init2=true; }
    lp2f+=LP_FAST*(a[0]-lp2f);
    dynFwd=a[0]-lp2f;
    float gMag=sqrtf(g[0]*g[0]+g[1]*g[1]+g[2]*g[2]);
    // Yaw is the rotation rate about the real vertical (gravity), so sweeps and
    // shakes work whether the hand is flat or turned edge-on. Flick stays about
    // the hand's own left axis.
    float gn=sqrtf(lp[0]*lp[0]+lp[1]*lp[1]+lp[2]*lp[2]);
    float yr=gn>1.0f ? (g[0]*lp[0]+g[1]*lp[1]+g[2]*lp[2])/gn : g[2];
    yawWorld=yr;
    float pr=g[1];                                // pitch rate (flick)
    float apr=fabsf(pr), ayr=fabsf(yr);
    int ev=NONE;

    // Double flick: two wrist snaps FLICK_GAP_MIN..MAX apart. The rebound of one
    // snap lands inside the minimum gap and is ignored.
    bool flickLike=apr>FLICK_RATE && apr>1.5f*ayr;   // rotation about the hand's own axis, not the vertical
    float rot=apr>ayr ? apr : ayr;
    rotEnv=rot>rotEnv*0.95f ? rot : rotEnv*0.95f;     // recent rotation, decays over ~0.15 s
    if (flickArmed) {
      if (flickLike) {
        flickArmed=false;
        uint32_t gap=now-firstFlick;
        if (flickCount==1 && gap>=FLICK_GAP_MIN && gap<=FLICK_GAP_MAX) { flickCount=0; ev=DOUBLE_FLICK; }
        else if (flickCount==1 && gap<FLICK_GAP_MIN) { /* rebound of the same snap */ }
        else { flickCount=1; firstFlick=now; }
      }
    } else if (apr<FLICK_REARM) flickArmed=true;

    // Punch: a sharp push along the fingers. It is judged PUNCH_WINDOW ms after it
    // starts, so a swat or flick (big rotation around the push) vetoes it.
    if (punchPending) {
      if (rot>punchRotPeak) punchRotPeak=rot;
      if (now-punchStart>=PUNCH_WINDOW) {
        punchPending=false; lastPunch=now;
        if (!ev && punchRotPeak<PUNCH_MAX_ROT) ev=PUNCH;
      }
    } else if (dynFwd>PUNCH_ACC && now-lastPunch>PUNCH_REFRACT) {
      punchPending=true; punchStart=now; punchRotPeak=rotEnv>rot ? rotEnv : rot;
    }

    // Yaw swings: SHAKE_SWINGS alternating swings = shake; one clean swing = sweep.
    int ys=(flickLike || fabsf(yr)<=SWING_RATE) ? 0 : (yr>0 ? 1 : -1);
    if (shakeLocked) {
      // Ignore the rest of a long shake: one shake = one SHAKE, however long it lasts.
      if (ys) lastSwingAt=now;
      else if (now-lastSwingAt>=SHAKE_RELEASE) shakeLocked=false;
    } else if (ys) {
      if (ys!=lastSwingDir) {
        if (lastSwingDir && now-lastSwingAt<=SWING_GAP_MAX) ++swings; else { swings=1; sweepPeak=0; }
        lastSwingDir=ys;
      }
      lastSwingAt=now;
      if (ayr>sweepPeak) sweepPeak=ayr;
      if (!ev && swings>=SHAKE_SWINGS) { ev=SHAKE; resetSwings(); shakeLocked=true; }
    } else if (lastSwingDir) {
      uint32_t idle=now-lastSwingAt;
      if (swings==1 && idle>=SWEEP_SETTLE) {
        if (!ev && sweepPeak>=SWEEP_RATE) ev=lastSwingDir>0 ? SWEEP_LEFT : SWEEP_RIGHT;
        resetSwings();
      } else if (swings>1 && idle>SWING_GAP_MAX) resetSwings();
    }

    // Still: level and quiet for STILL_MS, reported once until you move again.
    bool quiet=gMag<STILL_GYRO && dynMag<STILL_DYN && fabsf(rollDeg)<STILL_TILT && fabsf(pitchDeg)<STILL_TILT;
    if (quiet) {
      if (!stillTiming) { stillTiming=true; stillSince=now; }
      else if (!stillFired && now-stillSince>=STILL_MS) { stillFired=true; if (!ev) ev=STILL; }
    } else { stillTiming=false; stillFired=false; }

    if (ev && ev!=STILL) {
      if ((int32_t)(now-quietUntil)<0) ev=NONE;      // too soon after the last action
      else quietUntil=now+COOLDOWN;
    }
    return ev;
  }

  float dynFwd=0, yawWorld=0;   // last values, for tuning output

  static const char *name(int ev) {
    switch (ev) {
      case DOUBLE_FLICK: return "DOUBLE FLICK";
      case SHAKE: return "SHAKE";
      case STILL: return "STILL";
      case PUNCH: return "PUNCH";
      case SWEEP_LEFT: return "SWEEP LEFT";
      case SWEEP_RIGHT: return "SWEEP RIGHT";
      default: return "-";
    }
  }

 private:
  void resetSwings() { swings=0; lastSwingDir=0; sweepPeak=0; }
  bool punchPending=false;
  float rotEnv=0, punchRotPeak=0;
  uint32_t punchStart=0;
  bool init=false, init2=false, flickArmed=true, stillTiming=false, stillFired=false, shakeLocked=false;
  float lp[3]={0,0,0}, lp2f=0, sweepPeak=0;
  int flickCount=0, swings=0, lastSwingDir=0;
  uint32_t firstFlick=0, lastPunch=0, lastSwingAt=0, stillSince=0, quietUntil=0;
};
