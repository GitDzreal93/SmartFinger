export function parseStateLine(line) {
  const payload = line.slice("STATE ".length).trim();
  const parsed = {};

  for (const entry of payload.split(" ")) {
    const separator = entry.indexOf("=");
    if (separator < 0) {
      continue;
    }
    parsed[entry.slice(0, separator)] = entry.slice(separator + 1);
  }

  return parsed;
}

export function formatTimeForCommand(date) {
  return [date.getHours(), date.getMinutes(), date.getSeconds()]
    .map((value) => String(value).padStart(2, "0"))
    .join(":");
}

export function choosePreferredSerialPort(ports) {
  return ports.find((port) => {
    const info = port.getInfo();
    return info.usbVendorId === 0x303a && info.usbProductId === 0x1001;
  }) ?? null;
}

export function buildImmediateStartCommands({ grade }) {
  const numericGrade = Number(grade);
  if (!Number.isInteger(numericGrade) || numericGrade < 1 || numericGrade > 5) {
    throw new RangeError("Immediate execution grade must be between 1-5");
  }
  return [`SELECT ${numericGrade}`, "START"];
}

export function buildScheduledStartCommands({ computerTime, type, value }) {
  if (type === "countdown") {
    const seconds = Number(value);
    if (!Number.isFinite(seconds) || seconds <= 0) {
      throw new RangeError("Countdown seconds must be positive");
    }
    return [`TIME ${computerTime}`, `IN ${seconds}`];
  }

  if (type === "absolute") {
    if (!/^\d{2}:\d{2}:\d{2}$/.test(value)) {
      throw new TypeError("Absolute time must use HH:MM:SS");
    }
    return [`TIME ${computerTime}`, `AT ${value}`];
  }

  throw new TypeError("Unknown schedule type");
}
