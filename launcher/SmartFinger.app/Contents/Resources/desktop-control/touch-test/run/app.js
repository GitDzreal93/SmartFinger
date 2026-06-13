import { createTouchTestState, recordTouch, resetTouchTest } from "./touch-test-model.mjs";

const touchPad = document.querySelector("#touch-pad");
const resetButton = document.querySelector("#reset-button");
const padValue = document.querySelector("#pad-value");
const padHint = document.querySelector("#pad-hint");
const fields = {
  lastInterval: document.querySelector("#last-interval"),
  clickCount: document.querySelector("#click-count"),
  average: document.querySelector("#average-interval"),
  minimum: document.querySelector("#minimum-interval"),
  maximum: document.querySelector("#maximum-interval"),
  standardDeviation: document.querySelector("#standard-deviation"),
};

let state = createTouchTestState();

function formatInterval(value) {
  return value === null ? "--" : `${value.toFixed(2)} ms`;
}

function render() {
  fields.lastInterval.textContent = formatInterval(state.lastInterval);
  fields.clickCount.textContent = String(state.clickCount);
  fields.average.textContent = formatInterval(state.average);
  fields.minimum.textContent = formatInterval(state.minimum);
  fields.maximum.textContent = formatInterval(state.maximum);
  fields.standardDeviation.textContent = formatInterval(state.standardDeviation);

  if (state.lastInterval === null) {
    padValue.textContent = "立即抢票";
    padHint.textContent = state.clickCount === 0 ? "等待电容头点击" : "已命中首次点击";
    return;
  }

  padValue.textContent = "立即抢票";
  padHint.textContent = `命中 ${state.clickCount} 次 · 最近 ${formatInterval(state.lastInterval)}`;
}

touchPad.addEventListener("pointerdown", (event) => {
  event.preventDefault();
  state = recordTouch(state, performance.now());
  render();
});

resetButton.addEventListener("click", () => {
  state = resetTouchTest();
  render();
});

document.addEventListener("contextmenu", (event) => event.preventDefault());
render();
