import assert from "node:assert/strict";

import {
  buildImmediateStartCommands,
  buildScheduledStartCommands,
  choosePreferredSerialPort,
  formatDateTimeForInput,
  formatDuration,
  formatTimeForCommand,
  normalizeDateTimeLocalToSecond,
  parseStateLine,
} from "../control-model.mjs";

assert.deepEqual(
  parseStateLine("STATE mode=ARMED grade=6 selected_grade=3 running=0 armed=1 step=WAIT left_ms=4200 clock=22:30:00"),
  {
    mode: "ARMED",
    grade: "6",
    selected_grade: "3",
    running: "0",
    armed: "1",
    step: "WAIT",
    left_ms: "4200",
    clock: "22:30:00",
  },
);

assert.equal(formatTimeForCommand(new Date(2026, 5, 11, 9, 5, 3)), "09:05:03");

assert.deepEqual(buildImmediateStartCommands({ grade: 3 }), ["SELECT 3", "START"]);
assert.throws(() => buildImmediateStartCommands({ grade: 6 }), /1-5/);

assert.deepEqual(
  buildScheduledStartCommands({
    computerTime: "22:30:00",
    type: "countdown",
    value: "5",
  }),
  ["TIME 22:30:00", "IN 5"],
);

assert.deepEqual(
  buildScheduledStartCommands({
    computerTime: "22:30:00",
    type: "absolute",
    value: "45",
  }),
  ["TIME 22:30:00", "IN 45"],
);

assert.equal(normalizeDateTimeLocalToSecond("2026-06-15T13:38"), "2026-06-15T13:38:00");
assert.equal(formatDateTimeForInput(new Date(2026, 5, 15, 13, 38, 45)), "2026-06-15T13:38");
assert.equal(formatDuration(0), "0 秒");
assert.equal(formatDuration(65_000), "1 分 5 秒");
assert.equal(formatDuration(90_061_000), "1 天 1 小时 1 分 1 秒");

const bluetoothPort = { getInfo: () => ({}) };
const esp32Port = { getInfo: () => ({ usbVendorId: 0x303a, usbProductId: 0x1001 }) };
assert.equal(choosePreferredSerialPort([bluetoothPort, esp32Port]), esp32Port);
assert.equal(choosePreferredSerialPort([bluetoothPort]), null);
