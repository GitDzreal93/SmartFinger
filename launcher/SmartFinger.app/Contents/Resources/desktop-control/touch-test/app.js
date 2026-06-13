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
    padValue.textContent = state.clickCount === 0 ? "等待点击" : "已记录首次点击";
    padHint.textContent = state.clickCount === 0 ? "首次点击后开始计算间隔" : "再次点击即可得到第一个间隔";
    return;
  }

  padValue.textContent = formatInterval(state.lastInterval);
  padHint.textContent = `已记录 ${state.clickCount} 次点击`;
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
