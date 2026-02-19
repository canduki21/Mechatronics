#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <Adafruit_TSC2007.h>
#include <Servo.h>

// ---------- TFT ----------
#define TFT_CS 10
#define TFT_DC 9

Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
Adafruit_TSC2007 touch;

// ---------- TOUCH ----------
#define RAW_MIN 400
#define RAW_MAX 3600
#define PRESSURE_THRESHOLD 150

// ---------- STEPPER ----------
#define IN1 30
#define IN2 31
#define IN3 32
#define IN4 33
#define STEPS_PER_REV 2048

int stepSequence[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

int currentStepIndex = 0;
long currentPosition = 0;

// ---------- SERVO ----------
Servo myServo;
#define SERVO_PIN 3

// ---------- UI ----------
enum ScreenState {
  MOTOR_SELECT,
  DIRECTION_SELECT,
  POSITION_ENTRY
};

ScreenState state = MOTOR_SELECT;

String motor = "";
String direction = "";
String positionValue = "";

#define HOLD_TIME 3000

// ---------- STEPPER ----------
void singleStep(bool clockwise) {

  if (clockwise)
    currentStepIndex++;
  else
    currentStepIndex--;

  if (currentStepIndex > 7) currentStepIndex = 0;
  if (currentStepIndex < 0) currentStepIndex = 7;

  digitalWrite(IN1, stepSequence[currentStepIndex][0]);
  digitalWrite(IN2, stepSequence[currentStepIndex][1]);
  digitalWrite(IN3, stepSequence[currentStepIndex][2]);
  digitalWrite(IN4, stepSequence[currentStepIndex][3]);

  delay(2);
}

void moveSteps(long steps, bool clockwise) {

  for (long i = 0; i < steps; i++) {

    singleStep(clockwise);

    if (clockwise)
      currentPosition++;
    else
      currentPosition--;
  }

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void moveStepperAngle(int angle, bool clockwise) {

  long steps = (long)angle * STEPS_PER_REV / 360;
  moveSteps(steps, clockwise);
}

void returnStepperToZero() {

  if (currentPosition > 0)
    moveSteps(currentPosition, false);
  else if (currentPosition < 0)
    moveSteps(-currentPosition, true);

  currentPosition = 0;
}

// ---------- SETUP ----------
void setup() {

  Serial.begin(9600);

  tft.begin();
  tft.setRotation(1);
  touch.begin();

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  myServo.attach(SERVO_PIN);
  myServo.write(0);

  drawMotorScreen();
}

// ---------- LOOP ----------
void loop() {

  uint16_t x, y, z1, z2;

  if (touch.read_touch(&x, &y, &z1, &z2)) {

    if (z1 < PRESSURE_THRESHOLD) return;

    int sx = map(y, RAW_MIN, RAW_MAX, 0, 320);
    int sy = map(x, RAW_MAX, RAW_MIN, 0, 240);

    sx = constrain(sx, 0, 319);
    sy = constrain(sy, 0, 239);

    handleTouch(sx, sy);

    delay(250);
    while (touch.read_touch(&x, &y, &z1, &z2)) delay(10);
  }
}

// ---------- TOUCH ----------
void handleTouch(int x, int y) {

  if (state == MOTOR_SELECT) {

    if (x < 160) motor = "SERVO";
    else motor = "STEPPER";

    drawDirectionScreen();
    state = DIRECTION_SELECT;
  }

  else if (state == DIRECTION_SELECT) {

    if (y > 80 && y < 160) {

      if (x < 160) direction = "CW";
      else direction = "CCW";

      positionValue = "";
      drawPositionScreen();
      state = POSITION_ENTRY;
    }
  }

  else if (state == POSITION_ENTRY) {

    if (y > 100) {

      int col = x / 80;
      int row = (y - 100) / 40;
      int key = row * 4 + col;

      if (key >= 0 && key <= 9) {
        if (positionValue.length() < 3)
          positionValue += String(key);
      }
      else if (key == 10) {
        positionValue = "";
      }
      else if (key == 11) {

        int angle = positionValue.toInt();
        bool cw = (direction == "CW");

        tft.fillScreen(ILI9341_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.setCursor(40, 40);
        tft.print("Moving...");

        if (motor == "STEPPER") {

          moveStepperAngle(angle, cw);
          delay(HOLD_TIME);
          returnStepperToZero();

        } else if (motor == "SERVO") {

  angle = constrain(angle, 0, 180);

  if (direction == "CW") {

    // Normal CW
    myServo.write(angle);
    delay(HOLD_TIME);
    myServo.write(0);
  }
  else {

    //  Go to 180
    myServo.write(180);
    delay(2500);

    // 2️⃣ Go backward from 180
    int backPosition = 180 - angle;
    myServo.write(backPosition);
    delay(1000);

    // Go to the entered angle
    myServo.write(angle);
    delay(HOLD_TIME);

    // Return to normal position
    myServo.write(0);
  }
}


        delay(1000);

        drawMotorScreen();
        state = MOTOR_SELECT;
        return;
      }

      drawPositionScreen();
    }
  }
}

// ---------- DRAW ----------
void drawMotorScreen() {

  tft.fillScreen(ILI9341_BLACK);

  tft.fillRect(0, 0, 160, 240, ILI9341_GREEN);
  tft.fillRect(160, 0, 160, 240, ILI9341_BLUE);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);

  tft.setCursor(30, 100);
  tft.print("SERVO");

  tft.setCursor(170, 100);
  tft.print("STEPPER");
}

void drawDirectionScreen() {

  tft.fillScreen(ILI9341_BLACK);

  tft.fillRect(0, 80, 160, 80, ILI9341_GREEN);
  tft.fillRect(160, 80, 160, 80, ILI9341_BLUE);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);

  tft.setCursor(50, 100);
  tft.print("CW");

  tft.setCursor(200, 100);
  tft.print("CCW");
}

void drawPositionScreen() {

  tft.fillScreen(ILI9341_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(20, 20);
  tft.print("Angle: " + positionValue);

  int key = 0;

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 4; col++) {

      int x = col * 80;
      int y = 100 + row * 40;

      tft.drawRect(x, y, 80, 40, ILI9341_WHITE);

      tft.setCursor(x + 30, y + 10);
      tft.setTextSize(2);

      if (key <= 9) tft.print(key);
      else if (key == 10) tft.print("CLR");
      else if (key == 11) tft.print("OK");

      key++;
    }
  }
}
