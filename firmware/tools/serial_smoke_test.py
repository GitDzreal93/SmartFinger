#!/usr/bin/env python3

import argparse
import time

import serial


def read_lines(port, duration):
    deadline = time.monotonic() + duration
    lines = []
    while time.monotonic() < deadline:
        raw = port.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", errors="replace").strip()
        if line:
            print(line)
            lines.append(line)
    return lines


def send(port, command, wait=0.25):
    print(f"> {command}")
    port.write(f"{command}\n".encode())
    port.flush()
    return read_lines(port, wait)


def require_line(lines, description, predicate):
    if not any(predicate(line) for line in lines):
        raise AssertionError(f"Missing {description}. Received: {lines}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    args = parser.parse_args()

    with serial.Serial(args.port, 115200, timeout=0.1) as port:
        time.sleep(1.5)
        port.reset_input_buffer()

        lines = send(port, "STATUS", 0.5)
        require_line(lines, "initial STATE response", lambda line: line.startswith("STATE "))

        lines = send(port, "SELECT 2", 0.4)
        require_line(lines, "selected fixed grade", lambda line: "selected_grade=2" in line)

        lines = send(port, "START", 0.4)
        require_line(lines, "Grade 2 running state", lambda line: "mode=RUN" in line and "grade=2" in line)

        lines = send(port, "STOP", 0.4)
        require_line(lines, "stopped state", lambda line: "mode=STOP" in line and "grade=0" in line)

        lines = send(port, "IN 0.5", 0.5)
        require_line(lines, "armed schedule state", lambda line: "mode=ARMED" in line and "grade=6" in line)

        lines = read_lines(port, 6.5)
        require_line(lines, "scheduled Grade 6 run", lambda line: "mode=RUN" in line and "grade=6" in line)
        require_line(lines, "completed Grade 6 run", lambda line: "mode=DONE" in line and "last_clicks=" in line)

        print("SERIAL_SMOKE_TEST_PASS")


if __name__ == "__main__":
    main()
