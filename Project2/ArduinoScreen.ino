#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <Adafruit_TSC2007.h>

#define TFT_CS 10
#define TFT_DC 9

Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
Adafruit_TSC2007 touch;

// ---- CALIBRATION ----
#define RAW_MIN 400
#define RAW_MAX 3600
#define PRESSURE_THRESHOLD 150

enum ScreenState {
  MOTOR_SELECT,
  DIRECTION_SELECT,
  POSITION_ENTRY,
  RUN_SCREEN
};

ScreenState state = MOTOR_SELECT;

String motor = "";
String direction = "";
String positionValue = "";

void setup() {
  Serial.begin(9600);

  tft.begin();
  tft.setRotation(1);

  if (!touch.begin()) {
    while (1);
  }

  drawMotorScreen();
}

void loop() {

  uint16_t x, y, z1, z2;

  if (touch.read_touch(&x, &y, &z1, &z2)) {

    if (z1 < PRESSURE_THRESHOLD) return;

    // ---- CORRECTED MAPPING (NO MIRROR) ----
    int sx = map(y, RAW_MIN, RAW_MAX, 0, 320);   // horizontal FIXED
    int sy = map(x, RAW_MAX, RAW_MIN, 0, 240);   // vertical reversed

    sx = constrain(sx, 0, 319);
    sy = constrain(sy, 0, 239);

    handleTouch(sx, sy);

    delay(250);

    while (touch.read_touch(&x, &y, &z1, &z2)) {
      delay(10);
    }
  }

  delay(50);
}

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
        drawRunScreen();
        state = RUN_SCREEN;
        return;
      }

      drawPositionScreen();
    }
  }

  else if (state == RUN_SCREEN) {

    motor = "";
    direction = "";
    positionValue = "";
    drawMotorScreen();
    state = MOTOR_SELECT;
  }
}

// ---------------- DRAW FUNCTIONS ----------------

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

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);

  tft.setCursor(20, 20);
  tft.print("Motor: ");
  tft.print(motor);

  tft.fillRect(0, 80, 160, 80, ILI9341_GREEN);
  tft.fillRect(160, 80, 160, 80, ILI9341_BLUE);

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
  tft.print(motor + " " + direction);

  tft.setCursor(20, 50);
  tft.print("Pos: " + positionValue);

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

void drawRunScreen() {

  tft.fillScreen(ILI9341_BLACK);

  tft.setTextSize(3);
  tft.setTextColor(ILI9341_GREEN);

  tft.setCursor(40, 60);
  tft.print("RUNNING");

  tft.setTextSize(2);

  tft.setCursor(40, 110);
  tft.print(motor);

  tft.setCursor(40, 140);
  tft.print(direction);

  tft.setCursor(40, 170);
  tft.print(positionValue + " deg");
}
