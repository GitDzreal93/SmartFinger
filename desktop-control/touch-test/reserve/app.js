import { TARGET_STORAGE_KEY, remainingMs, resolvePracticeTarget } from "../practice-model.mjs";

const countdown = document.querySelector("#reserve-countdown");
const targetMs = resolvePracticeTarget(sessionStorage.getItem(TARGET_STORAGE_KEY), Date.now());
sessionStorage.setItem(TARGET_STORAGE_KEY, String(targetMs));

document.querySelectorAll(".choices button").forEach((button) => {
  button.addEventListener("click", () => {
    const group = button.closest("div");
    group.querySelectorAll("button").forEach((item) => item.classList.remove("selected"));
    button.classList.add("selected");
  });
});

function render() {
  countdown.textContent = `距离开抢 ${(remainingMs(targetMs, Date.now()) / 1000).toFixed(1)} 秒，返回详情页继续`;
}

window.setInterval(render, 100);
render();
