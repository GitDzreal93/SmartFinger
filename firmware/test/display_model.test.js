const assert = require("assert");
const {
  buildStatusLines,
  computeAdaptiveCycle,
  createTapScheduler,
  getClickProfile,
  normalizeTimeString,
  secondsUntilTarget,
} = require("../tools/display_model");

assert.deepStrictEqual(getClickProfile(3), {
  tapMs: 45,
  restMs: 155,
  rate: 5,
});

assert.deepStrictEqual(getClickProfile(6), {
  tapMs: 0,
  restMs: 0,
  rate: 0,
});

assert.deepStrictEqual(buildStatusLines({
  grade: 0,
  rate: 0,
  tapPin: 3,
  sda: 8,
  scl: 9,
  activeHigh: true,
}), [
  "SmartFinger Tap",
  "Mode: STOP",
  "Grade: 0",
  "Rate: 0/s",
  "Tap GPIO3 HIGH",
  "I2C SDA8 SCL9",
]);

assert.deepStrictEqual(buildStatusLines({
  grade: 6,
  rate: 0,
  tapPin: 3,
  sda: 8,
  scl: 9,
  activeHigh: true,
  running: true,
  adaptiveMode: true,
  remainingMs: 4200,
  adaptiveStep: "PRESS",
}), [
  "SmartFinger Tap",
  "Mode: RUN",
  "Grade: 6",
  "Left: 4.2s",
  "Step: PRESS",
  "Tap GPIO3 HIGH",
]);

assert.deepStrictEqual(buildStatusLines({
  grade: 6,
  rate: 0,
  tapPin: 3,
  sda: 8,
  scl: 9,
  activeHigh: true,
  running: false,
  adaptiveMode: true,
  remainingMs: 8100,
  adaptiveStep: "WAIT",
  armed: true,
}), [
  "SmartFinger Tap",
  "Mode: ARMED",
  "Grade: 6",
  "Left: 8.1s",
  "Step: WAIT",
  "Tap GPIO3 HIGH",
]);

assert.strictEqual(normalizeTimeString(9, 5, 3), "09:05:03");
assert.strictEqual(secondsUntilTarget(10 * 3600 + 0 * 60 + 0, 10 * 3600 + 0 * 60 + 5), 5);
assert.strictEqual(secondsUntilTarget(23 * 3600 + 59 * 60 + 58, 0), 2);

const scheduler = createTapScheduler();
scheduler.setGrade(2, 1000);

assert.strictEqual(scheduler.update(1194), "LOW");
assert.strictEqual(scheduler.update(1195), "HIGH");
assert.strictEqual(scheduler.tapping, true);

assert.strictEqual(scheduler.update(1249), "HIGH");
assert.strictEqual(scheduler.update(1250), "LOW");
assert.strictEqual(scheduler.tapping, false);

scheduler.setGrade(0, 2000);
assert.strictEqual(scheduler.update(3000), "LOW");
assert.strictEqual(scheduler.tapping, false);

const adaptiveScheduler = createTapScheduler();
adaptiveScheduler.setGrade(6, 0);
assert.strictEqual(adaptiveScheduler.running, true);

const adaptiveCycle = computeAdaptiveCycle({
  elapsedUs: 2500000,
  durationUs: 5000000,
  longPauseRoll: 0.7,
  gaussianCycleOffsetUs: -4000,
  requestedTapUs: 22000,
});
assert.deepStrictEqual(adaptiveCycle, {
  totalCycleUs: 28000,
  tapUs: 16000,
  releaseUs: 12000,
  longPause: false,
});

const longPauseCycle = computeAdaptiveCycle({
  elapsedUs: 200000,
  durationUs: 5000000,
  longPauseRoll: 0.01,
  longPauseUs: 91000,
  requestedTapUs: 18000,
});
assert.deepStrictEqual(longPauseCycle, {
  totalCycleUs: 91000,
  tapUs: 18000,
  releaseUs: 73000,
  longPause: true,
});

adaptiveScheduler.updateAdaptiveCycle({ totalCycleUs: 30000, tapUs: 18000, releaseUs: 12000 });
adaptiveScheduler.updateAdaptiveCycle({ totalCycleUs: 30000, tapUs: 18000, releaseUs: 12000 });
adaptiveScheduler.finishAdaptiveRun({ elapsedUs: 5020000, clickCount: 2 });
assert.strictEqual(adaptiveScheduler.running, false);
assert.strictEqual(adaptiveScheduler.locked, true);
assert.deepStrictEqual(adaptiveScheduler.lastRun, {
  elapsedUs: 5020000,
  clickCount: 2,
  averageCps: 0.4,
});

assert.throws(() => scheduler.setGrade(7, 0), /grade must be between 0 and 6/);
