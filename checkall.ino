// Back L298N Driver Pins
const int B_ENA = 22; // IO22
const int B_IN1 = 16; // IO16
const int B_IN2 = 17; // IO17
const int B_IN3 = 18; // IO18
const int B_IN4 = 19; // IO19
const int B_ENB = 23; // IO23

// Front L298N Driver Pins
const int F_ENA = 32; // IO32
const int F_IN1 = 33; // IO33
const int F_IN2 = 25; // IO25
const int F_IN3 = 26; // IO26
const int F_IN4 = 27; // IO27
const int F_ENB = 14; // IO14

void moveMotors(int f1, int f2, int f3, int f4, int b1, int b2, int b3, int b4) {
  digitalWrite(F_IN1, f1); digitalWrite(F_IN2, f2);
  digitalWrite(F_IN3, f3); digitalWrite(F_IN4, f4);
  digitalWrite(B_IN1, b1); digitalWrite(B_IN2, b2);
  digitalWrite(B_IN3, b3); digitalWrite(B_IN4, b4);
}

void setup() {
  Serial.begin(115200);
  
  // Initialize all motor pins as OUTPUT
  int allPins[] = {32, 33, 25, 26, 27, 14, 22, 16, 17, 18, 19, 23};
  for(int p : allPins) pinMode(p, OUTPUT);
  
  // Enable motor drivers
  digitalWrite(F_ENA, HIGH); digitalWrite(F_ENB, HIGH);
  digitalWrite(B_ENA, HIGH); digitalWrite(B_ENB, HIGH);
  
  Serial.println("Starting 5-second Hardware Test...");
}

void loop() {
  // 1. FORWARD
  Serial.println("Moving: FORWARD");
  moveMotors(1,0,1,0,1,0,1,0); 
  delay(5000);

  // 2. BACKWARD
  Serial.println("Moving: BACKWARD");
  moveMotors(0,1,0,1,0,1,0,1); 
  delay(5000);

  // 3. STRAFE LEFT
  Serial.println("Moving: STRAFE LEFT");
  moveMotors(0,1,1,0,1,0,0,1); 
  delay(5000);

  // 4. STRAFE RIGHT
  Serial.println("Moving: STRAFE RIGHT");
  moveMotors(1,0,0,1,0,1,1,0); 
  delay(5000);

  // 5. STOP
  Serial.println("STOPPED - Restarting Cycle");
  moveMotors(0,0,0,0,0,0,0,0);
  delay(2000);
}
