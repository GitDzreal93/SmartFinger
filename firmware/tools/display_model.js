const clickProfiles = {
  0: { tapMs: 0, restMs: 0, rate: 0 },
  1: { tapMs: 70, restMs: 260, rate: 3 },
  2: { tapMs: 55, restMs: 195, rate: 4 },
  3: { tapMs: 45, restMs: 155, rate: 5 },
  4: { tapMs: 35, restMs: 105, rate: 7 },
  5: { tapMs: 25, restMs: 75, rate: 10 },
};

function getClickProfile(grade) {
  const profile = clickProfiles[grade];
  if (!profile) {
    throw new RangeError("grade must be between 0 and 5");
  }
  return profile;
}

function buildStatusLines({ grade, rate, tapPin, sda, scl, activeHigh }) {
  return [
    "SmartFinger Tap",
    `Mode: ${grade > 0 ? "RUN" : "STOP"}`,
    `Grade: ${grade}`,
    `Rate: ${rate}/s`,
    `Tap GPIO${tapPin} ${activeHigh ? "HIGH" : "LOW"}`,
    `I2C SDA${sda} SCL${scl}`,
  ];
}

function createTapScheduler() {
  return {
    grade: 0,
    tapping: false,
    phaseStartedMs: 0,
    level: "LOW",

    setGrade(nextGrade, nowMs) {
      if (nextGrade < 0 || nextGrade > 5) {
        throw new RangeError("grade must be between 0 and 5");
      }
      this.grade = nextGrade;
      this.tapping = false;
      this.level = "LOW";
      this.phaseStartedMs = nowMs;
    },

    update(nowMs) {
      if (this.grade === 0) {
        this.tapping = false;
        this.level = "LOW";
        return this.level;
      }

      const profile = getClickProfile(this.grade);
      if (!this.tapping && nowMs - this.phaseStartedMs >= profile.restMs) {
        this.tapping = true;
        this.level = "HIGH";
        this.phaseStartedMs = nowMs;
        return this.level;
      }

      if (this.tapping && nowMs - this.phaseStartedMs >= profile.tapMs) {
        this.tapping = false;
        this.level = "LOW";
        this.phaseStartedMs = nowMs;
      }

      return this.level;
    },
  };
}

module.exports = {
  buildStatusLines,
  createTapScheduler,
  getClickProfile,
};

