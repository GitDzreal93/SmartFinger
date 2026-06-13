import assert from "node:assert/strict";

import { createTouchTestState, recordTouch, resetTouchTest } from "../touch-test/touch-test-model.mjs";

const initial = createTouchTestState();
assert.deepEqual(initial, {
  clickCount: 0,
  lastTime: null,
  intervals: [],
  lastInterval: null,
  average: null,
  minimum: null,
  maximum: null,
  standardDeviation: null,
});

const afterFirstTouch = recordTouch(initial, 100);
assert.equal(afterFirstTouch.clickCount, 1);
assert.deepEqual(afterFirstTouch.intervals, []);
assert.equal(afterFirstTouch.lastInterval, null);

const afterSecondTouch = recordTouch(afterFirstTouch, 153);
const afterThirdTouch = recordTouch(afterSecondTouch, 203);
const afterFourthTouch = recordTouch(afterThirdTouch, 263);

assert.equal(afterFourthTouch.clickCount, 4);
assert.deepEqual(afterFourthTouch.intervals, [53, 50, 60]);
assert.equal(afterFourthTouch.lastInterval, 60);
assert.equal(afterFourthTouch.average, 163 / 3);
assert.equal(afterFourthTouch.minimum, 50);
assert.equal(afterFourthTouch.maximum, 60);
assert.ok(Math.abs(afterFourthTouch.standardDeviation - 4.189935029992179) < 0.000001);

assert.deepEqual(resetTouchTest(), createTouchTestState());
