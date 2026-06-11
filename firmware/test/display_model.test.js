const assert = require("assert");
const {
  buildStatusLines,
  createTapScheduler,
  getClickProfile,
} = require("../tools/display_model");

assert.deepStrictEqual(getClickProfile(3), {
  tapMs: 45,
  restMs: 155,
  rate: 5,
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

assert.throws(() => scheduler.setGrade(6, 0), /grade must be between 0 and 5/);

