#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>

#define MPU 0x68
#define CHANNEL 6
#define DEADZONE 7000 // Increased deadzone for better stability

uint8_t peerAddress[] = {0x80, 0xF3, 0xDA, 0x63, 0x2F, 0x00}; // REPLACE WITH YOUR RECEIVER MAC

QueueHandle_t commandQueue;
struct CommandData { char action; };

void TaskMPU(void *pvParameters) {
  Wire.begin(21, 22);
  Wire.beginTransmission(MPU);
  Wire.write(0x6B); Wire.write(0x00);
  Wire.endTransmission(true);

  int16_t ax, ay;
  CommandData cmd;

  for(;;) {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 4, true);

    if(Wire.available() >= 4) {
      ax = Wire.read()<<8 | Wire.read();
      ay = Wire.read()<<8 | Wire.read();

      if(ax < -DEADZONE)      cmd.action = 'F'; // Tilt Forward
      else if(ax > DEADZONE)  cmd.action = 'B'; // Tilt Backward
      else if(ay > DEADZONE)  cmd.action = 'L'; // Tilt Left (Shift)
      else if(ay < -DEADZONE) cmd.action = 'R'; // Tilt Right (Shift)
      else                    cmd.action = 'S'; // Stop

      xQueueOverwrite(commandQueue, &cmd);
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Sample at 10Hz to keep things cool
  }
}

void TaskSend(void *pvParameters) {
  CommandData cmd;
  for(;;) {
    if(xQueueReceive(commandQueue, &cmd, portMAX_DELAY)) {
      esp_now_send(peerAddress, (uint8_t*)&cmd, sizeof(cmd));
    }
    vTaskDelay(pdMS_TO_TICKS(50)); // Small gap between sends to prevent crashing
  }
}

void setup() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  if(esp_now_init() != ESP_OK) ESP.restart();

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = CHANNEL;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  commandQueue = xQueueCreate(1, sizeof(CommandData));
  xTaskCreatePinnedToCore(TaskMPU, "MPU", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskSend, "Send", 4096, NULL, 2, NULL, 0);
}

void loop() { vTaskDelete(NULL); }
