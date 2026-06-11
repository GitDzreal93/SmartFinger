function buildStatusLines({ running, grade, rate, sda, scl }) {
  return [
    "SmartFinger",
    `Mode: ${grade > 0 ? "RUN" : "STOP"}`,
    `Grade: ${grade}`,
    `Rate: ${rate}/s`,
    `I2C SDA${sda} SCL${scl}`,
  ];
}

const clickProfiles = {
  0: { downMs: 0, upMs: 0, rate: 0 },
  1: { downMs: 100, upMs: 250, rate: 3 },
  2: { downMs: 90, upMs: 160, rate: 4 },
  3: { downMs: 80, upMs: 120, rate: 5 },
  4: { downMs: 70, upMs: 90, rate: 6 },
  5: { downMs: 60, upMs: 70, rate: 8 },
};

function getClickProfile(grade) {
  const profile = clickProfiles[grade];
  if (!profile) {
    throw new RangeError("grade must be between 0 and 5");
  }
  return profile;
}

module.exports = { buildStatusLines, getClickProfile };
