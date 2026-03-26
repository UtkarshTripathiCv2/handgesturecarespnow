#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>

#define MPU 0x68
#define CHANNEL 6
#define DEADZONE 8000 
#define HEARTBEAT_INTERVAL 50 

uint8_t peerAddress[] = {0x80, 0xF3, 0xDA, 0x63, 0x2F, 0x00}; // Ensure this is your RX MAC

struct CommandData { char action; };
CommandData cmd;
char lastSent = 'S';
unsigned long lastSendTime = 0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) ESP.restart();

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = CHANNEL;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Wire.begin(21, 22);
  Wire.beginTransmission(MPU);
  Wire.write(0x6B); Wire.write(0x00);
  Wire.endTransmission(true);
}

void loop() {
  int16_t ax, ay;
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 4, true);

  if (Wire.available() >= 4) {
    ax = (Wire.read() << 8) | Wire.read();
    ay = (Wire.read() << 8) | Wire.read();

    if (ax < -DEADZONE)      cmd.action = 'F'; 
    else if (ax > DEADZONE)  cmd.action = 'B'; 
    else if (ay > DEADZONE)  cmd.action = 'L'; 
    else if (ay < -DEADZONE) cmd.action = 'R'; 
    else                     cmd.action = 'S'; 

    unsigned long currentTime = millis();
    if (currentTime - lastSendTime > HEARTBEAT_INTERVAL) {
      esp_now_send(peerAddress, (uint8_t *)&cmd, sizeof(cmd));
      lastSendTime = currentTime;
      lastSent = cmd.action;
    }
  }
  delay(10); 
}
