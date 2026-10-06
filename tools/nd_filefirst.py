#!/usr/bin/env python3
"""Move the File topic to the top of the file.

Natural Docs titles a page after the first topic it finds. A `File:` topic
sitting below the includes works only for as long as nothing is documented
above it: add a topic to a type declared near the top and the page silently
loses its title and its description.

This lifts the `File:` topic above every other topic, placing it after the
licence header the file already opens with.

    python3 tools/nd_filefirst.py --report drivers samples ...
    python3 tools/nd_filefirst.py --apply  drivers samples ...
"""

import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dox2nd
import nd_macros


def block_of(spans, position):
    """The comment block containing a topic."""
    return next(((s, e) for _, s, e in spans if s <= position < e), None)


def landing(text, spans):
    """Where the File topic belongs: after the opening licence comment."""
    cut = 0
    for _, s, e in spans:
        if text[cut:s].strip():
            break
        cut = e
        while text[cut:cut + 1] in (" ", "\t"):
            cut += 1
        if text[cut:cut + 1] == "\n":
            cut += 1
    return cut


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--report", action="store_true")
    args = ap.parse_args()

    moved = refused = 0
    for path in dox2nd.gather(args.paths):
        if not path.endswith((".c", ".h", ".hpp")):
            continue
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        topics = list(nd_macros.TOPIC.finditer(text))
        if not topics or topics[0].group(2) == "File":
            continue
        first_file = next((m for m in topics if m.group(2) == "File"), None)
        if not first_file:
            continue

        spans = dox2nd.find_comment_spans(text)
        block = block_of(spans, first_file.start())
        if not block:
            continue
        start, end = block
        cut = landing(text, spans)
        if cut > start:
            continue  # already above everything that opens the file

        body = text[start:end].rstrip() + "\n"
        rest = text[end:]
        while rest[:1] in (" ", "\t"):
            rest = rest[1:]
        if rest[:1] == "\n":
            rest = rest[1:]
        while rest[:1] == "\n":
            rest = rest[1:]
        removed = text[:start].rstrip("\n") + "\n\n" + rest \
            if text[:start].strip() else rest
        # Offsets shifted, so place by searching the trimmed text.
        head = removed[:cut] if cut <= len(removed) else removed
        new = head.rstrip("\n") + ("\n\n" if head.strip() else "") \
            + body + "\n" + removed[len(head):].lstrip("\n")

        if dox2nd.strip_comments(new) != dox2nd.strip_comments(text):
            print("REFUSED (code would change): %s" % path, file=sys.stderr)
            refused += 1
            continue
        moved += 1
        if args.apply:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)
        if args.report:
            print("%-54s %s" % (path[-52:], first_file.group(3)[:34]))

    print("\nFile topics moved to the top : %d" % moved)
    if refused:
        print("files refused                : %d" % refused)
    return 0


if __name__ == "__main__":
    sys.exit(main())
