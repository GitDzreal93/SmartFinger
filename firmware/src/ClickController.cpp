#include "ClickController.h"

ClickController::ClickController(int tapPin, int activeLevel, int idleLevel)
    : tapPin_(tapPin), activeLevel_(activeLevel), idleLevel_(idleLevel) {}

void ClickController::begin() {
  pinMode(tapPin_, OUTPUT);
  releaseTap();
}

void ClickController::setGrade(uint8_t nextGrade, unsigned long nowMs) {
  if (nextGrade > AppConfig::kMaxGrade) {
    nextGrade = AppConfig::kMaxGrade;
  }

  grade_ = nextGrade;
  releaseTap();
  phaseStartedMs_ = nowMs;
}

void ClickController::update(unsigned long nowMs) {
  if (grade_ == 0) {
    releaseTap();
    return;
  }

  const ClickProfile& profile = currentProfile();

  if (!tapping_ && nowMs - phaseStartedMs_ >= profile.restMs) {
    startTap();
    phaseStartedMs_ = nowMs;
    return;
  }

  if (tapping_ && nowMs - phaseStartedMs_ >= profile.tapMs) {
    releaseTap();
    phaseStartedMs_ = nowMs;
  }
}

uint8_t ClickController::grade() const {
  return grade_;
}

bool ClickController::isRunning() const {
  return grade_ > 0;
}

bool ClickController::isTapping() const {
  return tapping_;
}

const ClickProfile& ClickController::currentProfile() const {
  return AppConfig::profileForGrade(grade_);
}

void ClickController::releaseTap() {
  digitalWrite(tapPin_, idleLevel_);
  tapping_ = false;
}

void ClickController::startTap() {
  digitalWrite(tapPin_, activeLevel_);
  tapping_ = true;
}

