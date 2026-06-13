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

export function formatDateTimeForInput(date) {
  return [
    date.getFullYear(),
    String(date.getMonth() + 1).padStart(2, "0"),
    String(date.getDate()).padStart(2, "0"),
  ].join("-") + `T${String(date.getHours()).padStart(2, "0")}:${String(date.getMinutes()).padStart(2, "0")}`;
}

export function normalizeDateTimeLocalToSecond(value) {
  if (/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}$/.test(value)) {
    return `${value}:00`;
  }
  if (/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}$/.test(value)) {
    return value;
  }
  throw new TypeError("Date time must use YYYY-MM-DDTHH:MM or YYYY-MM-DDTHH:MM:SS");
}

export function formatDuration(durationMs) {
  const totalSeconds = Math.max(0, Math.floor(durationMs / 1000));
  const days = Math.floor(totalSeconds / 86400);
  const hours = Math.floor((totalSeconds % 86400) / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  const parts = [];

  if (days > 0) parts.push(`${days} 天`);
  if (hours > 0 || days > 0) parts.push(`${hours} 小时`);
  if (minutes > 0 || hours > 0 || days > 0) parts.push(`${minutes} 分`);
  parts.push(`${seconds} 秒`);

  return parts.join(" ");
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
    const seconds = Number(value);
    if (!Number.isFinite(seconds) || seconds <= 0) {
      throw new RangeError("Absolute date time must be in the future");
    }
    return [`TIME ${computerTime}`, `IN ${seconds}`];
  }

  throw new TypeError("Unknown schedule type");
}
