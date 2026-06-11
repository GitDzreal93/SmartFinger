#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int MOS_PIN = 3;
constexpr int ENC_CLK_PIN = 4;
constexpr int ENC_DT_PIN = 5;
constexpr int OLED_SDA_PIN = 8;
constexpr int OLED_SCL_PIN = 9;
constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;
constexpr int OLED_RESET_PIN = -1;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr int TAP_ACTIVE_LEVEL = HIGH;
constexpr int TAP_IDLE_LEVEL = LOW;

struct ClickProfile {
  uint16_t tapMs;
  uint16_t restMs;
  uint8_t rate;
};

constexpr ClickProfile CLICK_PROFILES[] = {
  {0, 0, 0},
  {70, 260, 3},
  {55, 195, 4},
  {45, 155, 5},
  {35, 105, 7},
  {25, 75, 10},
};

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN);

int grade = 0;
int lastClkState = HIGH;
bool tapping = false;
unsigned long phaseStartedMs = 0;

const ClickProfile& currentProfile() {
  return CLICK_PROFILES[grade];
}

void releaseTap() {
  digitalWrite(MOS_PIN, TAP_IDLE_LEVEL);
  tapping = false;
}

void startTap() {
  digitalWrite(MOS_PIN, TAP_ACTIVE_LEVEL);
  tapping = true;
}

void drawStatusScreen() {
  const ClickProfile& profile = currentProfile();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SmartFinger Tap");
  display.print("Mode: ");
  display.println(grade > 0 ? "RUN" : "STOP");
  display.print("Grade: ");
  display.println(grade);
  display.print("Rate: ");
  display.print(profile.rate);
  display.println("/s");
  display.print("Tap GPIO");
  display.print(MOS_PIN);
  display.println(" HIGH");
  display.println("I2C SDA8 SCL9");
  display.display();
}

void resetClickTiming() {
  releaseTap();
  phaseStartedMs = millis();
  drawStatusScreen();
}

void updateEncoder() {
  int clkState = digitalRead(ENC_CLK_PIN);
  if (clkState != lastClkState && clkState == LOW) {
    int dtState = digitalRead(ENC_DT_PIN);
    int delta = (dtState == HIGH) ? 1 : -1;
    int nextGrade = constrain(grade + delta, 0, 5);
    if (nextGrade != grade) {
      grade = nextGrade;
      resetClickTiming();
    }
  }
  lastClkState = clkState;
}

void updateTapOutput() {
  if (grade == 0) {
    releaseTap();
    return;
  }

  unsigned long now = millis();
  const ClickProfile& profile = currentProfile();

  if (!tapping && now - phaseStartedMs >= profile.restMs) {
    startTap();
    phaseStartedMs = now;
    return;
  }

  if (tapping && now - phaseStartedMs >= profile.tapMs) {
    releaseTap();
    phaseStartedMs = now;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(ENC_CLK_PIN, INPUT_PULLUP);
  pinMode(ENC_DT_PIN, INPUT_PULLUP);
  pinMode(MOS_PIN, OUTPUT);
  releaseTap();
  lastClkState = digitalRead(ENC_CLK_PIN);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED init failed. Check GND/VCC/SCL/SDA or I2C address.");
    return;
  }

  Serial.println("Tap MOS MVP init OK");
  drawStatusScreen();
}

void loop() {
  updateEncoder();
  updateTapOutput();
}
