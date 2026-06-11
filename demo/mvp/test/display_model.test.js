const assert = require("assert");
const { buildStatusLines, getClickProfile } = require("../tools/display_model");

const lines = buildStatusLines({
  grade: 3,
  rate: 8,
  sda: 8,
  scl: 9,
});

assert.deepStrictEqual(lines, [
  "SmartFinger",
  "Mode: RUN",
  "Grade: 3",
  "Rate: 8/s",
  "I2C SDA8 SCL9",
]);

assert.deepStrictEqual(buildStatusLines({
  grade: 0,
  rate: 0,
  sda: 8,
  scl: 9,
}), [
  "SmartFinger",
  "Mode: STOP",
  "Grade: 0",
  "Rate: 0/s",
  "I2C SDA8 SCL9",
]);

assert.deepStrictEqual(getClickProfile(0), {
  downMs: 0,
  upMs: 0,
  rate: 0,
});

assert.deepStrictEqual(getClickProfile(1), {
  downMs: 100,
  upMs: 250,
  rate: 3,
});

assert.deepStrictEqual(getClickProfile(5), {
  downMs: 60,
  upMs: 70,
  rate: 8,
});

assert.throws(() => getClickProfile(6), /grade must be between 0 and 5/);
