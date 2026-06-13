import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const detailHtml = readFileSync(new URL("../touch-test/index.html", import.meta.url), "utf8");
const runHtml = readFileSync(new URL("../touch-test/run/index.html", import.meta.url), "utf8");

assert.match(detailHtml, /id="reset-practice"/);
assert.match(runHtml, /id="touch-pad" class="purchase-button"/);
assert.doesNotMatch(runHtml, /class="touch-pad"/);
assert.ok(runHtml.indexOf('class="stats-panel"') < runHtml.indexOf('id="touch-pad"'));
