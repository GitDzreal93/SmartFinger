export const PRACTICE_DURATION_MS = 10_000;
export const RUN_THRESHOLD_MS = 3_000;
export const TARGET_STORAGE_KEY = "smartfingerPracticeTarget";

export function createPracticeTarget(nowMs, durationMs = PRACTICE_DURATION_MS) {
  return nowMs + durationMs;
}

export function resolvePracticeTarget(storedTarget, nowMs, durationMs = PRACTICE_DURATION_MS) {
  const parsed = Number(storedTarget);
  return Number.isFinite(parsed) && parsed > nowMs ? parsed : createPracticeTarget(nowMs, durationMs);
}

export function remainingMs(targetMs, nowMs) {
  return Math.max(0, targetMs - nowMs);
}

export function practiceDestination(leftMs) {
  return leftMs <= RUN_THRESHOLD_MS ? "./run/" : "./reserve/";
}
