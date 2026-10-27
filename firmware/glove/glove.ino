#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <math.h>
#include <Preferences.h>
#include "Mpu6500.h"
#include "Gestures.h"

GestureDetector gestures;
uint32_t eventSeq = 0, lastPacketAt = 0;
int eventCode = 0;
float pkPr = 0, pkYr = 0, pkDf = 0;
uint32_t lastPeak = 0;

const char *SERVICE = "60184255-b28e-4200-a001-000000000001";
const char *DATA = "60184255-b28e-4200-a001-000000000002";
Adafruit_MPU6050 mpu;
Mpu6500 mpu6500;
bool using6500=false;
BLECharacteristic *dataChar;
BLEServer *server;
volatile bool restartAdvertising = false;
volatile bool peerConnected = false;
uint32_t lastStatus = 0;
uint8_t sensorAddress = 0x68;
float zeroRoll = 0, zeroPitch = 0, roll = 0, pitch = 0;
uint32_t sequence = 0;

class Callbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *) override { peerConnected=true; }
  void onDisconnect(BLEServer *) override { peerConnected=false; restartAdvertising = true; }
};

// Hand frame in sensor coordinates, learned by the 'o' command so the board
// can sit in any orientation: forward (fingers), left, up (flat hand).
Preferences prefs;
bool haveFrame=false;
float fwd[3], lft[3], upv[3];

static float dot3(const float *a, const float *b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
static bool unit3(float *v) {
  float n=sqrtf(dot3(v,v));
  if (!(n>0.001f)) return false;
  for (int i=0;i<3;++i) v[i]/=n;
  return true;
}

// One sensor read: roll/pitch from gravity, plus accel and gyro in the hand
// frame (fingers, left, up) for the gesture detector. Hand-frame values are
// zero until an orientation has been learned with 'o' (or 'f' then 'd').
bool readHand(float *ah, float *gh, float &r, float &p) {
  sensors_event_t a, g, t;
  bool valid=using6500 ? mpu6500.getEvent(&a,&g,&t) : mpu.getEvent(&a,&g,&t);
  if (!valid) return false;
  if (haveFrame) {
    float v[3]={a.acceleration.x,a.acceleration.y,a.acceleration.z};
    float w[3]={g.gyro.x,g.gyro.y,g.gyro.z};
    ah[0]=dot3(v,fwd); ah[1]=dot3(v,lft); ah[2]=dot3(v,upv);
    gh[0]=dot3(w,fwd); gh[1]=dot3(w,lft); gh[2]=dot3(w,upv);
    // Rolling right lifts the left side: positive roll. Fingers down: negative pitch.
    r = atan2f(ah[1], ah[2]) * 180.0f / PI;
    p = atan2f(ah[0], sqrtf(ah[1]*ah[1]+ah[2]*ah[2])) * 180.0f / PI;
    return isfinite(r) && isfinite(p);
  }
  for (int i=0;i<3;++i) ah[i]=gh[i]=0;
  r = atan2f(a.acceleration.y, a.acceleration.z) * 180.0f / PI;
  p = atan2f(-a.acceleration.x,
       sqrtf(a.acceleration.y*a.acceleration.y + a.acceleration.z*a.acceleration.z)) * 180.0f / PI;
  return isfinite(r) && isfinite(p);
}

bool angles(float &r, float &p) {
  float a[3], g[3];
  return readHand(a,g,r,p);
}

bool sampleGravity(float *out) {
  float sum[3]={0,0,0};
  int n=0;
  for (int i=0;i<100;++i) {
    sensors_event_t a, g, t;
    bool valid=using6500 ? mpu6500.getEvent(&a,&g,&t) : mpu.getEvent(&a,&g,&t);
    if (valid) { sum[0]+=a.acceleration.x; sum[1]+=a.acceleration.y; sum[2]+=a.acceleration.z; ++n; }
    delay(10);
  }
  if (n<50) return false;
  for (int i=0;i<3;++i) out[i]=sum[i]/n;
  return true;
}

void waitSeconds(int s) {
  for (int i=s;i>0;--i) { Serial.printf("  %d...\n",i); delay(1000); }
}

// Wait until the sensor is held steady (0.6 s with little change) and, if ref is
// given, tilted at least minAngle degrees away from it. Returns the mean vector.
// The onboard LED (GPIO2) gives the cues: blinking = hold flat and still,
// solid on = tip fingers down and hold.
void flashLed(int times, int ms) {
  for (int i=0;i<times;++i) { digitalWrite(2,HIGH); delay(ms); digitalWrite(2,LOW); delay(ms); }
}

bool waitStable(float *out, const float *ref, float minAngle, uint32_t timeoutMs) {
  uint32_t start=millis(), lastMsg=0;
  while (millis()-start<timeoutMs) {
    digitalWrite(2, ref ? HIGH : !digitalRead(2));
    float lo[3]={1e9f,1e9f,1e9f}, hi[3]={-1e9f,-1e9f,-1e9f}, sum[3]={0,0,0};
    int n=0;
    for (int i=0;i<30;++i) {
      sensors_event_t a, g, t;
      bool valid=using6500 ? mpu6500.getEvent(&a,&g,&t) : mpu.getEvent(&a,&g,&t);
      if (valid) {
        float v[3]={a.acceleration.x,a.acceleration.y,a.acceleration.z};
        for (int k=0;k<3;++k) { lo[k]=min(lo[k],v[k]); hi[k]=max(hi[k],v[k]); sum[k]+=v[k]; }
        ++n;
      }
      delay(20);
    }
    if (n<20) continue;
    float mean[3]={sum[0]/n,sum[1]/n,sum[2]/n};
    float mag=sqrtf(dot3(mean,mean));
    bool steady=hi[0]-lo[0]<0.6f && hi[1]-lo[1]<0.6f && hi[2]-lo[2]<0.6f && mag>8.0f && mag<11.5f;
    float angle=0;
    if (ref) {
      float c=dot3(mean,ref)/mag/sqrtf(dot3(ref,ref));
      angle=acosf(constrain(c,-1.0f,1.0f))*180.0f/PI;
    }
    if (millis()-lastMsg>2000) {
      lastMsg=millis();
      if (ref) Serial.printf("  tilt from flat=%.0f deg (need >=%.0f) %s\n",angle,minAngle,steady?"steady":"moving");
      else Serial.printf("  %s\n",steady?"steady":"moving - hold still");
    }
    if (steady && angle>=minAngle) { for (int k=0;k<3;++k) out[k]=mean[k]; return true; }
  }
  return false;
}

// 'o': learn the hand frame from two poses, then store it in flash.
// phase 0 = both poses in one run ('o'); 1 = capture the flat pose only ('f');
// 2 = capture the fingers-down pose using the stored flat pose ('d').
float storedFlat[3];
bool haveStoredFlat=false;

bool orientOk=false;

void orient(int phase=0) {
  float flat[3], down[3];
  orientOk=false;
  pinMode(2,OUTPUT);
  if (phase!=2) {
    Serial.println("ORIENT step 1: hold your hand FLAT and STILL (palm down, as when worn)... LED blinking");
    if (!waitStable(flat,nullptr,0,phase==1 ? 15000 : 60000) || !unit3(flat)) { digitalWrite(2,LOW); Serial.println("ORIENT failed: no steady flat pose."); return; }
    digitalWrite(2,LOW); delay(500); flashLed(3,150);
    for (int i=0;i<3;++i) storedFlat[i]=flat[i];
    haveStoredFlat=true;
    Serial.println("ORIENT flat pose captured (3 flashes).");
    if (phase==1) return;
  } else {
    if (!haveStoredFlat) { Serial.println("ORIENT failed: capture the flat pose first (f)."); return; }
    for (int i=0;i<3;++i) flat[i]=storedFlat[i];
  }
  Serial.println("ORIENT step 2: LED solid - tip FINGERS DOWN (30-60 degrees) and hold still...");
  if (!waitStable(down,flat,25,phase==2 ? 15000 : 60000)) { digitalWrite(2,LOW); Serial.println("ORIENT failed: no steady tipped pose."); return; }
  digitalWrite(2,LOW); flashLed(5,80);
  float along=dot3(down,flat);
  float f[3]; for (int i=0;i<3;++i) f[i]=-(down[i]-along*flat[i]);
  float mag=sqrtf(dot3(f,f)), total=sqrtf(dot3(down,down));
  if (total<1 || mag<0.25f*total || !unit3(f)) {
    Serial.println("ORIENT failed: the tip was too small or you moved. Try again and tip further.");
    return;
  }
  // left = up x forward
  float l[3]={flat[1]*f[2]-flat[2]*f[1], flat[2]*f[0]-flat[0]*f[2], flat[0]*f[1]-flat[1]*f[0]};
  if (!unit3(l)) { Serial.println("ORIENT failed: degenerate axes."); return; }
  for (int i=0;i<3;++i) { fwd[i]=f[i]; lft[i]=l[i]; upv[i]=flat[i]; }
  haveFrame=true;
  float frame[9]={fwd[0],fwd[1],fwd[2],lft[0],lft[1],lft[2],upv[0],upv[1],upv[2]};
  prefs.putBytes("frame",frame,sizeof(frame));
  Serial.printf("ORIENT saved. fwd=(%.2f,%.2f,%.2f) left=(%.2f,%.2f,%.2f) up=(%.2f,%.2f,%.2f)\n",
    fwd[0],fwd[1],fwd[2],lft[0],lft[1],lft[2],upv[0],upv[1],upv[2]);
  Serial.println("ORIENT: return your hand FLAT and still; recalibrating in:");
  waitSeconds(4);
  orientOk=true;
}

void calibrate() {
  Serial.println("Hold glove flat and still for calibration.");
  float sumR = 0, sumP = 0;
  int samples = 0;
  for (int i = 0; i < 100; ++i) {
    float r, p;
    if (angles(r, p)) { sumR += r; sumP += p; ++samples; }
    delay(10);
  }
  if (samples) { zeroRoll = sumR/samples; zeroPitch = sumP/samples; }
  roll = pitch = 0;
  Serial.printf("Calibration samples=%d neutralRoll=%.2f neutralPitch=%.2f\n",samples,zeroRoll,zeroPitch);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.setTimeOut(25);
  bool sensorReady=false;
  while (!sensorReady) {
    for (uint8_t address : {0x68,0x69}) {
      sensorAddress=address;
      if (mpu.begin(address)) { sensorReady=true; break; }
      if (mpu6500.begin(address)) { using6500=true; sensorReady=true; break; }
    }
    if (!sensorReady) {
      Serial.printf("GLOVE I2C scan SDAlevel=%d SCLlevel=%d\n",digitalRead(21),digitalRead(22));
      int found=0;
      for (uint8_t address=1;address<127;++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission()==0) {
          ++found; Serial.printf("GLOVE I2C ACK address=0x%02X\n",address);
          if (address==0x68 || address==0x69) {
            Wire.beginTransmission(address); Wire.write(0x75);
            if (Wire.endTransmission(false)==0 && Wire.requestFrom(address,(uint8_t)1)==1)
              Serial.printf("GLOVE WHO_AM_I=0x%02X (MPU6050 expected 0x68)\n",Wire.read());
          }
        }
      }
      Serial.printf("GLOVE I2C devices=%d; MPU6050/MPU6500 init failed; retrying in 3 seconds\n",found);
      delay(3000);
    }
  }
  Serial.printf("%s detected at 0x%02X\n",using6500?"MPU6500":"MPU6050",sensorAddress);
  if (!using6500) {
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    // Wide gyro range and filter so fast wrist snaps are not clipped or smoothed away.
    mpu.setGyroRange(MPU6050_RANGE_1000_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
  }
  prefs.begin("glove",false);
  float frame[9];
  if (prefs.getBytes("frame",frame,sizeof(frame))==sizeof(frame)) {
    for (int i=0;i<3;++i) { fwd[i]=frame[i]; lft[i]=frame[3+i]; upv[i]=frame[6+i]; }
    haveFrame=true;
  }
  Serial.printf("Orientation frame: %s\n",haveFrame ? "saved (from 'o')" : "default axes; send o to learn how the board sits");
  calibrate();
  BLEDevice::init("GestureGlove");
  Serial.printf("Glove BLE address: %s\n",BLEDevice::getAddress().toString().c_str());
  server = BLEDevice::createServer();
  server->setCallbacks(new Callbacks());
  BLEService *service = server->createService(SERVICE);
  dataChar = service->createCharacteristic(DATA, BLECharacteristic::PROPERTY_READ);
  dataChar->setValue("0,90,90");
  service->start();
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE);
  BLEDevice::getAdvertising()->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("Ready. c=recalibrate neutral, o=learn board orientation, r=reset orientation.");
}

void loop() {
  if (restartAdvertising) {
    restartAdvertising = false;
    BLEDevice::startAdvertising();
  }
  if (Serial.available()) {
    char command=Serial.read();
    if (command=='c') calibrate();
    else if (command=='o') { orient(); if (orientOk) calibrate(); }
    else if (command=='f') orient(1);
    else if (command=='d') { orient(2); if (orientOk) calibrate(); }
    else if (command=='r') { prefs.remove("frame"); haveFrame=false; Serial.println("Orientation reset to default axes."); calibrate(); }
  }
  float r, p, ah[3], gh[3];
  if (readHand(ah, gh, r, p)) {
    uint32_t now = millis();
    if (haveFrame) {   // gestures need the learned hand frame
      int ev = gestures.update(now, ah, gh, roll, pitch);
      // Tuning aid: peak pitch rate, yaw rate and forward accel; printed only during motion.
      pkPr=max(pkPr,fabsf(gh[1])); pkYr=max(pkYr,fabsf(gestures.yawWorld)); pkDf=max(pkDf,gestures.dynFwd);
      if (now-lastPeak>=250) {
        lastPeak=now;
        if (pkPr>2.0f || pkYr>1.5f || pkDf>5.0f)
          Serial.printf("GLOVE peak pitchRate=%.1f yawRate=%.1f fwdAcc=%.1f\n",pkPr,pkYr,pkDf);
        pkPr=pkYr=pkDf=0;
      }
      if (ev) {
        eventCode = ev; ++eventSeq;
        Serial.printf("GLOVE gesture=%s evSeq=%lu\n", GestureDetector::name(ev), (unsigned long)eventSeq);
      }
    }
    // The sensor is read every 10 ms for gestures; angles go out every 40 ms.
    if (now-lastPacketAt >= 40) {
      lastPacketAt = now;
      float delta = r-zeroRoll;
      while (delta > 180) delta -= 360;
      while (delta < -180) delta += 360;
      roll += 0.2f*(delta-roll);
      pitch += 0.2f*(p-zeroPitch-pitch);
      // Roll steers pan; pitch steers tilt. Invert either sign if necessary.
      int pan = constrain((int)lroundf(90 + roll), 30, 150);
      int tilt = constrain((int)lroundf(90 + pitch), 45, 135);
      char packet[40];
      snprintf(packet, sizeof(packet), "%lu,%d,%d,%lu,%d", (unsigned long)++sequence, pan, tilt,
               (unsigned long)eventSeq, eventCode);
      dataChar->setValue(packet);
      if (now-lastStatus>=500) {
        lastStatus=now;
        Serial.printf("GLOVE sensor=OK seq=%lu roll=%.1f pitch=%.1f pan=%d tilt=%d BLE=%s\n",
          (unsigned long)sequence,roll,pitch,pan,tilt,peerConnected?"CONNECTED":"ADVERTISING");
      }
    }
  } else if (millis()-lastStatus>=1000) {
    lastStatus=millis(); Serial.println("GLOVE sensor read failed; command sequence not updated");
  }
  delay(10);
}
