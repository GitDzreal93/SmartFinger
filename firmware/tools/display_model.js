const clickProfiles = {
  0: { tapMs: 0, restMs: 0, rate: 0 },
  1: { tapMs: 70, restMs: 260, rate: 3 },
  2: { tapMs: 55, restMs: 195, rate: 4 },
  3: { tapMs: 45, restMs: 155, rate: 5 },
  4: { tapMs: 35, restMs: 105, rate: 7 },
  5: { tapMs: 25, restMs: 75, rate: 10 },
  6: { tapMs: 0, restMs: 0, rate: 0 },
};

function getClickProfile(grade) {
  const profile = clickProfiles[grade];
  if (!profile) {
    throw new RangeError("grade must be between 0 and 6");
  }
  return profile;
}

function buildStatusLines({
  grade,
  rate,
  tapPin,
  sda,
  scl,
  activeHigh,
  running = grade > 0,
  adaptiveMode = false,
  rushMode = false,
  remainingMs = 0,
  adaptiveStep = "IDLE",
  armed = false,
}) {
  if (rushMode) {
    return [
      "SmartFinger Tap",
      "Mode: RUSH",
      `Grade: ${grade}`,
      "Rate: 28-33/s",
      `Step: ${adaptiveStep}`,
      `Tap GPIO${tapPin} ${activeHigh ? "HIGH" : "LOW"}`,
    ];
  }

  if (adaptiveMode) {
    return [
      "SmartFinger Tap",
      `Mode: ${armed ? "ARMED" : (running ? "RUN" : "STOP")}`,
      `Grade: ${grade}`,
      `Left: ${(remainingMs / 1000).toFixed(1)}s`,
      `Step: ${adaptiveStep}`,
      `Tap GPIO${tapPin} ${activeHigh ? "HIGH" : "LOW"}`,
    ];
  }

  return [
    "SmartFinger Tap",
    `Mode: ${running ? "RUN" : "STOP"}`,
    `Grade: ${grade}`,
    `Rate: ${rate}/s`,
    `Tap GPIO${tapPin} ${activeHigh ? "HIGH" : "LOW"}`,
    `I2C SDA${sda} SCL${scl}`,
  ];
}

function normalizeTimeString(hours, minutes, seconds) {
  return [hours, minutes, seconds].map((value) => String(value).padStart(2, "0")).join(":");
}

function secondsUntilTarget(currentSeconds, targetSeconds) {
  if (targetSeconds >= currentSeconds) {
    return targetSeconds - currentSeconds;
  }
  return 24 * 3600 - currentSeconds + targetSeconds;
}

function computeAdaptiveCycle({
  elapsedUs,
  durationUs,
  longPauseRoll,
  gaussianCycleOffsetUs = 0,
  requestedTapUs,
  longPauseUs = 80000,
}) {
  const burstDurationUs = durationUs * 0.1;
  const fatigueRatio = elapsedUs <= burstDurationUs
    ? 0
    : Math.min(1, (elapsedUs - burstDurationUs) / (durationUs - burstDurationUs));
  const fatigueBaseCycleUs = 30000 * (1 + 0.12 * fatigueRatio);

  const longPause = longPauseRoll < 0.04;
  const totalCycleUs = longPause
    ? Math.max(80000, Math.min(120000, longPauseUs))
    : Math.max(28000, Math.round(fatigueBaseCycleUs + gaussianCycleOffsetUs));

  let tapUs = Math.max(12000, requestedTapUs);
  if (totalCycleUs - tapUs < 12000) {
    tapUs = totalCycleUs - 12000;
  }

  return {
    totalCycleUs,
    tapUs,
    releaseUs: totalCycleUs - tapUs,
    longPause,
  };
}

function computeRushCycle({ totalCycleUs, tapUs }) {
  const normalizedCycleUs = Math.max(30000, Math.min(36000, totalCycleUs));
  let normalizedTapUs = Math.max(14000, Math.min(18000, tapUs));
  if (normalizedCycleUs - normalizedTapUs < 12000) {
    normalizedTapUs = normalizedCycleUs - 12000;
  }

  return {
    totalCycleUs: normalizedCycleUs,
    tapUs: normalizedTapUs,
    releaseUs: normalizedCycleUs - normalizedTapUs,
  };
}

function createTapScheduler() {
  return {
    grade: 0,
    tapping: false,
    phaseStartedMs: 0,
    level: "LOW",
    running: false,
    locked: false,
    lastRun: null,

    setGrade(nextGrade, nowMs) {
      if (nextGrade < 0 || nextGrade > 6) {
        throw new RangeError("grade must be between 0 and 6");
      }
      this.grade = nextGrade;
      this.tapping = false;
      this.level = "LOW";
      this.phaseStartedMs = nowMs;
      this.locked = false;
      this.running = nextGrade > 0;
    },

    update(nowMs) {
      if (this.grade === 0) {
        this.tapping = false;
        this.level = "LOW";
        this.running = false;
        return this.level;
      }

       if (this.grade === 6) {
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

    updateAdaptiveCycle(cycle) {
      this.running = true;
      this.tapping = false;
      this.level = "LOW";
      return cycle;
    },

    finishAdaptiveRun({ elapsedUs, clickCount }) {
      this.running = false;
      this.locked = true;
      this.tapping = false;
      this.level = "LOW";
      this.lastRun = {
        elapsedUs,
        clickCount,
        averageCps: Number((clickCount / (elapsedUs / 1000000)).toFixed(1)),
      };
    },
  };
}

module.exports = {
  buildStatusLines,
  computeAdaptiveCycle,
  computeRushCycle,
  createTapScheduler,
  getClickProfile,
  normalizeTimeString,
  secondsUntilTarget,
};
