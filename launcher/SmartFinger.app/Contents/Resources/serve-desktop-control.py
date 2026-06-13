#!/usr/bin/env python3

import json
import socket
import sys
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit


def get_local_ip():
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        try:
          sock.connect(("8.8.8.8", 80))
          ip = sock.getsockname()[0]
          if ip and not ip.startswith("127."):
              return ip
        except OSError:
            pass
    return "127.0.0.1"


class QuietHandler(SimpleHTTPRequestHandler):
    runtime_info = {"ip": "127.0.0.1", "port": 4173, "touch_test_url": "http://127.0.0.1:4173/test/"}

    def log_message(self, format, *args):
        sys.stderr.write("%s - - [%s] %s\n" % (self.client_address[0], self.log_date_time_string(), format % args))

    def translate_path(self, path):
        parsed = urlsplit(path)
        rewritten = parsed.path
        if rewritten == "/test":
            rewritten = "/test/"
        if rewritten.startswith("/test/"):
            rewritten = "/touch-test/" + rewritten[len("/test/"):]
        return super().translate_path(rewritten)

    def do_GET(self):
        if self.path == "/__smartfinger__/runtime.json":
            payload = json.dumps(self.runtime_info).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if self.path == "/test":
            self.send_response(301)
            self.send_header("Location", "/test/")
            self.end_headers()
            return

        super().do_GET()


def main():
    if len(sys.argv) < 3:
        raise SystemExit("usage: serve-desktop-control.py PORT DESKTOP_DIR")

    port = int(sys.argv[1])
    desktop_dir = Path(sys.argv[2]).resolve()
    local_ip = get_local_ip()
    runtime_info = {"ip": local_ip, "port": port, "touch_test_url": f"http://{local_ip}:{port}/test/"}
    handler = partial(QuietHandler, directory=str(desktop_dir))
    QuietHandler.runtime_info = runtime_info
    server = ThreadingHTTPServer(("0.0.0.0", port), handler)
    server.serve_forever()


if __name__ == "__main__":
    main()
