#!/usr/bin/env python3

import os
import sys
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


class QuietHandler(SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        sys.stderr.write("%s - - [%s] %s\n" % (self.client_address[0], self.log_date_time_string(), format % args))


def main():
    if len(sys.argv) < 3:
        raise SystemExit("usage: serve-desktop-control.py PORT DESKTOP_DIR")

    port = int(sys.argv[1])
    desktop_dir = Path(sys.argv[2]).resolve()
    handler = partial(QuietHandler, directory=str(desktop_dir))
    server = ThreadingHTTPServer(("127.0.0.1", port), handler)
    server.serve_forever()


if __name__ == "__main__":
    main()
