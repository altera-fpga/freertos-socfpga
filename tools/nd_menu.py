#!/usr/bin/env python3
"""Rewrite the Natural Docs menu as a mirror of the directory tree.

Natural Docs groups the sidebar by directory but flattens any directory that
holds only a file or two, so single-file components such as drivers/ecc end up
loose beside the groups. This rebuilds the menu so every file sits in a group
named after the directory it lives in, exactly as in the source tree.

It also pins each entry to its filename with Natural Docs' "no auto-title"
marker, because page headings carry readable titles (see tools/nd_titles.py)
and Natural Docs would otherwise reuse those in the sidebar too.

Only entries Natural Docs itself put in the menu are reorganized; the header,
the index section and the trailing data lines are preserved verbatim.

    python3 tools/nd_menu.py docs/naturaldocs/project/Menu.txt [repo_root]

Prints "changed" or "unchanged" so the build knows whether to regenerate.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from nd_titles import pretty

ENTRY = re.compile(
    r"^([ \t]*)File:[ \t]+(.*?)[ \t]*\((?:no auto-title,[ \t]*)?(.*?)\)[ \t]*$")
INDENT = "   "


def add(tree, parts, path):
    node = tree
    for part in parts:
        node = node.setdefault("dirs", {}).setdefault(part, {})
    node.setdefault("files", []).append(path)


def render(node, depth, out):
    pad = INDENT * depth
    for name in sorted(node.get("dirs", {}), key=str.lower):
        child = node["dirs"][name]
        out.append("%sGroup: %s  {\n" % (pad, pretty(name) or name))
        out.append("\n")
        render(child, depth + 1, out)
        out.append("%s}  # Group: %s\n" % (INDENT * (depth + 1), pretty(name) or name))
        out.append("\n")
    # Files come last and get no trailing blank line: Natural Docs strips one
    # that sits directly above a closing brace, which would make every build
    # look like the menu had changed.
    for path in sorted(node.get("files", []), key=lambda p: os.path.basename(p).lower()):
        out.append("%sFile: %s  (no auto-title, %s)\n"
                   % (pad, os.path.basename(path), path))


def main(argv):
    if len(argv) < 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    menu = argv[1]
    root = os.path.abspath(argv[2]) if len(argv) > 2 else os.getcwd()
    if not os.path.exists(menu):
        print("no menu at %s" % menu, file=sys.stderr)
        return 0

    with open(menu, encoding="utf-8", errors="surrogateescape") as fh:
        lines = fh.readlines()

    # Split the file into the part we own and the parts we must not touch.
    first, last = None, None
    for i, line in enumerate(lines):
        if re.match(r"^(Group|File):", line):
            if first is None:
                first = i
            if line.startswith("Group: Index"):
                last = i
                break
    if first is None:
        print("menu has no entries yet (unchanged)")
        return 0
    tail = last if last is not None else len(lines)

    tree, count = {}, 0
    for line in lines[first:tail]:
        m = ENTRY.match(line.rstrip("\n"))
        if not m:
            continue
        path = m.group(3)
        rel = os.path.relpath(path, root)
        if rel.startswith(".."):
            rel = os.path.basename(path)
        add(tree, os.path.dirname(rel).replace("\\", "/").split("/") if
            os.path.dirname(rel) else [], path)
        count += 1

    body = []
    render(tree, 0, body)
    while body and body[-1] == "\n":
        body.pop()
    body.append("\n")

    new = lines[:first] + body + lines[tail:]
    changed = new != lines
    if changed:
        with open(menu, "w", encoding="utf-8", errors="surrogateescape") as fh:
            fh.writelines(new)

    print("menu rebuilt from directory tree: %d files (%s)"
          % (count, "changed" if changed else "unchanged"))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
