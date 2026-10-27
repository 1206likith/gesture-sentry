#pragma once
#include <Wire.h>
#include <Adafruit_Sensor.h>

// MPU6500: WHO_AM_I=0x70. Separate initialization from the MPU6050 driver.
class Mpu6500 {
 public:
  bool begin(uint8_t address) {
    address_=address;
    uint8_t id;
    if (!read(0x75,&id,1) || id!=0x70) return false;
    if (!write(0x6B,0x80)) return false; // device reset
    delay(100);
    if (!write(0x6B,0x01)) return false; // wake, automatic PLL clock
    delay(100);
    if (!write(0x6C,0x00) || !write(0x1A,0x02) || // all axes, gyro DLPF 92Hz
        !write(0x19,0x09) || !write(0x1B,0x10) || // 100Hz, +/-1000 deg/s
        !write(0x1C,0x08) || !write(0x1D,0x02)) return false; // +/-4g, accel DLPF 92Hz
    uint8_t range;
    return read(0x1C,&range,1) && (range&0x18)==0x08;
  }

  bool getEvent(sensors_event_t *a,sensors_event_t *g,sensors_event_t *t) {
    uint8_t data[14];
    if (!read(0x3B,data,sizeof(data))) return false;
    *a={}; *g={}; *t={};
    a->acceleration.x=word(data)*9.80665f/8192;
    a->acceleration.y=word(data+2)*9.80665f/8192;
    a->acceleration.z=word(data+4)*9.80665f/8192;
    t->temperature=word(data+6)/333.87f+21.0f;
    constexpr float gyroScale=0.01745329252f/32.8f;   // +/-1000 deg/s
    g->gyro.x=word(data+8)*gyroScale;
    g->gyro.y=word(data+10)*gyroScale;
    g->gyro.z=word(data+12)*gyroScale;
    // All-zero acceleration is not a usable gravity orientation sample.
    return a->acceleration.x!=0 || a->acceleration.y!=0 || a->acceleration.z!=0;
  }

 private:
  uint8_t address_=0x68;
  static int16_t word(const uint8_t *p) { return (int16_t)((uint16_t)p[0]<<8 | p[1]); }
  bool write(uint8_t reg,uint8_t value) {
    Wire.beginTransmission(address_); Wire.write(reg); Wire.write(value);
    return Wire.endTransmission()==0;
  }
  bool read(uint8_t reg,uint8_t *buffer,uint8_t length) {
    Wire.beginTransmission(address_); Wire.write(reg);
    if (Wire.endTransmission(false)!=0) return false;
    if (Wire.requestFrom(address_,length)!=length) return false;
    for (int i=0;i<length;++i) buffer[i]=Wire.read();
    return true;
  }
};
