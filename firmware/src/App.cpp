#include "App.h"

#include "AppConfig.h"

namespace {

String formatTimeOfDay(uint32_t secondsOfDay) {
  const uint32_t hours = (secondsOfDay / 3600U) % 24U;
  const uint32_t minutes = (secondsOfDay / 60U) % 60U;
  const uint32_t seconds = secondsOfDay % 60U;
  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu",
           static_cast<unsigned long>(hours),
           static_cast<unsigned long>(minutes),
           static_cast<unsigned long>(seconds));
  return String(buffer);
}

const char* adaptiveStepToken(AdaptiveStep step) {
  switch (step) {
    case AdaptiveStep::Calculate:
      return "CALC";
    case AdaptiveStep::Press:
      return "PRESS";
    case AdaptiveStep::Release:
      return "RELEASE";
    case AdaptiveStep::Complete:
      return "DONE";
    case AdaptiveStep::Idle:
    default:
      return "WAIT";
  }
}

uint32_t secondsUntilTarget(uint32_t currentSeconds, uint32_t targetSeconds) {
  if (targetSeconds >= currentSeconds) {
    return targetSeconds - currentSeconds;
  }
  return 24U * 3600U - currentSeconds + targetSeconds;
}

}  // namespace

App::App()
    : clickController_(AppConfig::kTapPin, AppConfig::kTapActiveLevel, AppConfig::kTapIdleLevel),
      encoder_(AppConfig::kEncoderClkPin, AppConfig::kEncoderDtPin),
      display_(&Wire,
               AppConfig::kOledWidth,
               AppConfig::kOledHeight,
               AppConfig::kOledResetPin,
               AppConfig::kOledAddress) {}

void App::begin() {
  Serial.begin(115200);
  delay(300);

  encoder_.begin();
  clickController_.begin();
  clickController_.setGrade(0, millis());

  if (!display_.begin(AppConfig::kOledSdaPin, AppConfig::kOledSclPin)) {
    Serial.println("OLED init failed. Check GND/VCC/SCL/SDA or I2C address.");
  }

  Serial.println("SmartFinger firmware init OK");
  Serial.println("Commands: RUSH START|STOP | SELECT <1-5> | START | STOP | TIME HH:MM:SS | AT HH:MM:SS | IN <seconds> | CANCEL | STATUS | PROFILE <tap_ms> <rest_ms> | PROFILE CLEAR");
  refreshDisplay(millis());
  emitState(millis());
}

void App::loop() {
  const unsigned long nowMs = millis();
  processSerial(nowMs);

  updateSchedule(nowMs);
  if (!(scheduleState_.armed && clickController_.grade() == AppConfig::kAdaptiveGrade)) {
    clickController_.update(nowMs);
  }
  if (clickController_.isAdaptiveMode() && (clickController_.isRunning() || scheduleState_.armed)) {
    static unsigned long lastAdaptiveDisplayRefreshMs = 0;
    if (nowMs - lastAdaptiveDisplayRefreshMs >= 100) {
      refreshDisplay(nowMs);
      lastAdaptiveDisplayRefreshMs = nowMs;
    }
  }
  if ((scheduleState_.armed || clickController_.isRushMode() ||
       (clickController_.isAdaptiveMode() && clickController_.isRunning())) &&
      nowMs - lastStateEmitMs_ >= 200) {
    emitState(nowMs);
  }

  if (clickController_.consumeAdaptiveCompletion()) {
    const ClickController::AdaptiveRunStats run = clickController_.lastAdaptiveRun();
    Serial.print("Adaptive run complete. elapsed_us=");
    Serial.print(run.elapsedUs);
    Serial.print(" clicks=");
    Serial.print(run.clickCount);
    Serial.print(" avg_cps=");
    Serial.println(run.averageCps, 2);
    refreshDisplay(nowMs);
    emitState(nowMs);
  }
}

void App::processSerial(unsigned long nowMs) {
  while (Serial.available() > 0) {
    const char ch = static_cast<char>(Serial.read());
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      const String line = serialBuffer_;
      serialBuffer_ = "";
      if (line.length() > 0) {
        handleCommand(line, nowMs);
      }
      continue;
    }
    if (serialBuffer_.length() < 64) {
      serialBuffer_ += ch;
    }
  }
}

void App::handleCommand(const String& line, unsigned long nowMs) {
  String command = line;
  command.trim();
  if (command.length() == 0) {
    return;
  }

  const int spaceIndex = command.indexOf(' ');
  String verb = spaceIndex >= 0 ? command.substring(0, spaceIndex) : command;
  String arg = spaceIndex >= 0 ? command.substring(spaceIndex + 1) : "";
  verb.toUpperCase();
  arg.trim();

  if (verb == "TIME") {
    uint32_t secondsOfDay = 0;
    if (!parseTimeOfDay(arg, &secondsOfDay)) {
      Serial.println("TIME parse failed. Use TIME HH:MM:SS");
      return;
    }
    clockState_.valid = true;
    clockState_.syncedMillis = nowMs;
    clockState_.secondsOfDay = secondsOfDay;
    emitOk(String("Clock set to ") + formatTimeOfDay(secondsOfDay));
    refreshDisplay(nowMs);
    emitState(nowMs);
    return;
  }

  if (verb == "AT") {
    uint32_t targetSecondsOfDay = 0;
    if (!clockState_.valid) {
      Serial.println("Set clock first with TIME HH:MM:SS");
      return;
    }
    if (!parseTimeOfDay(arg, &targetSecondsOfDay)) {
      Serial.println("AT parse failed. Use AT HH:MM:SS");
      return;
    }
    armClockStart(targetSecondsOfDay, nowMs);
    return;
  }

  if (verb == "IN") {
    const float delaySeconds = arg.toFloat();
    if (delaySeconds <= 0.0f) {
      Serial.println("IN parse failed. Use IN <seconds>");
      return;
    }
    armRelativeStart(static_cast<unsigned long>(delaySeconds * 1000.0f), nowMs);
    return;
  }

  if (verb == "CANCEL") {
    cancelSchedule();
    emitOk("Schedule cancelled.");
    refreshDisplay(nowMs);
    emitState(nowMs);
    return;
  }

  if (verb == "STATUS") {
    emitState(nowMs);
    return;
  }

  if (verb == "RUSH") {
    if (arg.equalsIgnoreCase("START")) {
      cancelSchedule();
      clickController_.startRush(nowMs);
      refreshDisplay(nowMs);
      emitOk("Rush mode started.");
      emitState(nowMs);
      return;
    }
    if (arg.equalsIgnoreCase("STOP")) {
      clickController_.stopRush(nowMs);
      refreshDisplay(nowMs);
      emitOk("Rush mode stopped.");
      emitState(nowMs);
      return;
    }
    emitError("RUSH expects START or STOP");
    return;
  }

  if (verb == "GRADE") {
    const int grade = arg.toInt();
    if (grade < 0 || grade > AppConfig::kMaxGrade) {
      emitError("GRADE expects 0-6");
      return;
    }
    cancelSchedule();
    applyGrade(static_cast<uint8_t>(grade), nowMs);
    emitOk(String("Grade set to ") + grade);
    emitState(nowMs);
    return;
  }

  if (verb == "SELECT") {
    const int grade = arg.toInt();
    if (grade < 1 || grade > 5) {
      emitError("SELECT expects 1-5");
      return;
    }
    selectedFixedGrade_ = static_cast<uint8_t>(grade);
    emitOk(String("Immediate grade selected: ") + grade);
    emitState(nowMs);
    return;
  }

  if (verb == "START") {
    cancelSchedule();
    clickController_.setGrade(selectedFixedGrade_, nowMs);
    refreshDisplay(nowMs);
    emitOk(String("Immediate run started at Grade ") + selectedFixedGrade_);
    emitState(nowMs);
    return;
  }

  if (verb == "STOP") {
    cancelSchedule();
    clickController_.setGrade(0, nowMs);
    refreshDisplay(nowMs);
    emitOk("Stopped.");
    emitState(nowMs);
    return;
  }

  if (verb == "PROFILE") {
    if (arg.equalsIgnoreCase("CLEAR")) {
      clickController_.clearFixedProfileOverride();
      emitOk("Custom profile cleared.");
      emitState(nowMs);
      return;
    }

    const int separator = arg.indexOf(' ');
    if (separator < 0) {
      emitError("PROFILE expects: PROFILE <tap_ms> <rest_ms>");
      return;
    }
    const uint16_t tapMs = static_cast<uint16_t>(arg.substring(0, separator).toInt());
    const uint16_t restMs = static_cast<uint16_t>(arg.substring(separator + 1).toInt());
    if (tapMs == 0 || restMs == 0) {
      emitError("PROFILE values must be positive");
      return;
    }
    clickController_.setFixedProfileOverride(tapMs, restMs);
    refreshDisplay(nowMs);
    emitOk(String("Custom profile set tap=") + tapMs + " rest=" + restMs);
    emitState(nowMs);
    return;
  }

  emitError("Unknown command");
}

void App::emitOk(const String& message) {
  Serial.print("OK ");
  Serial.println(message);
}

void App::emitError(const String& message) {
  Serial.print("ERR ");
  Serial.println(message);
}

void App::emitState(unsigned long nowMs) {
  const ClickProfile profile = clickController_.currentProfile();
  Serial.print("STATE ");
  Serial.print("mode=");
  Serial.print(currentModeLabel());
  Serial.print(" grade=");
  Serial.print(clickController_.grade());
  Serial.print(" selected_grade=");
  Serial.print(selectedFixedGrade_);
  Serial.print(" running=");
  Serial.print(clickController_.isRunning() ? 1 : 0);
  Serial.print(" armed=");
  Serial.print(scheduleState_.armed ? 1 : 0);
  Serial.print(" adaptive=");
  Serial.print(clickController_.isAdaptiveMode() ? 1 : 0);
  Serial.print(" rush=");
  Serial.print(clickController_.isRushMode() ? 1 : 0);
  Serial.print(" step=");
  Serial.print(currentStepLabel());
  Serial.print(" left_ms=");
  Serial.print(scheduleState_.armed ? scheduleRemainingMs(nowMs) : clickController_.adaptiveRemainingMs());
  Serial.print(" tap_ms=");
  Serial.print(profile.tapMs);
  Serial.print(" rest_ms=");
  Serial.print(profile.restMs);
  Serial.print(" custom=");
  Serial.print(clickController_.hasFixedProfileOverride() ? 1 : 0);
  Serial.print(" cps=");
  Serial.print(profile.rate);
  Serial.print(" clock=");
  Serial.print(currentClockLabel(nowMs));
  if (clickController_.isAdaptiveComplete()) {
    const ClickController::AdaptiveRunStats run = clickController_.lastAdaptiveRun();
    Serial.print(" last_elapsed_us=");
    Serial.print(run.elapsedUs);
    Serial.print(" last_clicks=");
    Serial.print(run.clickCount);
    Serial.print(" last_avg_cps=");
    Serial.print(run.averageCps, 2);
  }
  Serial.println();
  lastStateEmitMs_ = nowMs;
}

void App::armRelativeStart(unsigned long delayMs, unsigned long nowMs) {
  scheduleState_.armed = true;
  scheduleState_.targetMillis = nowMs + delayMs;
  scheduleState_.hasClockTarget = false;
  emitOk(String("Scheduled Grade 6 in ") + String(delayMs / 1000.0f, 1) + "s");
  if (clickController_.grade() != AppConfig::kAdaptiveGrade) {
    clickController_.setGrade(AppConfig::kAdaptiveGrade, nowMs);
  }
  refreshDisplay(nowMs);
  emitState(nowMs);
}

void App::armClockStart(uint32_t targetSecondsOfDay, unsigned long nowMs) {
  const uint32_t currentSeconds = currentSecondsOfDay(nowMs);
  const uint32_t waitSeconds = secondsUntilTarget(currentSeconds, targetSecondsOfDay);
  scheduleState_.armed = true;
  scheduleState_.targetMillis = nowMs + waitSeconds * 1000UL;
  scheduleState_.targetSecondsOfDay = targetSecondsOfDay;
  scheduleState_.hasClockTarget = true;
  emitOk(String("Scheduled Grade 6 at ") + formatTimeOfDay(targetSecondsOfDay) + " in " + waitSeconds + "s");
  if (clickController_.grade() != AppConfig::kAdaptiveGrade) {
    clickController_.setGrade(AppConfig::kAdaptiveGrade, nowMs);
  }
  refreshDisplay(nowMs);
  emitState(nowMs);
}

void App::cancelSchedule() {
  scheduleState_.armed = false;
  scheduleState_.targetMillis = 0;
  scheduleState_.targetSecondsOfDay = 0;
  scheduleState_.hasClockTarget = false;
}

void App::updateSchedule(unsigned long nowMs) {
  if (!scheduleState_.armed) {
    return;
  }

  if (nowMs < scheduleState_.targetMillis) {
    return;
  }

  scheduleState_.armed = false;
  scheduleState_.hasClockTarget = false;
  clickController_.setGrade(AppConfig::kAdaptiveGrade, nowMs);
  emitOk("Scheduled trigger fired. Starting Grade 6.");
  refreshDisplay(nowMs);
  emitState(nowMs);
}

uint32_t App::currentSecondsOfDay(unsigned long nowMs) const {
  if (!clockState_.valid) {
    return 0;
  }
  const unsigned long elapsedSeconds = (nowMs - clockState_.syncedMillis) / 1000UL;
  return (clockState_.secondsOfDay + elapsedSeconds) % (24U * 3600U);
}

unsigned long App::scheduleRemainingMs(unsigned long nowMs) const {
  if (!scheduleState_.armed || nowMs >= scheduleState_.targetMillis) {
    return 0;
  }
  return scheduleState_.targetMillis - nowMs;
}

bool App::parseTimeOfDay(const String& token, uint32_t* secondsOfDayOut) const {
  if (token.length() != 8 || token.charAt(2) != ':' || token.charAt(5) != ':') {
    return false;
  }

  const int hours = token.substring(0, 2).toInt();
  const int minutes = token.substring(3, 5).toInt();
  const int seconds = token.substring(6, 8).toInt();
  if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59 || seconds < 0 || seconds > 59) {
    return false;
  }

  *secondsOfDayOut = static_cast<uint32_t>(hours * 3600 + minutes * 60 + seconds);
  return true;
}

String App::currentClockLabel(unsigned long nowMs) const {
  if (!clockState_.valid) {
    return "unset";
  }
  return formatTimeOfDay(currentSecondsOfDay(nowMs));
}

String App::currentModeLabel() const {
  if (scheduleState_.armed) {
    return "ARMED";
  }
  if (clickController_.isRushMode()) {
    return "RUSH";
  }
  if (clickController_.grade() == 0) {
    return "STOP";
  }
  if (clickController_.isAdaptiveComplete()) {
    return "DONE";
  }
  if (clickController_.isRunning()) {
    return "RUN";
  }
  return "IDLE";
}

String App::currentStepLabel() const {
  return String(adaptiveStepToken(scheduleState_.armed ? AdaptiveStep::Idle : clickController_.adaptiveStep()));
}

void App::applyGrade(uint8_t nextGrade, unsigned long nowMs) {
  if (nextGrade == clickController_.grade()) {
    return;
  }

  if (scheduleState_.armed && nextGrade != AppConfig::kAdaptiveGrade) {
    cancelSchedule();
    emitOk("Schedule cancelled by grade change.");
  }
  clickController_.setGrade(nextGrade, nowMs);
  refreshDisplay(nowMs);
}

void App::refreshDisplay(unsigned long nowMs) {
  const DisplaySnapshot snapshot = {
      clickController_.grade(),
      clickController_.currentProfile(),
      clickController_.isRunning(),
      clickController_.isAdaptiveMode(),
      clickController_.isRushMode(),
      scheduleState_.armed && clickController_.grade() == AppConfig::kAdaptiveGrade,
      scheduleState_.armed ? scheduleRemainingMs(nowMs) : clickController_.adaptiveRemainingMs(),
      scheduleState_.armed ? AdaptiveStep::Idle : clickController_.adaptiveStep(),
  };
  display_.render(snapshot,
                  AppConfig::kTapPin,
                  AppConfig::kTapActiveLevel == HIGH,
                  AppConfig::kOledSdaPin,
                  AppConfig::kOledSclPin);
}
