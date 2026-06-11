function buildStatusLines({ grade, rate, mosPin, sda, scl }) {
  return [
    "SmartFinger Tap",
    `Mode: ${grade > 0 ? "RUN" : "STOP"}`,
    `Grade: ${grade}`,
    `Rate: ${rate}/s`,
    `Tap GPIO${mosPin} HIGH`,
    `I2C SDA${sda} SCL${scl}`,
  ];
}

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

function getDriverConfig() {
  return {
    activeLevel: "HIGH",
    offLevel: "LOW",
    directGpioActiveLevel: "LOW",
  };
}

module.exports = { buildStatusLines, getClickProfile, getDriverConfig };
