import {
  buildImmediateStartCommands,
  buildScheduledStartCommands,
  choosePreferredSerialPort,
  formatTimeForCommand,
  parseStateLine,
} from "./control-model.mjs";

const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => Array.from(document.querySelectorAll(selector));
const wait = (ms) => new Promise((resolve) => window.setTimeout(resolve, ms));
const encoder = new TextEncoder();

const ui = {
  computerClock: $("#computer-clock"),
  deviceStatus: $("#device-status"),
  connectButton: $("#connect-button"),
  disconnectButton: $("#disconnect-button"),
  connectionHealth: $("#connection-health"),
  workspace: $("#workspace"),
  modeButtons: $$(".mode-button"),
  immediatePanel: $("#immediate-panel"),
  scheduledPanel: $("#scheduled-panel"),
  gradeCards: $$(".grade-card"),
  immediateStartButton: $("#immediate-start-button"),
  immediateStopButton: $("#immediate-stop-button"),
  immediateState: $("#immediate-state"),
  tapMsInput: $("#tap-ms-input"),
  restMsInput: $("#rest-ms-input"),
  applyProfileButton: $("#apply-profile-button"),
  clearProfileButton: $("#clear-profile-button"),
  delaySecondsInput: $("#delay-seconds-input"),
  targetTimeInput: $("#target-time-input"),
  countdownArmButton: $("#countdown-arm-button"),
  absoluteArmButton: $("#absolute-arm-button"),
  cancelScheduleButton: $("#cancel-schedule-button"),
  scheduledState: $("#scheduled-state"),
  countdownValue: $("#countdown-value"),
  scheduleStep: $("#schedule-step"),
  stateMode: $("#state-mode"),
  stateGrade: $("#state-grade"),
  selectedGrade: $("#selected-grade"),
  lastCps: $("#last-cps"),
  refreshStatusButton: $("#refresh-status-button"),
  clearLogButton: $("#clear-log-button"),
  logOutput: $("#log-output"),
};

const state = {
  port: null,
  reader: null,
  writer: null,
  buffer: "",
  connected: false,
  deviceResponsive: false,
  selectedGrade: 3,
  statusTimer: null,
  readTask: null,
};

function log(message, kind = "info") {
  const time = new Date().toLocaleTimeString("zh-CN", { hour12: false });
  ui.logOutput.textContent += `[${time}] ${kind === "out" ? "> " : ""}${message}\n`;
  ui.logOutput.scrollTop = ui.logOutput.scrollHeight;
}

function renderConnection() {
  const ready = state.connected && state.deviceResponsive;
  ui.workspace.classList.toggle("is-locked", !ready);
  ui.connectButton.disabled = state.connected;
  ui.disconnectButton.disabled = !state.connected;
  ui.deviceStatus.classList.toggle("is-online", ready);
  ui.deviceStatus.classList.toggle("is-error", state.connected && !ready);
  ui.deviceStatus.textContent = ready ? "设备已连接" : state.connected ? "设备无响应" : "设备未连接";
  ui.connectionHealth.textContent = ready
    ? "通信正常，已收到设备状态。请选择执行方式。"
    : state.connected
      ? "串口已打开，但尚未收到设备回包。"
      : "等待连接";
}

function renderDeviceState(device) {
  state.deviceResponsive = true;
  renderConnection();

  const leftMs = Number(device.left_ms ?? 0);
  const mode = device.mode ?? "STOP";
  const grade = device.grade ?? "0";
  const selectedGrade = Number(device.selected_grade ?? state.selectedGrade);
  state.selectedGrade = selectedGrade;

  ui.stateMode.textContent = mode;
  ui.stateGrade.textContent = grade;
  ui.selectedGrade.textContent = String(selectedGrade);
  ui.lastCps.textContent = device.last_avg_cps ?? "--";
  ui.immediateState.textContent = mode === "RUN" && Number(grade) >= 1 && Number(grade) <= 5
    ? `${grade} 档运行中`
    : "未运行";
  ui.scheduledState.textContent = mode === "ARMED" ? "已武装" : mode === "RUN" && grade === "6" ? "执行中" : mode === "DONE" ? "已完成" : "未武装";
  ui.countdownValue.textContent = `${(leftMs / 1000).toFixed(1)}s`;
  ui.scheduleStep.textContent = device.step ?? "WAIT";

  ui.gradeCards.forEach((card) => {
    card.classList.toggle("is-selected", Number(card.dataset.grade) === selectedGrade);
  });
}

function handleLine(line) {
  if (!line) return;
  log(line);
  if (line.startsWith("STATE ")) {
    renderDeviceState(parseStateLine(line));
  }
}

async function readLoop() {
  const decoder = new TextDecoder();
  try {
    while (state.reader) {
      const { value, done } = await state.reader.read();
      if (done) break;
      state.buffer += decoder.decode(value, { stream: true });
      const lines = state.buffer.split(/\r?\n/);
      state.buffer = lines.pop() ?? "";
      lines.forEach(handleLine);
    }
  } catch (error) {
    log(`串口读取失败：${error.message}`);
  }
}

async function send(command, { quiet = false } = {}) {
  if (!state.writer) throw new Error("串口未连接");
  if (!quiet) log(command, "out");
  await state.writer.write(encoder.encode(`${command}\n`));
}

async function sendSequence(commands) {
  for (const command of commands) {
    await send(command);
    await wait(60);
  }
}

async function syncClock() {
  await send(`TIME ${formatTimeForCommand(new Date())}`);
}

async function connect() {
  if (!("serial" in navigator)) throw new Error("当前浏览器不支持 Web Serial，请使用 Chrome 或 Edge");
  const authorizedPorts = await navigator.serial.getPorts();
  state.port = choosePreferredSerialPort(authorizedPorts);
  if (state.port) {
    log("正在重新连接已授权的 ESP32 串口...");
  } else {
    state.port = await navigator.serial.requestPort({
      filters: [{ usbVendorId: 0x303a, usbProductId: 0x1001 }],
    });
  }
  await state.port.open({ baudRate: 115200 });
  state.reader = state.port.readable.getReader();
  state.writer = state.port.writable.getWriter();
  state.connected = true;
  state.deviceResponsive = false;
  renderConnection();
  state.readTask = readLoop();
  log("串口已打开，等待设备启动...");
  await wait(1200);
  await syncClock();
  await send("STATUS");
  await wait(1200);
  if (!state.deviceResponsive) {
    throw new Error("设备没有回包。请确认已烧录最新版固件，然后断开重连");
  }
  state.statusTimer = window.setInterval(() => send("STATUS", { quiet: true }).catch(() => {}), 500);
}

async function disconnect({ quiet = false } = {}) {
  if (state.statusTimer) window.clearInterval(state.statusTimer);
  state.statusTimer = null;
  if (state.reader) {
    await state.reader.cancel().catch(() => {});
    state.reader.releaseLock();
  }
  if (state.writer) state.writer.releaseLock();
  if (state.port) await state.port.close().catch(() => {});
  state.port = null;
  state.reader = null;
  state.writer = null;
  state.connected = false;
  state.deviceResponsive = false;
  renderConnection();
  if (!quiet) log("串口已断开。");
}

async function connectWithCleanup() {
  try {
    await connect();
  } catch (error) {
    await disconnect({ quiet: true });
    throw error;
  }
}

function selectMode(mode) {
  const immediate = mode === "immediate";
  ui.immediatePanel.classList.toggle("is-hidden", !immediate);
  ui.scheduledPanel.classList.toggle("is-hidden", immediate);
  ui.modeButtons.forEach((button) => button.classList.toggle("is-active", button.dataset.mode === mode));
}

function bindEvents() {
  ui.connectButton.addEventListener("click", () => connectWithCleanup().catch((error) => {
    log(`连接失败：${error.message}`);
  }));
  ui.disconnectButton.addEventListener("click", () => disconnect().catch((error) => log(`断开失败：${error.message}`)));

  ui.modeButtons.forEach((button) => button.addEventListener("click", () => selectMode(button.dataset.mode)));
  ui.gradeCards.forEach((card) => card.addEventListener("click", async () => {
    state.selectedGrade = Number(card.dataset.grade);
    await send(`SELECT ${state.selectedGrade}`);
    await send("STATUS", { quiet: true });
  }));

  ui.immediateStartButton.addEventListener("click", () => sendSequence(buildImmediateStartCommands({ grade: state.selectedGrade })).catch((error) => log(`启动失败：${error.message}`)));
  ui.immediateStopButton.addEventListener("click", () => send("STOP").catch((error) => log(`停止失败：${error.message}`)));
  ui.applyProfileButton.addEventListener("click", () => send(`PROFILE ${Number(ui.tapMsInput.value)} ${Number(ui.restMsInput.value)}`).catch((error) => log(`设置频率失败：${error.message}`)));
  ui.clearProfileButton.addEventListener("click", () => send("PROFILE CLEAR").catch((error) => log(`恢复预设失败：${error.message}`)));

  ui.countdownArmButton.addEventListener("click", () => sendSequence(buildScheduledStartCommands({
    computerTime: formatTimeForCommand(new Date()),
    type: "countdown",
    value: ui.delaySecondsInput.value,
  })).catch((error) => log(`武装失败：${error.message}`)));

  ui.absoluteArmButton.addEventListener("click", () => {
    const value = ui.targetTimeInput.value.length === 5 ? `${ui.targetTimeInput.value}:00` : ui.targetTimeInput.value;
    return sendSequence(buildScheduledStartCommands({
      computerTime: formatTimeForCommand(new Date()),
      type: "absolute",
      value,
    })).catch((error) => log(`武装失败：${error.message}`));
  });

  ui.cancelScheduleButton.addEventListener("click", () => sendSequence(["CANCEL", "STOP"]).catch((error) => log(`取消失败：${error.message}`)));
  ui.refreshStatusButton.addEventListener("click", () => send("STATUS").catch((error) => log(`刷新失败：${error.message}`)));
  ui.clearLogButton.addEventListener("click", () => { ui.logOutput.textContent = ""; });

  window.addEventListener("pagehide", () => {
    disconnect({ quiet: true }).catch(() => {});
  });

  navigator.serial?.addEventListener("disconnect", () => {
    disconnect({ quiet: true }).catch(() => {});
  });
}

function init() {
  renderConnection();
  bindEvents();
  ui.targetTimeInput.value = formatTimeForCommand(new Date(Date.now() + 60_000));
  window.setInterval(() => {
    ui.computerClock.textContent = `电脑时间 ${formatTimeForCommand(new Date())}`;
  }, 250);
}

init();
