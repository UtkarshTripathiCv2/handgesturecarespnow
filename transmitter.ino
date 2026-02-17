#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>

#define MPU_ADDR 0x68
#define CHANNEL 6
#define DEADZONE 6000

uint8_t broadcastAddress[] = {0x80, 0xF3, 0xDA, 0x63, 0x2F, 0x00};
QueueHandle_t commandQueue;

struct CommandData { char action; };

// Task 1: Read MPU9250 (Core 1)
void TaskReadMPU(void *pvParameters) {
  Wire.begin(21, 22);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);
  Wire.endTransmission(true);

  int16_t ax, ay;
  CommandData cmd;

  for (;;) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 4, true);

    if (Wire.available() >= 4) {
      ax = Wire.read() << 8 | Wire.read();
      ay = Wire.read() << 8 | Wire.read();

      if (ax < -DEADZONE)      cmd.action = 'F';
      else if (ax > DEADZONE)  cmd.action = 'B';
      else if (ay < -DEADZONE) cmd.action = 'R';
      else if (ay > DEADZONE)  cmd.action = 'L';
      else                     cmd.action = 'S';

      xQueueOverwrite(commandQueue, &cmd);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// Task 2: ESP-NOW Transmission (Core 0 - High Priority)
void TaskSendData(void *pvParameters) {
  CommandData cmdToSend;
  for (;;) {
    if (xQueueReceive(commandQueue, &cmdToSend, portMAX_DELAY)) {
      esp_now_send(broadcastAddress, (uint8_t *)&cmdToSend, sizeof(cmdToSend));
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void setup() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) ESP.restart();

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = CHANNEL;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  commandQueue = xQueueCreate(1, sizeof(CommandData));

  xTaskCreatePinnedToCore(TaskReadMPU, "ReadMPU", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskSendData, "SendData", 4096, NULL, 2, NULL, 0);
}

void loop() { vTaskDelete(NULL); }
