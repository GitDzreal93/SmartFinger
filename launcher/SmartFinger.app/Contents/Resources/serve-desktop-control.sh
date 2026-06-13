#!/bin/bash
set -euo pipefail

port="${1:-4173}"
bundle_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
desktop_dir="$bundle_dir/Resources/desktop-control"
server_script="$bundle_dir/Resources/serve-desktop-control.py"

if [[ -x "/opt/homebrew/bin/python3" ]]; then
  python_bin="/opt/homebrew/bin/python3"
else
  python_bin="/usr/bin/python3"
fi

exec "$python_bin" "$server_script" "$port" "$desktop_dir"
