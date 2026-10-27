// Host test for glove/Gestures.h using SYNTHETIC IMU traces (100 Hz).
// It checks the rule logic (does a clean gesture fire, do tilts/steering/
// walking stay quiet). It cannot measure real-world accuracy: real wrists are
// messier than these traces, so thresholds must be tuned on the real glove.
//
// Build/run (any C++17 compiler):  c++ -std=c++17 -O1 tests/test_gestures.cpp -o test_gestures && ./test_gestures
#include "../firmware/glove/Gestures.h"
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static std::mt19937 rng(12345);
static float uni(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); }
static float gauss(float s) { return std::normal_distribution<float>(0, s)(rng); }

struct Trace {
  int n;
  std::vector<float> a[3], g[3], roll, pitch;
  explicit Trace(int ms) : n(ms / 10) {
    for (int i = 0; i < 3; ++i) { a[i].assign(n, 0); g[i].assign(n, 0); }
    roll.assign(n, 0); pitch.assign(n, 0);
  }
  void sine(std::vector<float> &v, int t0, int dur, float amp) {   // half-sine pulse
    for (int i = t0 / 10; i < (t0 + dur) / 10 && i < n; ++i)
      v[i] += amp * sinf((float)M_PI * (i - t0 / 10) / (dur / 10.0f));
  }
  void osc(std::vector<float> &v, int t0, int dur, float hz, float amp) {
    for (int i = t0 / 10; i < (t0 + dur) / 10 && i < n; ++i)
      v[i] += amp * sinf(2 * (float)M_PI * hz * (i - t0 / 10) / 100.0f);
  }
};

static void flick(Trace &t, int t0, float amp) {
  float s = uni(0, 1) < 0.5f ? 1.f : -1.f;
  t.sine(t.g[1], t0, 80, s * amp);
  t.sine(t.g[1], t0 + 90, 80, -0.5f * s * amp);      // wrist rebound
  t.sine(t.a[2], t0, 80, s * 6);                     // tangential/centripetal accel
  t.sine(t.a[0], t0, 80, 3);
}
static void punch(Trace &t, int t0, float amp) {
  t.sine(t.a[0], t0, 100, amp);
  t.sine(t.a[0], t0 + 110, 100, -0.6f * amp);
  t.sine(t.g[1], t0, 100, uni(-1.5f, 1.5f));
}
// Yaw is rotation about the real vertical. With the hand rolled by rollDeg, the
// vertical is spread over the hand's left and up axes (rollDeg=90: hand edge-on).
static void setRoll(Trace &t, float rollDeg) { for (int i = 0; i < t.n; ++i) t.roll[i] = rollDeg; }
static void shake(Trace &t, int t0, int dur, float hz, float amp, float rollDeg = 0) {
  setRoll(t, rollDeg);
  float r = rollDeg * (float)M_PI / 180;
  t.osc(t.g[1], t0, dur, hz, amp * sinf(r));
  t.osc(t.g[2], t0, dur, hz, amp * cosf(r));
  t.osc(t.a[1], t0, dur, hz, 0.8f * amp);
}
static void sweep(Trace &t, int t0, float sign, float amp, int dur, float rollDeg = 0) {
  setRoll(t, rollDeg);
  float r = rollDeg * (float)M_PI / 180;
  t.sine(t.g[1], t0, dur, sign * amp * sinf(r));
  t.sine(t.g[2], t0, dur, sign * amp * cosf(r));
  t.sine(t.g[1], t0 + dur, 100, -0.15f * sign * amp * sinf(r));
  t.sine(t.g[2], t0 + dur, 100, -0.15f * sign * amp * cosf(r));
  t.sine(t.a[1], t0, dur, sign * 3);
  t.sine(t.a[0], t0, dur, uni(6, 12));    // a real swat also pushes the hand forward (seen on the glove)
}

// Gravity in the hand frame for a given roll/pitch (degrees), plus sensor noise.
static void finish(Trace &t) {
  // A real hand cannot change tilt instantly: smooth roll/pitch (about 0.16 s time constant).
  for (int i = 1; i < t.n; ++i) {
    t.roll[i] = t.roll[i - 1] + 0.06f * (t.roll[i] - t.roll[i - 1]);
    t.pitch[i] = t.pitch[i - 1] + 0.06f * (t.pitch[i] - t.pitch[i - 1]);
  }
  for (int i = 0; i < t.n; ++i) {
    float r = t.roll[i] * (float)M_PI / 180, p = t.pitch[i] * (float)M_PI / 180;
    t.a[0][i] += 9.81f * sinf(p);
    t.a[1][i] += 9.81f * cosf(p) * sinf(r);
    t.a[2][i] += 9.81f * cosf(p) * cosf(r);
    for (int k = 0; k < 3; ++k) { t.a[k][i] += gauss(0.15f); t.g[k][i] += gauss(0.05f); }
  }
}

struct Result { std::vector<int> events; };
static Result run(Trace &t) {
  finish(t);
  GestureDetector d;
  Result res;
  for (int i = 0; i < t.n; ++i) {
    float a[3] = {t.a[0][i], t.a[1][i], t.a[2][i]}, g[3] = {t.g[0][i], t.g[1][i], t.g[2][i]};
    int ev = d.update(1000 + i * 10, a, g, t.roll[i], t.pitch[i]);
    if (ev) res.events.push_back(ev);
  }
  return res;
}
static int count(const Result &r, int ev) { int c = 0; for (int e : r.events) c += e == ev; return c; }
static int others(const Result &r, int ev, bool ignoreStill = true) {
  int c = 0; for (int e : r.events) c += e != ev && !(ignoreStill && e == GestureDetector::STILL); return c;
}

static int failures = 0;
static void check(bool ok, const char *what) { printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what); if (!ok) ++failures; }

// Fraction of randomised gesture trials that produce exactly the expected event.
template <class F>
static float rate(int ev, int trials, F make) {
  int good = 0;
  for (int i = 0; i < trials; ++i) {
    Trace t(4000);
    make(t);
    Result r = run(t);
    good += count(r, ev) == 1 && others(r, ev) == 0;
  }
  return 100.0f * good / trials;
}

int main() {
  using G = GestureDetector;
  printf("== Deterministic cases\n");
  { Trace t(4000); flick(t, 1000, 11); flick(t, 1500, 11); Result r = run(t);
    check(count(r, G::DOUBLE_FLICK) == 1 && others(r, G::DOUBLE_FLICK) == 0, "double flick (0.5 s apart) -> DOUBLE_FLICK once"); }
  { Trace t(4000); flick(t, 1000, 11); Result r = run(t);
    check(others(r, G::NONE) == 0, "single flick -> no action (a STILL 2 s later is expected)"); }
  { Trace t(5000); shake(t, 1000, 3000, 4, 7); Result r = run(t);
    check(count(r, G::SHAKE) == 1, "3 s of continuous shaking -> still only one SHAKE"); }
  { Trace t(5000); flick(t, 1000, 11); flick(t, 2800, 11); Result r = run(t);
    check(count(r, G::DOUBLE_FLICK) == 0, "two flicks 1.8 s apart -> nothing"); }
  { Trace t(4000); flick(t, 1000, 11); flick(t, 1130, 11); Result r = run(t);
    check(count(r, G::DOUBLE_FLICK) == 0, "snap + immediate rebound (130 ms) -> nothing"); }
  { Trace t(4000); punch(t, 1000, 10); Result r = run(t);
    check(count(r, G::PUNCH) == 1 && others(r, G::PUNCH) == 0, "punch -> PUNCH once"); }
  { Trace t(4000); shake(t, 1000, 1200, 4, 7); Result r = run(t);
    check(count(r, G::SHAKE) == 1 && count(r, G::SWEEP_LEFT) + count(r, G::SWEEP_RIGHT) == 0, "shake (4 Hz) -> SHAKE, no sweep"); }
  { Trace t(4000); sweep(t, 1000, +1, 6.5f, 200); Result r = run(t);
    check(count(r, G::SWEEP_LEFT) == 1 && others(r, G::SWEEP_LEFT) == 0, "sweep (yaw +) -> SWEEP_LEFT"); }
  { Trace t(4000); sweep(t, 1000, -1, 6.5f, 200); Result r = run(t);
    check(count(r, G::SWEEP_RIGHT) == 1 && others(r, G::SWEEP_RIGHT) == 0, "sweep (yaw -) -> SWEEP_RIGHT"); }
  { Trace t(4000); sweep(t, 1000, +1, 10, 200, 90); Result r = run(t);
    check(count(r, G::SWEEP_LEFT) == 1 && others(r, G::SWEEP_LEFT) == 0, "edge-on swat left (roll 90) -> SWEEP_LEFT, not a flick"); }
  { Trace t(4000); sweep(t, 1000, -1, 10, 200, -90); Result r = run(t);
    check(count(r, G::SWEEP_RIGHT) == 1 && others(r, G::SWEEP_RIGHT) == 0, "edge-on swat right (roll -90) -> SWEEP_RIGHT"); }
  { Trace t(6000); Result r = run(t);
    check(count(r, G::STILL) == 1, "3 s... level and still -> STILL fires once (not repeatedly)"); }
  { Trace t(6000); for (int i = 0; i < t.n; ++i) t.pitch[i] = 30; Result r = run(t);
    check(r.events.empty(), "steady 30 degree tilt (driving) -> no STILL"); }

  printf("== Randomised gesture trials (amplitude, timing, noise vary), exact-match rate\n");
  float flickRate = rate(G::DOUBLE_FLICK, 300, [](Trace &t) {
    int gap = (int)uni(300, 800); float a = uni(9, 16); flick(t, 1000, a); flick(t, 1000 + gap, a * uni(0.8f, 1.2f)); });
  auto pose = []() { static const float poses[] = {0, 0, 90, -90, 45, -45}; return poses[(int)uni(0, 6)]; };
  // Real jabs measured on the glove: 5-8 m/s^2 forward, rotation up to ~4.5 rad/s.
  float punchRate = rate(G::PUNCH, 300, [](Trace &t) { punch(t, 1000, uni(7.5f, 12)); });
  float shakeRate = rate(G::SHAKE, 300, [&](Trace &t) { shake(t, 1000, (int)uni(900, 1500), uni(3, 5), uni(6, 10), pose()); });
  float leftRate = rate(G::SWEEP_LEFT, 300, [&](Trace &t) { sweep(t, 1000, +1, uni(5.5f, 12), (int)uni(150, 300), pose()); });
  float rightRate = rate(G::SWEEP_RIGHT, 300, [&](Trace &t) { sweep(t, 1000, -1, uni(5.5f, 12), (int)uni(150, 300), pose()); });
  printf("  double flick %.1f%%  punch %.1f%%  shake %.1f%%  sweep left %.1f%%  sweep right %.1f%%\n",
         flickRate, punchRate, shakeRate, leftRate, rightRate);
  check(flickRate > 90 && punchRate > 90 && shakeRate > 90 && leftRate > 90 && rightRate > 90, "all gestures > 90% on synthetic traces");

  printf("== One hour of non-gesture motion (tilting, steering, walking, holds): must stay quiet\n");
  Trace hour(3600 * 1000);
  int t = 0, prevKind = -1;
  while (t < 3600 * 1000 - 8000) {
    int kind = (int)uni(0, 4), dur = (int)uni(2000, 6000);
    if (kind == 3 && prevKind == 3) kind = 0;     // two level holds in a row would exceed 2 s
    prevKind = kind;
    if (kind == 0) {            // fast roll/pitch tilting up to ~5 rad/s, like steering hard
      float ar = uni(10, 50), ap = uni(10, 40), hr = uni(0.4f, 1.0f), hp = uni(0.4f, 1.0f);
      for (int i = t / 10; i < (t + dur) / 10; ++i) {
        float s = (i - t / 10) / 100.0f;
        hour.roll[i] = ar * sinf(2 * (float)M_PI * hr * s); hour.pitch[i] = ap * sinf(2 * (float)M_PI * hp * s);
        hour.g[0][i] = ar * (float)M_PI / 180 * 2 * (float)M_PI * hr * cosf(2 * (float)M_PI * hr * s);
        hour.g[1][i] = ap * (float)M_PI / 180 * 2 * (float)M_PI * hp * cosf(2 * (float)M_PI * hp * s);
      }
    } else if (kind == 1) {     // steady tilt held (driving), small tremor
      float r = uni(15, 45), p = uni(-45, 45);
      for (int i = t / 10; i < (t + dur) / 10; ++i) { hour.roll[i] = r; hour.pitch[i] = p; }
      hour.osc(hour.g[0], t, dur, 8, 0.3f);
    } else if (kind == 2) {     // walking: 2 Hz bounce on every axis, tilted so STILL cannot fire
      for (int i = t / 10; i < (t + dur) / 10; ++i) hour.pitch[i] = 15;
      hour.osc(hour.a[0], t, dur, 2, 3); hour.osc(hour.a[2], t, dur, 2, 3);
      hour.osc(hour.g[0], t, dur, 2, 1); hour.osc(hour.g[1], t, dur, 2, 1); hour.osc(hour.g[2], t, dur, 2, 0.8f);
    } else {                    // level, short hold (well under the 2 s STILL time)
      dur = (int)uni(500, 1200);
    }
    t += dur;
  }
  for (int i = t / 10; i < hour.n; ++i) hour.pitch[i] = 30;   // the leftover tail is a held tilt, not a level rest
  Result r = run(hour);
  int falseActions = 0, stills = 0, byType[7] = {0};
  for (int e : r.events) { ++byType[e]; if (e == G::STILL) ++stills; else ++falseActions; }
  printf("  by type: double flick %d, shake %d, punch %d, sweep left %d, sweep right %d\n",
         byType[G::DOUBLE_FLICK], byType[G::SHAKE], byType[G::PUNCH], byType[G::SWEEP_LEFT], byType[G::SWEEP_RIGHT]);
  printf("  false action gestures in 1 h: %d, STILL events: %d\n", falseActions, stills);
  check(falseActions == 0, "no false action gestures from normal motion");
  check(stills == 0, "no STILL while moving or holding < 2 s");

  printf("%s (%d failure(s))\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures);
  return failures ? 1 : 0;
}
