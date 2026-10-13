#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only
#
# Packs a JavaScript plugin folder into a .pnplugin file (a zip with
# plugin.json at its root) that users install from Settings > Plugins.
#
#   packaging/plugins/build-pnplugin.sh extras/ifs-cloud [output-dir]
#
# The file is named <id>-<version>.pnplugin. Editor backups, dot files and
# TypeScript declaration files are left out.
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "usage: $0 <plugin-folder> [output-dir]" >&2
    exit 2
fi

src="$(cd "$1" && pwd)"
out="$(mkdir -p "${2:-dist}" && cd "${2:-dist}" && pwd)"

if [[ ! -f "$src/plugin.json" ]]; then
    echo "error: $src has no plugin.json" >&2
    exit 1
fi

read_field() {
    python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))[sys.argv[2]])' "$src/plugin.json" "$1"
}
id="$(read_field id)"
version="$(read_field version)"
main="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1])).get("main","main.mjs"))' "$src/plugin.json")"

if [[ ! -f "$src/$main" ]]; then
    echo "error: main module $main is missing" >&2
    exit 1
fi
if command -v node >/dev/null 2>&1; then
    while IFS= read -r -d '' module; do
        node --check "$module"
    done < <(find "$src" -name '*.mjs' -print0)
fi

package="$out/$id-$version.pnplugin"
rm -f "$package"
(
    cd "$src"
    find . -type f ! -name '.*' ! -name '*~' ! -name '*.d.ts' ! -path '*/.*/*' -print | sed 's|^\./||' | sort \
        | zip -q -X "$package" -@
)
echo "$package"
