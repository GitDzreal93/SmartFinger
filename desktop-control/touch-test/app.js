import {
  PRACTICE_DURATION_MS,
  TARGET_STORAGE_KEY,
  practiceDestination,
  remainingMs,
  resolvePracticeTarget,
} from "./practice-model.mjs";

const countdownValue = document.querySelector("#countdown-value");
const buttonHint = document.querySelector("#button-hint");
const reservedButton = document.querySelector("#reserved-button");
const resetButton = document.querySelector("#reset-practice");

let targetMs = resolvePracticeTarget(sessionStorage.getItem(TARGET_STORAGE_KEY), Date.now());
sessionStorage.setItem(TARGET_STORAGE_KEY, String(targetMs));

function currentRemainingMs() {
  return remainingMs(targetMs, Date.now());
}

function formatCountdown(leftMs) {
  const seconds = Math.floor(leftMs / 1000);
  const tenths = Math.floor((leftMs % 1000) / 100);
  return `00:${String(seconds).padStart(2, "0")}.${tenths}`;
}

function render() {
  const leftMs = currentRemainingMs();
  countdownValue.textContent = formatCountdown(leftMs);
  buttonHint.textContent = leftMs <= 3_000 ? "点击进入抢票测试" : "点击查看预约票档";
  document.body.classList.toggle("is-rush-ready", leftMs <= 3_000);
}

reservedButton.addEventListener("click", () => {
  window.location.href = practiceDestination(currentRemainingMs());
});

resetButton.addEventListener("click", () => {
  targetMs = Date.now() + PRACTICE_DURATION_MS;
  sessionStorage.setItem(TARGET_STORAGE_KEY, String(targetMs));
  render();
});

window.setInterval(render, 50);
render();
