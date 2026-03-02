#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <Adafruit_TSC2007.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <SPI.h>
#include <SD.h>

// ---------------- PIN DEFINITIONS ----------------
#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8
#define SD_CS    46
#define POT_PIN  A8

#define PRESSURE_THRESHOLD 150

// ---------------- OBJECTS ----------------
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
Adafruit_TSC2007 touch;
Adafruit_BNO055 bno = Adafruit_BNO055(55);

// ---------------- GLOBALS ----------------
volatile bool timerFlag = false;
enum Mode { MENU, MODE1_INPUT, MODE1_RUN, MODE2, MODE3 };
Mode state = MENU;

String inputBuffer = "";
int samplingRate = 20;
int totalPoints = 100;
int pointsLogged = 0;
bool enteringRate = true;

File logFile;

// ---------------- TIMER ISR ----------------
// This handles the timing for data logging
ISR(TIMER1_COMPA_vect) {
  timerFlag = true;
}

void startTimer(int hz) {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  // Calculate compare match value for 16MHz clock with 256 prescaler
  OCR1A = (16000000 / (256 * hz)) - 1;
  TCCR1B |= (1 << WGM12); // CTC Mode
  TCCR1B |= (1 << CS12);  // 256 Prescaler
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

void stopTimer() {
  TIMSK1 &= ~(1 << OCIE1A);
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);

  // Pin 53 must be OUTPUT HIGH for Mega SPI Master mode stability
  pinMode(53, OUTPUT);
  digitalWrite(53, HIGH);

  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  delay(10);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  touch.begin();
  bno.begin();

  if (!SD.begin(SD_CS)) {
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(3);
    tft.setCursor(40, 100);
    tft.print("SD FAIL");
    while(1);
  }

  drawMenu();
}

// ---------------- LOOP ----------------
void loop() {
  uint16_t x, y, z1, z2;

  // Touch sensing logic
  if (touch.read_touch(&x, &y, &z1, &z2)) {
    if (z1 > PRESSURE_THRESHOLD) {
      // Map raw touch coordinates to screen pixels
      int sx = map(y, 400, 3600, 0, 320);
      int sy = map(x, 3600, 400, 0, 240);
      handleTouch(constrain(sx, 0, 319), constrain(sy, 0, 239));
      delay(250); // Debounce
    }
  }

  // Handle timed logging in Mode 1
  if (state == MODE1_RUN && timerFlag) {
    timerFlag = false;
    executeLogging();
  }

  // Constant update for Voltmeter in Mode 2
  if (state == MODE2) updateVoltmeter();
}

// ---------------- MODE 1: DATA LOGGING ----------------
void executeLogging() {
  static unsigned long startTime = millis();
  float voltage = analogRead(POT_PIN) * (5.0 / 1023.0);

  sensors_event_t event;
  bno.getEvent(&event);

  float timeSec = (millis() - startTime) / 1000.0;

  // Log data to SD card
  logFile.print(timeSec, 3);
  logFile.print("\t");
  logFile.print(voltage, 3);
  logFile.print("\t");
  logFile.print(event.orientation.x, 2);
  logFile.print("\t");
  logFile.print(event.orientation.y, 2);
  logFile.print("\t");
  logFile.println(event.orientation.z, 2);

  pointsLogged++;

  if (pointsLogged >= totalPoints) {
    stopTimer();
    logFile.flush();
    delay(200);
    logFile.close();
    state = MENU;
    drawMenu();
  }
}

void drawStopButton() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("Measuring...");
  // Stop button area visualization could be added here
}

// ---------------- MODE 2: VOLTMETER ----------------
void updateVoltmeter() {
  static unsigned long last = 0;
  if (millis() - last >= 100) {
    last = millis();
    float v = analogRead(POT_PIN) * (5.0 / 1023.0);
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.setTextSize(5);
    tft.setCursor(60, 100);
    tft.print(v, 3);
    tft.print(" V");
  }
}

// ---------------- MODE 3: DATA RETRIEVAL ----------------
void runRetrieval() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("Sending Data...");

  File f = SD.open("LOG.TXT");
  if (f) {
    while (f.available()) {
      Serial.write(f.read());
    }
    f.close();
  }

  delay(500);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(3);
  tft.setCursor(20, 100);
  tft.print("REMOVE SD");

  delay(2000);
  state = MENU;
  drawMenu();
}

// ---------------- TOUCH HANDLING ----------------
void handleTouch(int x, int y) {
  if (state == MENU) {
    if (x < 106) {
      state = MODE1_INPUT;
      enteringRate = true;
      inputBuffer = "";
      drawKeypad("Rate (Hz):");
    }
    else if (x < 212) {
      state = MODE2;
      tft.fillScreen(ILI9341_BLACK);
    }
    else {
      state = MODE3;
      runRetrieval();
    }
  }
  else if (state == MODE1_INPUT) {
    handleKeypadPress(x, y);
  }
  else if (state == MODE1_RUN) {
    // Check if the bottom center area is pressed to stop
    if (x > 80 && x < 240 && y > 180) {
      stopTimer();
      logFile.flush();
      logFile.close();
      state = MENU;
      drawMenu();
    }
  }
  else if (state == MODE2) {
    state = MENU;
    drawMenu();
  }
}

void handleKeypadPress(int x, int y) {
  if (y > 100) {
    int col = x / 80;
    int row = (y - 100) / 40;
    int key = row * 4 + col;

    if (key <= 9) {
      inputBuffer += String(key);
    } 
    else if (key == 10) {
      inputBuffer = ""; // Clear
    } 
    else if (key == 11) { // OK
      if (enteringRate) {
        samplingRate = constrain(inputBuffer.toInt(), 1, 40);
        enteringRate = false;
        inputBuffer = "";
        drawKeypad("Points:");
      } else {
        totalPoints = inputBuffer.toInt();
        pointsLogged = 0;
        SD.remove("LOG.TXT");
        logFile = SD.open("LOG.TXT", FILE_WRITE);
        logFile.println("Time\tVoltage\tX-axis\tY-axis\tZ-axis");
        
        state = MODE1_RUN;
        drawStopButton();
        startTimer(samplingRate);
      }
    }
    if (state == MODE1_INPUT)
      drawKeypad(enteringRate ? "Rate (Hz):" : "Points:");
  }
}

// ---------------- UI DRAWING ----------------
void drawMenu() {
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, 105, 240, ILI9341_BLUE);
  tft.fillRect(107, 0, 105, 240, ILI9341_GREEN);
  tft.fillRect(214, 0, 106, 240, ILI9341_RED);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(15, 110);  tft.print("Mode-1");
  tft.setCursor(122, 110); tft.print("Mode-2");
  tft.setCursor(228, 110); tft.print("Mode-3");
}

void drawKeypad(String msg) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  tft.print(msg + " " + inputBuffer);

  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 4; c++) {
      int x = c * 80, y = 100 + r * 40;
      tft.drawRect(x, y, 80, 40, ILI9341_WHITE);
      tft.setCursor(x + 30, y + 10);
      int v = r * 4 + c;
      if (v <= 9) tft.print(v);
      else if (v == 10) tft.print("CLR");
      else if (v == 11) tft.print("OK");
    }
  }
}
