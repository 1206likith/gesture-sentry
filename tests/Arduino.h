#pragma once
#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <initializer_list>
using std::max;
using std::abs;
template<class T> T constrain(T x,T lo,T hi) { return std::clamp(x,lo,hi); }
inline uint32_t fakeNow=1;
inline int pins[40]={}, duty[40]={};
inline uint32_t millis() { return fakeNow; }
constexpr int OUTPUT=1,LOW=0,HIGH=1,pdPASS=1;
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
#define pdMS_TO_TICKS(x) (x)
inline void pinMode(int,int) {}
inline void digitalWrite(int p,int x) { pins[p]=x; }
inline bool ledcAttachChannel(int,int,int,int) { return true; }
inline void ledcWrite(int p,int x) { duty[p]=x; }
inline void (*savedTask)(void*)=nullptr;
inline void *savedArgument=nullptr;
inline int xTaskCreate(void(*fn)(void*),const char*,int,void *arg,int,void*) {
  savedTask=fn; savedArgument=arg; return pdPASS;
}
struct OneIteration {};
inline void vTaskDelay(int) { throw OneIteration{}; }
inline void tick() { try { savedTask(savedArgument); } catch (OneIteration&) {} }
