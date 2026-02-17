#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define CHANNEL 6
int motorSpeed = 160;

const int F_ENA = 32, F_IN1 = 33, F_IN2 = 25, F_IN3 = 26, F_IN4 = 27, F_ENB = 14;
const int B_ENA = 22, B_IN1 = 16, B_IN2 = 17, B_IN3 = 18, B_IN4 = 19, B_ENB = 23;

QueueHandle_t motorQueue;
struct CommandData { char action; };

void moveMotors(int f1, int f2, int f3, int f4, int b1, int b2, int b3, int b4) {
  digitalWrite(F_IN1, f1); digitalWrite(F_IN2, f2);
  digitalWrite(F_IN3, f3); digitalWrite(F_IN4, f4);
  digitalWrite(B_IN1, b1); digitalWrite(B_IN2, b2);
  digitalWrite(B_IN3, b3); digitalWrite(B_IN4, b4);
  analogWrite(F_ENA, motorSpeed); analogWrite(F_ENB, motorSpeed);
  analogWrite(B_ENA, motorSpeed); analogWrite(B_ENB, motorSpeed);
}

// Task: Motor Execution (Core 1)
void TaskMotorControl(void *pvParameters) {
  CommandData rxCmd;
  for (;;) {
    if (xQueueReceive(motorQueue, &rxCmd, pdMS_TO_TICKS(500))) {
      switch (rxCmd.action) {
        case 'F': moveMotors(1,0, 1,0, 1,0, 1,0); break;
        case 'B': moveMotors(0,1, 0,1, 0,1, 0,1); break;
        case 'L': moveMotors(0,1, 1,0, 1,0, 0,1); break;
        case 'R': moveMotors(1,0, 0,1, 0,1, 1,0); break;
        default:  moveMotors(0,0, 0,0, 0,0, 0,0); break;
      }
    } else {
      moveMotors(0,0, 0,0, 0,0, 0,0); // Failsafe stop
    }
  }
}

// Callback (Core 0 Context)
void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  CommandData cmd;
  cmd.action = ((char*)data)[0];
  xQueueOverwriteFromISR(motorQueue, &cmd, NULL);
}

void setup() {
  int pins[] = {32,33,25,26,27,14,22,16,17,18,19,23};
  for(int p : pins) pinMode(p, OUTPUT);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) ESP.restart();
  esp_now_register_recv_cb(onDataRecv);

  motorQueue = xQueueCreate(1, sizeof(CommandData));

  // Motor Task on Core 1, Priority 1
  xTaskCreatePinnedToCore(TaskMotorControl, "MotorTask", 4096, NULL, 1, NULL, 1);
}

void loop() { vTaskDelete(NULL); }
