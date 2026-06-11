#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int SERVO_PIN = 3;
constexpr int ENC_CLK_PIN = 4;
constexpr int ENC_DT_PIN = 5;
constexpr int OLED_SDA_PIN = 8;
constexpr int OLED_SCL_PIN = 9;
constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;
constexpr int OLED_RESET_PIN = -1;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint16_t SERVO_UP_US = 900;
constexpr uint16_t SERVO_DOWN_US = 1500;
constexpr uint16_t SERVO_PERIOD_US = 20000;
constexpr bool SERVO_SELF_TEST = false;

struct ClickProfile {
  uint16_t downMs;
  uint16_t upMs;
  uint8_t rate;
};

constexpr ClickProfile CLICK_PROFILES[] = {
  {0, 0, 0},
  {80, 220, 3},
  {60, 140, 5},
  {50, 70, 8},
  {40, 40, 12},
  {25, 25, 20},
};

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN);

int grade = 0;
int lastClkState = HIGH;
unsigned long phaseStartedMs = 0;
bool pressPhase = false;

const ClickProfile& currentProfile() {
  return CLICK_PROFILES[grade];
}

void writeServoPulse(uint16_t pulseUs) {
  digitalWrite(SERVO_PIN, HIGH);
  delayMicroseconds(pulseUs);
  digitalWrite(SERVO_PIN, LOW);
  delayMicroseconds(SERVO_PERIOD_US - pulseUs);
}

void liftServo() {
  writeServoPulse(SERVO_UP_US);
}

void pressServo() {
  writeServoPulse(SERVO_DOWN_US);
}

void drawStatusScreen() {
  const ClickProfile& profile = currentProfile();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SmartFinger");
  display.print("Mode: ");
  display.println(grade > 0 ? "RUN" : "STOP");
  display.print("Grade: ");
  display.println(grade);
  display.print("Rate: ");
  display.print(profile.rate);
  display.println("/s");
  display.println("I2C SDA8 SCL9");
  display.display();
}

void drawServoTestScreen(uint16_t pulseUs) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SmartFinger");
  display.println("Servo Test");
  display.print("GPIO: ");
  display.println(SERVO_PIN);
  display.print("Pulse: ");
  display.print(pulseUs);
  display.println("us");
  display.display();
}

void resetClickTiming() {
  liftServo();
  pressPhase = false;
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

void updateClickOutput() {
  if (grade == 0) {
    liftServo();
    return;
  }

  unsigned long now = millis();
  const ClickProfile& profile = currentProfile();

  if (!pressPhase && now - phaseStartedMs >= profile.upMs) {
    pressServo();
    pressPhase = true;
    phaseStartedMs = now;
    return;
  }

  if (pressPhase && now - phaseStartedMs >= profile.downMs) {
    liftServo();
    pressPhase = false;
    phaseStartedMs = now;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(ENC_CLK_PIN, INPUT_PULLUP);
  pinMode(ENC_DT_PIN, INPUT_PULLUP);
  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);
  liftServo();
  lastClkState = digitalRead(ENC_CLK_PIN);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED init failed. Check GND/VCC/SCL/SDA or I2C address.");
    return;
  }

  Serial.println("OLED init OK");
  drawStatusScreen();
}

void loop() {
  if (SERVO_SELF_TEST) {
    static bool down = false;
    static unsigned long lastMoveMs = 0;
    static uint16_t pulseUs = SERVO_UP_US;
    unsigned long now = millis();
    if (now - lastMoveMs >= 1000) {
      down = !down;
      pulseUs = down ? SERVO_DOWN_US : SERVO_UP_US;
      drawServoTestScreen(pulseUs);
      lastMoveMs = now;
    }
    writeServoPulse(pulseUs);
    return;
  }

  updateEncoder();
  updateClickOutput();
}
