#include <Arduino.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <WebServer.h>
#include "WheelDrive.h"
#include "web_ui.h"

const char *SERVICE = "60184255-b28e-4200-a001-000000000001";
const char *DATA = "60184255-b28e-4200-a001-000000000002";
constexpr int PAN=13, TILT=27, TRIG=5, ECHO=18, PIR=19;
constexpr int LASER=23, BUZZER=25, RED=32, GREEN=33, BLUE=4;
// These outputs drive module logic inputs or transistor drivers, never loads.
constexpr bool RGB_COMMON_ANODE = false;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr bool PASSIVE_BUZZER = false; // true: tone(); false: active buzzer
Servo panServo, tiltServo;
WheelDrive wheels;
// Who drives the wheels: the glove over BLE, or the web page over WiFi.
enum Source { SRC_GLOVE, SRC_WEB };
Source source=SRC_GLOVE;
WebServer server(80);
// Phone and laptop join this network, then open http://192.168.4.1
const char *AP_SSID="GestureSentry";
const char *AP_PASS="sentry-drive";  // WPA2 needs at least 8 characters
int glovePan=90, gloveTilt=90;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool oledOK=false, motion=false, nearObject=false;
BLEClient *client;
BLERemoteCharacteristic *remoteData=nullptr;
int pan=90, tilt=90, targetPan=90, targetTilt=90;  // servos stay centered; only the hardware test moves them
float distanceCm=-1;
uint32_t lastPacket=0, lastSequence=0, laserUntil=0;
uint32_t lastRead=0, lastServo=0, lastSense=0, lastDisplay=0, lastScan=0;
bool haveSequence=false, laserOn=false, wasEnabled=false;
uint32_t beepUntil=0;
bool diagnostic=false;
uint32_t diagnosticStarted=0;
int diagnosticPhase=-1;

void rgb(bool r, bool g, bool b) {
  digitalWrite(RED, r != RGB_COMMON_ANODE);
  digitalWrite(GREEN, g != RGB_COMMON_ANODE);
  digitalWrite(BLUE, b != RGB_COMMON_ANODE);
}
void buzzer(bool on) {
  if (PASSIVE_BUZZER) { if (on) tone(BUZZER, 2000); else noTone(BUZZER); }
  else digitalWrite(BUZZER, on);
}
void laserOff() { digitalWrite(LASER, LOW); laserOn=false; }
bool fresh() {
  return client && client->isConnected() && haveSequence && millis()-lastPacket < 300;
}

void connectGlove() {
  laserOff();
  wheels.update(false,90,90);
  buzzer(false);
  remoteData=nullptr;
  haveSequence=false;
  BLEScan *scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  // Synchronous scan: outputs remain off; servos hold during reconnection.
  auto results = scan->start(1, false);
  (void)results;
  BLEScanResults *found = scan->getResults();
  for (int i=0; i<found->getCount(); ++i) {
    BLEAdvertisedDevice device = found->getDevice(i);
    if (!device.haveServiceUUID() || !device.isAdvertisingService(BLEUUID(SERVICE))) continue;
    if (client->connect(&device)) {
      BLERemoteService *service = client->getService(SERVICE);
      if (service) remoteData = service->getCharacteristic(DATA);
      if (!remoteData || !remoteData->canRead()) {
        remoteData=nullptr;
        client->disconnect();
      }
    }
    break;
  }
  scan->clearResults();
}

void readGlove() {
  if (!client->isConnected() || !remoteData) return;
  auto value = remoteData->readValue();
  unsigned long seq, evSeq=0;
  int p,t, ev=0, consumed=0, more=0;
  if (sscanf(value.c_str(), "%lu,%d,%d%n", &seq,&p,&t,&consumed) != 3 ||
      p<30 || p>150 || t<45 || t>135) return;
  if (consumed != (int)value.length()) {   // glove firmware appends ",evSeq,evCode"; validated, then ignored
    if (sscanf(value.c_str()+consumed, ",%lu,%d%n", &evSeq,&ev,&more) != 2 ||
        consumed+more != (int)value.length() || ev<0 || ev>6) return;
  }
  if (!haveSequence || seq != lastSequence) {
    lastSequence=seq; haveSequence=true; lastPacket=millis();
    glovePan=p; gloveTilt=t;
  }
}

// Switching source stops both paths. Wheels stay disabled until 'd' enables glove drive.
void setSource(Source next) {
  laserOff(); buzzer(false);
  wheels.stopManual();
  wheels.mode(false);
  haveSequence=false;
  source=next;
}

void sendStatus() {
  char body[160];
  snprintf(body,sizeof(body),
    "{\"source\":\"%s\",\"link\":%d,\"command\":\"%s\",\"armed\":%d,\"cm\":%.1f}",
    source==SRC_WEB ? "web" : "glove", client->isConnected() ? 1 : 0,
    WheelDrive::name(wheels.current()), wheels.isArmed() ? 1 : 0, distanceCm);
  server.send(200,"application/json",body);
}

void handleSource() {
  String value=server.arg("v");
  if (value=="web") setSource(SRC_WEB);
  else if (value=="glove") setSource(SRC_GLOVE);
  else { server.send(400,"text/plain","v must be glove or web"); return; }
  sendStatus();
}

void handleDrive() {
  if (source!=SRC_WEB) { server.send(409,"text/plain","web control not selected"); return; }
  int throttle=constrain(server.arg("t").toInt(),-100,100);
  int steer=constrain(server.arg("s").toInt(),-100,100);
  wheels.setManual(throttle,steer);
  server.send(200,"text/plain","ok");
}

void sense() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10); digitalWrite(TRIG, LOW);
  unsigned long duration = pulseIn(ECHO, HIGH, 25000);
  distanceCm = duration ? duration * 0.0343f / 2 : -1;
  // HC-SR501 needs about 60 seconds to settle after power-up.
  motion = millis()>60000 && digitalRead(PIR);
  nearObject = distanceCm>0 && distanceCm<30;
}

void scanBus() {
  Serial.println("I2C scan (turret SDA21/SCL22):");
  int count=0;
  for (uint8_t address=1;address<127;++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission()==0) {
      Serial.printf("I2C ACK 0x%02X\n",address); ++count;
    }
  }
  Serial.printf("I2C devices=%d OLED buffer=%d servoPanHz=%lu servoTiltHz=%lu wheelAHz=%lu wheelBHz=%lu\n",
    count,oledOK,(unsigned long)ledcReadFreq(PAN),(unsigned long)ledcReadFreq(TILT),(unsigned long)ledcReadFreq(WheelDrive::ENA),(unsigned long)ledcReadFreq(WheelDrive::ENB));
  Serial.printf("I2C diagnostic uptime=%lu PIRraw=%d PIRwarmup=%d servoAttached=%d/%d wheelDuty=%lu/%lu\n",
    (unsigned long)millis(),digitalRead(PIR),millis()<60000,panServo.attached(),tiltServo.attached(),
    (unsigned long)ledcRead(WheelDrive::ENA),(unsigned long)ledcRead(WheelDrive::ENB));
  Serial.println("ACK/configuration does not prove visible display or physical movement.");
}

void diagnosticTick(uint32_t now) {
  int phase=(now-diagnosticStarted)/1000;
  if (phase==diagnosticPhase) return;
  diagnosticPhase=phase;
  buzzer(false); rgb(false,false,false);
  switch (phase) {
    case 0: Serial.println("TEST RGB RED + OLED text"); rgb(true,false,false); break;
    case 1: Serial.println("TEST RGB GREEN"); rgb(false,true,false); break;
    case 2: Serial.println("TEST RGB BLUE"); rgb(false,false,true); break;
    case 3: Serial.println("TEST BUZZER 150ms"); break;
    case 4: Serial.println("TEST PAN -> 80 degrees"); targetPan=80; break;
    case 5: Serial.println("TEST PAN -> 100 degrees"); targetPan=100; break;
    case 6: Serial.println("TEST PAN -> 90 degrees"); targetPan=90; break;
    case 7: Serial.println("TEST TILT -> 80 degrees"); targetTilt=80; break;
    case 8: Serial.println("TEST TILT -> 100 degrees"); targetTilt=100; break;
    case 9: Serial.println("TEST TILT -> 90 degrees"); targetTilt=90; break;
    default:
      diagnostic=false; targetPan=targetTilt=90;
      Serial.println("TEST sequence complete; wheels disabled, laser off. Confirm visible/light/sound responses."); break;
  }
}

void commands() {
  while (Serial.available()) {
    char command=Serial.read();
    switch (command) {
      case 'U': case 'V': case 'J': case 'K': {
        diagnostic=false; laserOff(); wheels.mode(false);
        int l=command=='U' ? 180 : command=='J' ? -180 : 0;
        int r=command=='V' ? 180 : command=='K' ? -180 : 0;
        Serial.printf("TEST WHEEL %s %s 1500ms duty180 accepted=%d\n",
          l ? "LEFT":"RIGHT", (l+r)>0 ? "forward":"reverse", wheels.testPulse(l,r,1500));
        break;
      }
      case 'p':
        Serial.printf("WHEEL pins IN1=%d IN2=%d IN3=%d IN4=%d ENA=%lu ENB=%lu armed=%d\n",
          digitalRead(WheelDrive::IN1),digitalRead(WheelDrive::IN2),digitalRead(WheelDrive::IN3),digitalRead(WheelDrive::IN4),
          (unsigned long)ledcRead(WheelDrive::ENA),(unsigned long)ledcRead(WheelDrive::ENB),wheels.isArmed());
        break;
      case 'u':
        diagnostic=false; laserOff(); wheels.mode(false);
        Serial.println(wheels.testPulse(80,0) ? "TEST WHEEL LEFT forward 200ms duty80" : "TEST WHEEL FAILED: PWM unavailable");
        break;
      case 'v':
        diagnostic=false; laserOff(); wheels.mode(false);
        Serial.println(wheels.testPulse(0,80) ? "TEST WHEEL RIGHT forward 200ms duty80" : "TEST WHEEL FAILED: PWM unavailable");
        break;
      case 'i': scanBus(); break;
      case 't':
        wheels.mode(false); laserOff();
        diagnostic=true;
        diagnosticStarted=millis(); diagnosticPhase=-1;
        Serial.println("TEST starting: RGB/OLED/buzzer/servos; wheels and laser remain OFF");
        scanBus(); diagnosticStarted=millis(); break;
      case 's':
      case 'h': diagnostic=false; buzzer(false); wheels.mode(false); laserOff(); break;
      case 'd': diagnostic=false; wheels.mode(true); laserOff(); break;   // enable glove drive; the car still needs a HOLD pose to arm
      case 'l': // Manual one-second pulse, only with a live glove link.
        if (fresh()) {
          laserUntil=millis()+1000; laserOn=true; digitalWrite(LASER,HIGH);
        }
        break;
      case '0': laserOff(); break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  for (int pin : {TRIG,LASER,BUZZER,RED,GREEN,BLUE}) pinMode(pin, OUTPUT);
  pinMode(ECHO,INPUT); pinMode(PIR,INPUT);
  laserOff(); buzzer(false); rgb(false,false,true);
  Wire.begin(21,22);
  Wire.setTimeOut(25);
  oledOK=display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  panServo.setPeriodHertz(50); tiltServo.setPeriodHertz(50);
  // Conservative pulse range; calibrate to your servo and mechanical limits.
  panServo.attach(PAN,1000,2000); tiltServo.attach(TILT,1000,2000);
  panServo.write(pan); tiltServo.write(tilt);
  Serial.println(wheels.begin() ? "Wheel PWM ready (L298N); wheels disarmed" : "Wheel PWM initialization FAILED; drive disabled");
  BLEDevice::init("GestureTurret");
  client=BLEDevice::createClient();
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);   // no modem sleep: lower latency for drive commands
  WiFi.softAP(AP_SSID,AP_PASS);
  server.on("/", []() { server.send_P(200,"text/html",WEB_UI); });
  server.on("/api/status", sendStatus);
  server.on("/api/source", handleSource);
  server.on("/api/drive", handleDrive);
  server.begin();
  Serial.printf("WEB control: join WiFi '%s' (password %s) and open http://%s\n",AP_SSID,AP_PASS,WiFi.softAPIP().toString().c_str());
  Serial.println("d=enable glove drive s/h=stop+disable l=laser 1s 0=laser off i=I2C scan t=hardware test");
}

void loop() {
  commands();
  server.handleClient();
  uint32_t now=millis();
  if (laserOn && (!fresh() || (int32_t)(now-laserUntil)>=0)) laserOff();
  // In web mode the glove is ignored: no scanning (the scan blocks for a second) and no reads.
  if (!diagnostic && source==SRC_GLOVE && !client->isConnected() && now-lastScan>=2000) {
    lastScan=now; connectGlove(); now=millis();
  }
  if (!diagnostic && source==SRC_GLOVE && now-lastRead>=50) { lastRead=now; readGlove(); now=millis(); }
  if (now-lastSense>=150) { lastSense=now; sense(); now=millis(); }
  bool live=fresh();
  wheels.update(live && source==SRC_GLOVE,glovePan,gloveTilt);
  if (!live) laserOff();
  if (diagnostic) diagnosticTick(now);
  if (now-lastServo>=20) {
    lastServo=now;
    if (pan<targetPan) ++pan; else if (pan>targetPan) --pan;
    if (tilt<targetTilt) ++tilt; else if (tilt>targetTilt) --tilt;
    panServo.write(pan); tiltServo.write(tilt);
  }
  bool alert=motion || nearObject;
  if (diagnostic) buzzer(diagnosticPhase==3 && (now-diagnosticStarted)%1000<150);
  else {
    // Long beep when the glove turns drive on.
    bool driveOn=wheels.isEnabled();
    if (driveOn && !wasEnabled) beepUntil=now+400;
    wasEnabled=driveOn;
    buzzer((alert && (now%1000)<100) || (int32_t)(beepUntil-now)>0);
    rgb(alert, live && !alert, !live && !alert);
  }
  if (now-lastDisplay>=250) {
    lastDisplay=now;
    if (oledOK) {
      display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1); display.setCursor(0,0);
      if (diagnostic) { display.println("HARDWARE TEST"); display.printf("Step: %d/10\n",diagnosticPhase+1); }
      else if (source==SRC_WEB) display.println("WEB CONTROL");
      else if (!live) display.println("GLOVE LINK LOST");
      else if (!wheels.isEnabled()) display.println("DRIVE OFF: roll R 1.5s");
      else display.println(wheels.isArmed() ? WheelDrive::name(wheels.current()) : "ARM: HOLD FLAT");
      display.printf("Glove: %s\n",source==SRC_WEB ? "standby" : live ? "CONNECTED" : "no link");
      if (distanceCm<0) display.println("Range: no echo");
      else display.printf("Range: %.1f cm\n",distanceCm);
      display.printf("PIR:%s Near:%s\n",motion?"YES":"no",nearObject?"YES":"no");
      display.printf("Laser:%s\n",laserOn?"ON":"OFF");
      display.display();
    }
    Serial.printf("link=%d cmd=%s enabled=%d armed=%d cm=%.1f PIR=%d\n",live,WheelDrive::name(wheels.current()),wheels.isEnabled(),wheels.isArmed(),distanceCm,motion);
  }
  delay(2);
}
