#!/usr/bin/env python3
"""Give undocumented files a minimal Natural Docs `File:` topic.

A file with no topic at all never reaches the generated site and cannot be
placed in the menu. This adds the smallest thing that fixes that: a `File:`
topic and a one-line description, inserted after the licence header.

The description is taken from the file's own leading comment when it has one
that says something; otherwise it is composed from the component and the role
the filename implies. Files that already carry any topic are left untouched.

    python3 tools/nd_stubs.py --dry-run drivers samples ...
    python3 tools/nd_stubs.py --apply   drivers samples ...

Run tools/nd_titles.py afterwards to replace the placeholder names with
readable, project-unique titles.
"""

import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dox2nd
import nd_titles

ANY_TOPIC = re.compile(
    r"(?m)^[ \t]*(File|Function|Type|Struct|Enum|Constant|Macro|Callback|"
    r"Variable|Group|Section|Topic|Class):[ \t]+\S"
)

# A block containing licence terms is boilerplate however much prose surrounds
# it. A bare SPDX or copyright line is not: this tree routinely puts the file's
# one-line description inside that same header.
LICENCE_BLOCK = re.compile(
    r"(?i)permission is hereby granted|the software is provided|"
    r"redistribution and use|gnu general public|without warranty of any kind|"
    r"subject to the following conditions|this program is free software"
)
LICENCE_LINE = re.compile(
    r"(?i)^(spdx[-\w]*\s*:|copyright\b|\(c\)\s*\d|all rights reserved|"
    r"https?://|www\.|licen[cs]e\b)"
)

# Lines that carry no information even inside a real comment.
JUNK = re.compile(r"^\s*(?:[*=_#/\\+~<>|-]{3,}|\*+/?|)\s*$")

# Section banners that sit above the includes and describe nothing.
NOT_PROSE = re.compile(
    r"(?i)^(standard|scheduler|kernel|demo|library|freertos|system|local|"
    r"project|common|application|priv(ate)?|public|c)?\s*"
    r"(program\s+|demo\s+)?includes?\b|^includes?\s*[:.]?$"
)

ROLE_TEMPLATE = {
    "Registers": "Register definitions for the %s.",
    "Low-Level Driver": "Low-level register access for the %s.",
    "Definitions": "Type and constant definitions for the %s.",
    "Configuration": "Build-time configuration for the %s.",
    "Utilities": "Helper routines for the %s.",
    "Helpers": "Helper routines for the %s.",
    "Sample": "Sample application for the %s.",
    "CLI Commands": "Command-line interface commands for the %s.",
    "Driver": "Driver for the %s.",
    "Library": "Library routines for the %s.",
    "Sample Entry Point": "Entry point for the %s sample.",
    "Entry Point": "Entry point for the %s.",
}


def first_code_offset(text, spans):
    """Where real code starts, ignoring comments and blank space."""
    pos, i = 0, 0
    while pos < len(text):
        while i < len(spans) and spans[i][2] <= pos:
            i += 1
        if i < len(spans) and spans[i][1] <= pos < spans[i][2]:
            pos = spans[i][2]
            continue
        if text[pos].isspace():
            pos += 1
            continue
        return pos
    return len(text)


def first_sentence(blob):
    """First sentence, not fooled by a period inside "flash.c" split over lines."""
    pos = 0
    while True:
        m = re.compile(r".{8,300}?[.!?](?:\s|$)", re.S).match(blob, pos)
        if not m:
            return blob[:160].rstrip(" ,;:")
        end = m.end()
        tail = blob[end:].lstrip()
        # A lowercase continuation means the period was part of a filename.
        if tail and tail[0].islower() and end < 200:
            pos = end
            continue
        return blob[:end].strip()


def leading_description(text):
    """First sentence of the file's own header comment, if it has a real one."""
    spans = dox2nd.find_comment_spans(text)
    code_at = first_code_offset(text, spans)
    for kind, start, end in spans:
        # Only comments that sit above all code can describe the file.
        if end > code_at:
            break
        raw = text[start:end]
        if LICENCE_BLOCK.search(raw):
            continue
        lines = dox2nd.comment_body(raw, kind)
        lines = [l.strip() for l in lines if l.strip() and not JUNK.match(l)]
        lines = [re.sub(r"^[@\\](?:file|brief)\s*", "", l) for l in lines]
        lines = [l for l in lines
                 if l and not l.startswith(("@", "\\")) and not LICENCE_LINE.match(l)]
        if not lines or NOT_PROSE.match(lines[0]):
            continue
        blob = re.sub(r"\s+", " ", " ".join(lines[:4])).strip()
        # "PollQ. c" is a line-wrap artefact of the original comment.
        blob = re.sub(r"\b(\w+)\s*\.\s+([ch])\b", r"\1.\2", blob)
        sentence = first_sentence(blob)
        # Reject include-guard names, pointers elsewhere, and other non-prose.
        if len(sentence) < 12 or re.match(r"^[A-Z0-9_]+\.?$", sentence):
            continue
        if re.match(r"(?i)^(see |refer to |for more information|please )", sentence):
            continue
        if not sentence.endswith((".", "!", "?")):
            sentence += "."
        return sentence[0].upper() + sentence[1:]
    return None


def derived_description(path):
    """A description composed from what the path already tells us."""
    title = nd_titles.title_for(path)
    for role, template in ROLE_TEMPLATE.items():
        if title.endswith(role):
            subject = title[: -len(role)].strip()
            if subject:
                return template % subject
            break
    if path.endswith((".c", ".S", ".s")):
        return "Implementation of %s." % title
    return "Definitions for %s." % title


def insert_at(text):
    """Byte offset just after the licence header, before any code."""
    spans = dox2nd.find_comment_spans(text)
    cut = 0
    for kind, start, end in spans:
        if text.count("\n", 0, start) > 40:
            break
        if text[:start].strip():
            break
        cut = end
    while cut < len(text) and text[cut] in " \t":
        cut += 1
    if cut < len(text) and text[cut] == "\n":
        cut += 1
    return cut


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    added = mined = composed = 0
    for path in dox2nd.gather(args.paths):
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        if ANY_TOPIC.search(text):
            continue

        desc = leading_description(text)
        if desc:
            mined += 1
        else:
            desc = derived_description(path)
            composed += 1

        name = os.path.basename(path)
        block = "/*\n    File: %s\n\n    %s\n*/\n\n" % (name, desc)
        cut = insert_at(text)
        new = text[:cut].rstrip("\n")
        new = (new + "\n\n" if new else "") + block + text[cut:].lstrip("\n")

        if dox2nd.strip_comments(new) != dox2nd.strip_comments(text):
            print("REFUSED (code would change): %s" % path, file=sys.stderr)
            continue

        added += 1
        if args.apply:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)
        if args.dry_run:
            print("%-58s %s" % (path[-56:], desc[:80]))

    print("\nfile topics added   : %d" % added)
    print("  from own comment  : %d" % mined)
    print("  composed from path: %d" % composed)
    return 0


if __name__ == "__main__":
    sys.exit(main())
