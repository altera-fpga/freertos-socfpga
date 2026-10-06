#!/usr/bin/env python3
"""Give every group heading a description.

Natural Docs prints a group as a row of its own in the summary, with a cell
for the description beside it. A heading with no text under it leaves that
cell blank, which reads as though something is missing - and often it is: a
few headings have no topics under them at all.

The sentence is built from what the group actually contains rather than from
its title, because the two disagree often enough to matter: several headings
called "Macros" hold nothing but a callback typedef.

    python3 tools/nd_groups.py --dry-run drivers osal ...
    python3 tools/nd_groups.py --apply   drivers osal ...
"""

import argparse
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dox2nd
import nd_describe
import nd_macros
import nd_titles

# How each kind of topic reads when a group is described by its contents.
KINDS = [
    ("Function", "entry points"), ("Functions", "entry points"),
    ("Callback", "callback signatures"), ("Struct", "structures"),
    ("Type", "types"), ("Types", "types"), ("Enum", "enumerated values"),
    ("Constant", "constants"), ("Constants", "constants"),
    ("Macro", "macros"), ("Macros", "macros"), ("Variable", "variables"),
    ("Variables", "variables"),
]
ORDER = [name for name, _ in KINDS]
WORD = dict(KINDS)

# Titles that say nothing the contents do not, so the sentence leaves them out.
GENERIC = {"functions", "function", "macros", "macro", "constants", "constant",
           "structures", "structure", "structs", "types", "type",
           "enumerations", "enumeration", "enums", "enum", "variables",
           "callbacks", "definitions", "data types"}


def contents(kinds):
    """"Entry points and callback signatures" for the kinds in a group."""
    seen = collections.Counter(kinds)
    words = [WORD[k] for k in sorted(seen, key=lambda k: (
        ORDER.index(k) if k in ORDER else len(ORDER))) if k in WORD]
    words = list(dict.fromkeys(words))[:3]
    if not words:
        return None
    phrase = nd_describe.join(words)
    return phrase[0].upper() + phrase[1:]


def describe(title, kinds, path):
    phrase = contents(kinds)
    if not phrase:
        return None
    component = nd_describe.prose_name(nd_titles.title_for(path))
    plain = title.strip().lower()
    # An identifier-like heading reads no better than a generic one.
    if plain in GENERIC or "_" in plain:
        return "%s for the %s." % (phrase, component)
    return "%s for the %s's %s." % (phrase, component, plain)


def label_only(text, match, end, path):
    """Whether the body is a restatement of the heading and nothing more.

    A sample's "How to Run" also has no topics under it, but it carries the
    instructions themselves, so it stays. What goes is a header's "Structures"
    heading whose body reads "Timer Specific Structures" and which no topic
    ever followed.
    """
    if not path.endswith((".h", ".hpp")):
        return False
    body = [l for l in re.sub(r"[\*/]", " ", text[match.end():end]).split("\n")
            if l.strip()]
    return len(body) == 1 and len(body[0].strip()) <= 60


def group_spans(text):
    """[(match, comment_start, comment_end, member_kinds)] for each group."""
    spans = dox2nd.find_comment_spans(text)
    topics = list(nd_macros.TOPIC.finditer(text))
    out = []
    for i, m in enumerate(topics):
        if m.group(2) != "Group":
            continue
        span = next(((s, e) for _, s, e in spans if s <= m.start() < e), None)
        if not span:
            continue
        kinds = []
        for nxt in topics[i + 1:]:
            if nxt.group(2) in ("Group", "Section", "File"):
                break
            kinds.append(nxt.group(2))
        out.append((m, span[0], span[1], kinds))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    stats = collections.Counter()
    for path in dox2nd.gather(args.paths):
        if not path.endswith((".c", ".h")):
            continue
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        edits = []
        for m, start, end, kinds in group_spans(text):
            stats["groups"] += 1
            described = bool(re.sub(r"[\*/\s]", "", text[m.end():end]))
            if not kinds and (not described or label_only(text, m, end, path)):
                # A heading with nothing under it is a row leading nowhere.
                # Its topics, if it ever had any, are under the heading below.
                stats["removed"] += 1
                edits.append((start, end, None))
                continue
            if described:
                stats["described"] += 1
                continue
            sentence = describe(m.group(3), kinds, path)
            if not sentence:
                continue
            stats["filled"] += 1
            edits.append((m.end(), m.end(),
                          "\n\n%s%s" % (m.group(1), sentence)))

        if not edits:
            continue
        new = text
        for start, end, insert in sorted(edits, key=lambda e: -e[0]):
            if insert is None:
                line_start = new.rfind("\n", 0, start) + 1
                stop = end
                while stop < len(new) and new[stop] in " \t":
                    stop += 1
                if new[stop:stop + 1] == "\n":
                    stop += 1
                while new[stop:stop + 1] == "\n":
                    stop += 1
                new = new[:line_start] + new[stop:]
            else:
                new = new[:start] + insert + new[end:]

        if dox2nd.strip_comments(new) != dox2nd.strip_comments(text):
            print("REFUSED (code would change): %s" % path, file=sys.stderr)
            stats["refused"] += 1
            continue
        stats["files"] += 1
        if args.apply:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)
        if args.dry_run:
            print("--- %s" % path)
            for m, _, _, kinds in group_spans(text):
                if not re.sub(r"[\*/\s]", "", text[m.end():
                                                   text.find("*/", m.end())]):
                    print("    Group: %-26s %s" % (
                        m.group(3),
                        describe(m.group(3), kinds, path) or "(removed)"))

    print("\ngroup headings          : %d" % stats["groups"])
    print("  already described     : %d" % stats["described"])
    print("  description added     : %d" % stats["filled"])
    print("  removed, nothing under: %d" % stats["removed"])
    print("files touched           : %d" % stats["files"])
    if stats["refused"]:
        print("files refused           : %d" % stats["refused"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
