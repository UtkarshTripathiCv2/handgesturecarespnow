#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>

#define CHANNEL 6
#define MAX_SPEED 160  
#define START_SPEED 80 // Enough power to start the motors
#define WDT_TIMEOUT 5 

const int FL_EN = 32; const int FL_IN1 = 33; const int FL_IN2 = 25;
const int FR_EN = 14; const int FR_IN1 = 26; const int FR_IN2 = 27;
const int BL_EN = 22; const int BL_IN1 = 16; const int BL_IN2 = 17;
const int BR_EN = 23; const int BR_IN1 = 18; const int BR_IN2 = 19;

QueueHandle_t motorQueue;
struct CommandData { char action; };

void move(int f1, int f2, int f3, int f4, int b1, int b2, int b3, int b4, int spd) {
  digitalWrite(FL_IN1, f1); digitalWrite(FL_IN2, f2);
  digitalWrite(FR_IN1, f3); digitalWrite(FR_IN2, f4);
  digitalWrite(BL_IN1, b1); digitalWrite(BL_IN2, b2);
  digitalWrite(BR_IN1, b3); digitalWrite(BR_IN2, b4);
  
  ledcWrite(FL_EN, spd); ledcWrite(FR_EN, spd);
  ledcWrite(BL_EN, spd); ledcWrite(BR_EN, spd);
}

void TaskMotor(void *pvParameters) {
  esp_task_wdt_add(NULL); 
  CommandData cmd;
  char currentAction = 'S';
  int rampSpeed = 0;

  for (;;) {
    esp_task_wdt_reset(); 

    // Wait up to 500ms for a command. This prevents "pulsing" if a packet is lost.
    if (xQueueReceive(motorQueue, &cmd, pdMS_TO_TICKS(500))) {
      if (cmd.action != currentAction) {
        currentAction = cmd.action;
        rampSpeed = (currentAction == 'S') ? 0 : START_SPEED;
      }
      if (currentAction != 'S' && rampSpeed < MAX_SPEED) {
        rampSpeed += 20; // Fast ramp for responsiveness
        if (rampSpeed > MAX_SPEED) rampSpeed = MAX_SPEED;
      }
    } else {
      // If no signal for 0.5 seconds, safety stop
      currentAction = 'S';
      rampSpeed = 0;
    }

    switch (currentAction) {
      case 'F': move(1,0, 1,0, 1,0, 1,0, rampSpeed); break;
      case 'B': move(0,1, 0,1, 0,1, 0,1, rampSpeed); break;
      case 'L': move(0,1, 1,0, 1,0, 0,1, rampSpeed); break; 
      case 'R': move(1,0, 0,1, 0,1, 1,0, rampSpeed); break; 
      default:  move(0,0, 0,0, 0,0, 0,0, 0); break;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  CommandData incoming;
  incoming.action = ((char*)data)[0];
  xQueueOverwriteFromISR(motorQueue, &incoming, NULL);
}

void setup() {
  Serial.begin(115200);
  int p[] = {33,25,26,27,16,17,18,19};
  for(int i=0; i<8; i++) pinMode(p[i], OUTPUT);

  // 2kHz frequency is generally better for L298N
  ledcAttach(FL_EN, 2000, 8); ledcAttach(FR_EN, 2000, 8);
  ledcAttach(BL_EN, 2000, 8); ledcAttach(BR_EN, 2000, 8);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) ESP.restart();
  esp_now_register_recv_cb(onDataRecv);

  // WDT for Core 3.x
  esp_task_wdt_config_t twdt_config = {
      .timeout_ms = WDT_TIMEOUT * 1000, 
      .idle_core_mask = 3, 
      .trigger_panic = true
  };
  esp_task_wdt_init(&twdt_config);

  motorQueue = xQueueCreate(1, sizeof(CommandData));
  xTaskCreatePinnedToCore(TaskMotor, "Motor", 4096, NULL, 1, NULL, 1);
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }
