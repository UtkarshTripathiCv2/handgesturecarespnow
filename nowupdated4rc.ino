#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define CHANNEL 6
// Lowered speed to 130 to prevent voltage dips/disconnections
int speedVal = 130; 

// MOTOR PINS
const int FL_EN = 32; const int FL_IN1 = 33; const int FL_IN2 = 25;
const int FR_EN = 14; const int FR_IN1 = 26; const int FR_IN2 = 27;
const int BL_EN = 22; const int BL_IN1 = 16; const int BL_IN2 = 17;
const int BR_EN = 23; const int BR_IN1 = 18; const int BR_IN2 = 19;

QueueHandle_t motorQueue;
struct CommandData { char action; };

void driveMotor(int in1, int in2, int in3, int in4,
                int in5, int in6, int in7, int in8,
                int spFL, int spFR, int spBL, int spBR) {
  digitalWrite(FL_IN1, in1); digitalWrite(FL_IN2, in2);
  digitalWrite(FR_IN1, in3); digitalWrite(FR_IN2, in4);
  digitalWrite(BL_IN1, in5); digitalWrite(BL_IN2, in6);
  digitalWrite(BR_IN1, in7); digitalWrite(BR_IN2, in8);

  ledcWrite(FL_EN, spFL); ledcWrite(FR_EN, spFR);
  ledcWrite(BL_EN, spBL); ledcWrite(BR_EN, spBR);
}

void TaskMotor(void *pvParameters) {
  CommandData cmd;
  for(;;) {
    if(xQueueReceive(motorQueue, &cmd, pdMS_TO_TICKS(300))) {
      switch(cmd.action) {
        case 'F': // Forward
          driveMotor(1,0, 1,0, 1,0, 1,0, speedVal, speedVal, speedVal, speedVal);
          break;
        case 'B': // Backward
          driveMotor(0,1, 0,1, 0,1, 0,1, speedVal, speedVal, speedVal, speedVal);
          break;
        case 'L': // Mecanum Left Shift (Strafing)
          driveMotor(0,1, 1,0, 1,0, 0,1, speedVal, speedVal, speedVal, speedVal);
          break;
        case 'R': // Mecanum Right Shift (Strafing)
          driveMotor(1,0, 0,1, 0,1, 1,0, speedVal, speedVal, speedVal, speedVal);
          break;
        default: // Stop
          driveMotor(0,0, 0,0, 0,0, 0,0, 0,0,0,0);
          break;
      }
    } else {
      driveMotor(0,0, 0,0, 0,0, 0,0, 0,0,0,0);
    }
  }
}

void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  CommandData cmd;
  cmd.action = ((char*)data)[0];
  xQueueOverwriteFromISR(motorQueue, &cmd, NULL);
}

void setup() {
  int pins[] = {FL_IN1, FL_IN2, FR_IN1, FR_IN2, BL_IN1, BL_IN2, BR_IN1, BR_IN2};
  for(int i=0; i<8; i++) pinMode(pins[i], OUTPUT);

  ledcAttach(FL_EN, 1000, 8); ledcAttach(FR_EN, 1000, 8);
  ledcAttach(BL_EN, 1000, 8); ledcAttach(BR_EN, 1000, 8);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  if(esp_now_init() != ESP_OK) ESP.restart();
  esp_now_register_recv_cb(onDataRecv);
  motorQueue = xQueueCreate(1, sizeof(CommandData));
  xTaskCreatePinnedToCore(TaskMotor, "MotorTask", 4096, NULL, 1, NULL, 1);
}

void loop() { vTaskDelete(NULL); }
