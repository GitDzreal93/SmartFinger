const assert = require("assert");
const {
  buildStatusLines,
  getClickProfile,
  getDriverConfig,
} = require("../tools/display_model");

assert.deepStrictEqual(buildStatusLines({
  grade: 0,
  rate: 0,
  mosPin: 3,
  sda: 8,
  scl: 9,
}), [
  "SmartFinger Tap",
  "Mode: STOP",
  "Grade: 0",
  "Rate: 0/s",
  "Tap GPIO3 HIGH",
  "I2C SDA8 SCL9",
]);

assert.deepStrictEqual(buildStatusLines({
  grade: 3,
  rate: 5,
  mosPin: 3,
  sda: 8,
  scl: 9,
}), [
  "SmartFinger Tap",
  "Mode: RUN",
  "Grade: 3",
  "Rate: 5/s",
  "Tap GPIO3 HIGH",
  "I2C SDA8 SCL9",
]);

assert.deepStrictEqual(getClickProfile(0), {
  tapMs: 0,
  restMs: 0,
  rate: 0,
});

assert.deepStrictEqual(getClickProfile(1), {
  tapMs: 70,
  restMs: 260,
  rate: 3,
});

assert.deepStrictEqual(getClickProfile(5), {
  tapMs: 25,
  restMs: 75,
  rate: 10,
});

assert.throws(() => getClickProfile(6), /grade must be between 0 and 5/);

assert.deepStrictEqual(getDriverConfig(), {
  activeLevel: "HIGH",
  offLevel: "LOW",
  directGpioActiveLevel: "LOW",
});
