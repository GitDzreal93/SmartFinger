# SmartFinger Desktop Control GUI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a desktop-only local control GUI that connects to the ESP32 over Web Serial, syncs computer time automatically, and lets the user control grade, timing, and scheduled starts without using the rotary encoder.

**Architecture:** Keep `firmware/` as the source of truth for click behavior and scheduling. Add a small serial command/status protocol for GUI control. Build a static `desktop-control/` web app that runs on localhost, uses Web Serial in the browser, sends commands, and renders device state/logs.

**Tech Stack:** Arduino C++, static HTML/CSS/JavaScript, Web Serial API, PlatformIO

---

### Task 1: Extend firmware command surface

**Files:**
- Modify: `firmware/include/App.h`
- Modify: `firmware/src/App.cpp`
- Modify: `firmware/include/ClickTypes.h`

- [ ] Add command handlers for GUI-driven control such as `GRADE`, `START`, `STOP`, `PROFILE`, and `STATUS`.
- [ ] Reuse existing `TIME`, `AT`, `IN`, and `CANCEL` behavior, but make all command responses deterministic for GUI parsing.
- [ ] Keep rotary encoder support compatible, while letting GUI commands be the primary control path.

### Task 2: Add structured device state output

**Files:**
- Modify: `firmware/include/ClickController.h`
- Modify: `firmware/src/ClickController.cpp`
- Modify: `firmware/src/App.cpp`
- Modify: `firmware/src/StatusDisplay.cpp`

- [ ] Emit machine-readable `STATE ...` lines that include grade, mode, armed/running flags, adaptive step, remaining time, current timing values, and clock info.
- [ ] Make sure the GUI can trigger explicit status refreshes through `STATUS`.
- [ ] Preserve the OLED behavior and continue to show waiting/running states for grade 6.

### Task 3: Support GUI-driven frequency control

**Files:**
- Modify: `firmware/include/AppConfig.h`
- Modify: `firmware/include/ClickController.h`
- Modify: `firmware/src/ClickController.cpp`
- Modify: `firmware/src/App.cpp`

- [ ] Add a configurable fixed-profile override so the GUI can set custom `tapMs` and `restMs` without being limited to the preset grades.
- [ ] Ensure the selected/custom timing is reflected in device status output.
- [ ] Keep adaptive grade 6 behavior separate from fixed-profile overrides.

### Task 4: Build the local desktop GUI

**Files:**
- Create: `desktop-control/index.html`
- Create: `desktop-control/styles.css`
- Create: `desktop-control/app.js`
- Create: `desktop-control/README.md`

- [ ] Build a single-page control console with sections for serial connection, time sync, grade selection, custom timing, immediate actions, scheduled starts, and logs.
- [ ] Use browser Web Serial to connect to the board and stream logs.
- [ ] Automatically sync the computer time before absolute-time scheduling and on initial connection.
- [ ] Parse `STATE ...` lines to keep the UI in sync with the device.

### Task 5: Document and verify the workflow

**Files:**
- Modify: `firmware/README.md`
- Modify: `.gitignore`

- [ ] Add instructions for launching the local GUI and connecting the device.
- [ ] Mention browser requirements for Web Serial.
- [ ] Verify with `node firmware/test/display_model.test.js`, `cd firmware && pio run`, and a local static server launch for `desktop-control/`.

