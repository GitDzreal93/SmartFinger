# Ticket Practice Flow Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a realistic three-page mobile ticket practice flow before the existing capacitive touch test.

**Architecture:** A shared pure JavaScript model owns the absolute countdown target and routing decision. The detail and reservation pages use session storage to preserve that target, while the existing touch test moves unchanged into a dedicated run page.

**Tech Stack:** Static HTML, CSS, browser JavaScript modules, Node assertion tests.

---

### Task 1: Countdown Model

**Files:**
- Create: `desktop-control/touch-test/practice-model.mjs`
- Create: `desktop-control/test/practice-model.test.mjs`

- [ ] Write tests for countdown initialization, persisted target reuse, reset, remaining milliseconds, and the 3-second route boundary.
- [ ] Run `node desktop-control/test/practice-model.test.mjs` and confirm it fails because the module does not exist.
- [ ] Implement the pure countdown and routing helpers.
- [ ] Run the test and confirm it passes.

### Task 2: Detail And Reservation Pages

**Files:**
- Modify: `desktop-control/touch-test/index.html`
- Modify: `desktop-control/touch-test/app.js`
- Modify: `desktop-control/touch-test/styles.css`
- Create: `desktop-control/touch-test/reserve/index.html`
- Create: `desktop-control/touch-test/reserve/app.js`
- Create: `desktop-control/touch-test/reserve/styles.css`

- [ ] Build the event detail page with the fixed "已预约" action and visible countdown.
- [ ] Route clicks using the shared 3-second boundary.
- [ ] Build the selectable reservation page and preserve countdown state when returning.

### Task 3: Touch Test Run Page

**Files:**
- Create: `desktop-control/touch-test/run/index.html`
- Create: `desktop-control/touch-test/run/app.js`
- Create: `desktop-control/touch-test/run/styles.css`
- Create: `desktop-control/touch-test/run/touch-test-model.mjs`

- [ ] Move the current touch test UI and behavior into `/test/run/`.
- [ ] Add a return-to-practice link.

### Task 4: Package And Verify

**Files:**
- Mirror changed files under `launcher/SmartFinger.app/Contents/Resources/desktop-control/touch-test/`

- [ ] Run all Node tests and syntax checks.
- [ ] Sync the packaged app copy.
- [ ] Verify `/test/`, `/test/reserve/`, and `/test/run/` in a mobile-sized browser.
- [ ] Verify both `>3 seconds` and `<=3 seconds` routing branches.
