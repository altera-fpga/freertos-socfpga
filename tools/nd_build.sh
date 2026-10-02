#!/bin/bash
# Generate Natural Docs HTML for the SoCFPGA bare-metal software stack.
#
# Natural Docs 1.4 is provided by the ARC resource 'naturaldocs'. Run this
# script through it:
#
#     arc shell naturaldocs -- tools/nd_build.sh
#
# or simply use 'make docs', which wraps the arc invocation.
 
set -euo pipefail
 
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"
 
ND_CONFIG_DIR="docs/naturaldocs/project"
ND_HTML_DIR="docs/naturaldocs/html"
 
# Documented source layers, ordered so the generated menu reads from the
# application-facing surface down to the hardware.
SOURCE_DIRS=(FreeRTOS/Demo/SOCFPGA samples fcs rsu osal drivers)
 
if ! command -v NaturalDocs >/dev/null 2>&1; then
    echo "error: 'NaturalDocs' not found in PATH." >&2
    echo "Run inside the ARC resource:  arc shell naturaldocs -- tools/nd_build.sh" >&2
    exit 1
fi
 
mkdir -p "$ND_CONFIG_DIR" "$ND_HTML_DIR"
 
args=()
for dir in "${SOURCE_DIRS[@]}"; do
    [ -d "$dir" ] && args+=(-i "$dir")
done
 
if [ ${#args[@]} -eq 0 ]; then
    echo "error: no source directories found under $REPO_ROOT" >&2
    exit 1
fi
 
echo "Natural Docs: $(NaturalDocs -h 2>&1 | head -1)"
echo "Sources:      ${SOURCE_DIRS[*]}"
echo "Output:       $ND_HTML_DIR"
echo
 
NaturalDocs "${args[@]}" -o HTML "$ND_HTML_DIR" -p "$ND_CONFIG_DIR" "$@"
 
# Natural Docs flattens small directories and titles menu entries after the
# page heading. Rebuild the menu as a plain mirror of the source tree with
# filenames as labels, then regenerate if that changed anything.
if python3 tools/nd_menu.py "$ND_CONFIG_DIR/Menu.txt" "$REPO_ROOT" | grep -q '(changed)'; then
    # -r is needed because the menu is embedded in every page and Natural Docs
    # will not notice that Menu.txt was edited behind its back.
    echo "Menu entries renamed to filenames; regenerating."
    NaturalDocs "${args[@]}" -o HTML "$ND_HTML_DIR" -p "$ND_CONFIG_DIR" -r "$@"
fi
 
echo
echo "Documentation written to $ND_HTML_DIR/index.html"
