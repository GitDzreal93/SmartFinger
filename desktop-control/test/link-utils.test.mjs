import assert from "node:assert/strict";

import { buildTouchTestUrl } from "../link-utils.mjs";

assert.equal(buildTouchTestUrl("192.168.31.160", 4173), "http://192.168.31.160:4173/test/");
assert.equal(buildTouchTestUrl("localhost", 4173), "http://localhost:4173/test/");
