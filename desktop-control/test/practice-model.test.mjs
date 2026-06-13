import assert from "node:assert/strict";

import {
  createPracticeTarget,
  practiceDestination,
  remainingMs,
  resolvePracticeTarget,
} from "../touch-test/practice-model.mjs";

assert.equal(createPracticeTarget(1_000, 10_000), 11_000);
assert.equal(resolvePracticeTarget("11000", 1_000, 10_000), 11_000);
assert.equal(resolvePracticeTarget("expired", 1_000, 10_000), 11_000);
assert.equal(remainingMs(11_000, 4_500), 6_500);
assert.equal(remainingMs(11_000, 12_000), 0);
assert.equal(practiceDestination(3_001), "./reserve/");
assert.equal(practiceDestination(3_000), "./run/");
assert.equal(practiceDestination(0), "./run/");
