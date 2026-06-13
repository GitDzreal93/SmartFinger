export function createTouchTestState() {
  return {
    clickCount: 0,
    lastTime: null,
    intervals: [],
    lastInterval: null,
    average: null,
    minimum: null,
    maximum: null,
    standardDeviation: null,
  };
}

export function recordTouch(state, now) {
  const clickCount = state.clickCount + 1;

  if (state.lastTime === null) {
    return { ...state, clickCount, lastTime: now };
  }

  const lastInterval = now - state.lastTime;
  const intervals = [...state.intervals, lastInterval];
  const average = intervals.reduce((sum, interval) => sum + interval, 0) / intervals.length;
  const variance = intervals.reduce((sum, interval) => sum + (interval - average) ** 2, 0) / intervals.length;

  return {
    clickCount,
    lastTime: now,
    intervals,
    lastInterval,
    average,
    minimum: Math.min(...intervals),
    maximum: Math.max(...intervals),
    standardDeviation: Math.sqrt(variance),
  };
}

export function resetTouchTest() {
  return createTouchTestState();
}
