import assert from "node:assert/strict";

import {
  buildImmediateStartCommands,
  buildScheduledStartCommands,
  choosePreferredSerialPort,
  formatTimeForCommand,
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
    value: "22:31:15",
  }),
  ["TIME 22:30:00", "AT 22:31:15"],
);

const bluetoothPort = { getInfo: () => ({}) };
const esp32Port = { getInfo: () => ({ usbVendorId: 0x303a, usbProductId: 0x1001 }) };
assert.equal(choosePreferredSerialPort([bluetoothPort, esp32Port]), esp32Port);
assert.equal(choosePreferredSerialPort([bluetoothPort]), null);
