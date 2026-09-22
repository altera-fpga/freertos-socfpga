#!/usr/bin/env python3
"""List the macros of files whose page would otherwise be a description only.

Some headers are nothing but #defines, so their page showed a sentence and
no contents at all. Natural Docs does not print the expansion of a #define,
so each macro is listed with a short description of what it expands to, using
its own comment where it has one.

The macros go into list topics (`Macros:` / `Constants:`) rather than one topic
each: Natural Docs anchors and indexes every entry of a list, so the names stay
linkable without adding hundreds of comment blocks to the source.

Register maps are left out. A list of several thousand offsets helps nobody,
and the file description already names the register families.

    python3 tools/nd_macros.py --dry-run drivers osal ...
    python3 tools/nd_macros.py --apply   drivers osal ...
"""

import argparse
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dox2nd
import nd_cfuncs
import nd_describe
import nd_titles

TOPIC = re.compile(
    r"(?m)^([ \t]*)(File|Functions?|Types?|Struct|Enum|Constants?|Macros?|"
    r"Callback|Variables?|Group|Section):[ \t]+(\S.*?)[ \t]*$")
DEFINE = re.compile(r"(?m)^[ \t]*#[ \t]*define[ \t]+(\w+)(\([^)]*\))?[ \t]*(.*)$")
# A token with underscores at both ends is underline markup to Natural Docs.
MARKUP_TOKEN = re.compile(r"(?<!\w)__\w+__(?!\w)")

LIST_LIMIT = 150     # above this a file gets the family summary instead
FAMILY_LIMIT = 12    # above this the list is split into families

# A list this tool wrote earlier: a heading, then nothing but "name - text".
GENERATED = re.compile(
    r"/\*\n[ \t]*(?:Macros|Constants):[ \t][^\n]*\n\n"
    r"(?:[ \t]*[\w.]+ +- [^\n]*\n)+\*/\n+")


def expansion_of(text, match):
    """The replacement text, following backslash continuations."""
    out = [match.group(3)]
    pos = text.find("\n", match.end(3) if match.group(3) else match.end())
    while out and out[-1].rstrip().endswith("\\"):
        out[-1] = out[-1].rstrip()[:-1]
        nxt = text.find("\n", pos + 1)
        out.append(text[pos + 1:nxt if nxt != -1 else len(text)])
        pos = nxt if nxt != -1 else len(text)
    joined = " ".join(" ".join(o.split()) for o in out)
    joined = re.sub(r"/\*.*?\*/", " ", joined)
    return re.sub(r"//.*$", "", joined).strip()


def comments_by_line(text, spans):
    """Comment blocks keyed by the line they describe.

    A comment at the end of a line describes that line; one on a line of its
    own describes the line below it.
    """
    out = {}
    for _, s, e in spans:
        own_line = text.rfind("\n", 0, s) + 1
        if text[own_line:s].strip():
            out[own_line] = (s, e)
            continue
        stop = len(text) - len(text[e:].lstrip(" \t\r\n"))
        if text[e:stop].count("\n") > 1:
            continue
        target = text.rfind("\n", 0, stop) + 1
        # A comment above a run of #defines heads the run; it does not
        # describe the first one.
        following = text[text.find("\n", target) + 1:]
        if re.match(r"[ \t]*#[ \t]*define\b", following):
            continue
        out.setdefault(target, (s, e))
    return out


def own_comment(text, by_line, match):
    """Prose from the comment directly above the #define."""
    line_start = text.rfind("\n", 0, match.start()) + 1
    found = by_line.get(line_start)
    for _ in range(1 if found else 0):
        s, e = found
        lines = [re.sub(r"^\s*(?:/\*+|\*+/?|//+)\s?", "", l).rstrip()
                 for l in text[s:e].split("\n")]
        lines = [l for l in lines if l.strip()]
        if len(lines) != 1 or not nd_cfuncs.is_prose(lines):
            continue
        sentence = lines[0].strip().rstrip("*/").strip()
        if len(re.findall(r"[a-z]{3,}", sentence)) < 2:
            continue
        # Commented-out code is not a description.
        if sentence.startswith("#") or re.search(r"[;{}]|#\s*define", sentence):
            continue
        if not sentence.endswith((".", "!", "?")):
            sentence += "."
        return sentence[0].upper() + sentence[1:]
    return None


def description(expansion, comment):
    if comment:
        return comment
    e = expansion.replace("__VA_ARGS__", "the arguments").strip()
    if not e:
        return "Build switch, defined with no replacement text."
    if "/*" in e or "*/" in e:
        # Quoting this would close the comment block it is written into.
        return "Expands to a definition given in full in the source."
    attribute = re.search(r"__attribute__\s*\(\(\s*(\w+)", e)
    if attribute:
        return "Applies the %s attribute." % attribute.group(1)
    if re.fullmatch(r"[-+~(]*\s*(?:0[xX][0-9a-fA-F]+|\d+\.?\d*)[uUlLfF]*\s*\)*", e):
        return "Value: %s." % e
    if re.fullmatch(r'"[^"]*"', e):
        return "Value: %s." % e
    called = re.match(r"(\w+)\s*\(", e)
    if MARKUP_TOKEN.search(e):
        if called and not MARKUP_TOKEN.fullmatch(called.group(1)):
            return "Expands to a call to %s." % called.group(1)
        return "Expands to a toolchain-specific definition."
    if called and len(e) > 64:
        return "Expands to a call to %s." % called.group(1)
    if len(e) <= 64:
        return "Expands to %s." % e
    return "Expands to a multi-line definition."


def families(macros, tokens):
    groups = collections.OrderedDict()
    for entry in macros:
        parts = entry[0].lstrip("_").split("_")
        key = "_".join(parts[:tokens]) if len(parts) > tokens else entry[0].lstrip("_")
        groups.setdefault(key, []).append(entry)
    return groups


def grouped(macros):
    """Macros split into a handful of families, or one list if that fails."""
    for tokens in (2, 1):
        groups = families(macros, tokens)
        if 1 < len(groups) <= 8 and all(len(g) > 1 for g in groups.values()):
            return groups
    return None


def collect(path, text):
    """[(name, params, description)] for the macros worth listing."""
    spans = dox2nd.find_comment_spans(text)
    code = nd_cfuncs.masked(text, spans)
    by_line = comments_by_line(text, spans)
    guard = re.search(r"#\s*ifndef\s+(\w+)\s*\n\s*#\s*define\s+\1\b", code)
    guard = guard.group(1) if guard else None
    found = collections.OrderedDict()
    for m in DEFINE.finditer(code):
        if m.group(1) == guard:
            continue
        raw = DEFINE.match(text, m.start())
        found.setdefault(m.group(1), []).append(
            (bool(m.group(2)), expansion_of(text, raw) if raw else "",
             own_comment(text, by_line, m)))
    out = []
    for name, arms in found.items():
        comment = next((c for _, _, c in arms if c), None)
        said = merge([description(e, None) for _, e, _ in arms])
        out.append((name, arms[0][0],
                    comment or said or "Defined per compiler or configuration."))
    return out


def merge(arms):
    """One description for a macro defined once per #if arm."""
    empty = [d for d in arms if d.startswith("Build switch")]
    real = [d for d in dict.fromkeys(arms) if not d.startswith("Build switch")]
    if not real:
        return arms[0]
    if len(real) == 1:
        return real[0][:-1] + ", where the compiler supports it." if empty \
            else real[0]
    if len(real) <= 3 and all(d.startswith("Expands to ") for d in real):
        return "Expands to %s, depending on the build." % " or ".join(
            d[len("Expands to "):].rstrip(".") for d in real)
    return None


def render(indent, keyword, title, entries):
    out = ["/*", "%s%s: %s" % (indent, keyword, title), ""]
    width = max(len(n) for n, _, _ in entries)
    for name, _, desc in entries:
        out.append("%s%-*s - %s" % (indent, width, name, desc))
    out += ["*/", ""]
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--refresh", action="store_true",
                    help="rewrite lists written by an earlier run")
    args = ap.parse_args()

    listed = files = skipped = 0
    for path in dox2nd.gather(args.paths):
        original = open(path, encoding="utf-8", errors="surrogateescape").read()
        text = GENERATED.sub("", original) if args.refresh else original
        topics = list(TOPIC.finditer(text))
        if not topics or topics[0].group(2) != "File":
            continue
        if set(m.group(2) for m in topics) - {"File", "Group", "Section"}:
            continue  # the page already has contents, listed or otherwise
        macros = collect(path, text)
        if not macros:
            continue
        if len(macros) > LIST_LIMIT:
            skipped += 1
            continue

        subject = nd_describe.component_of(nd_titles.title_for(path))
        groups = grouped(macros) if len(macros) > FAMILY_LIMIT else None
        if groups is None:
            groups = collections.OrderedDict([(subject, macros)])

        indent = topics[0].group(1)
        block = []
        for title, entries in groups.items():
            keyword = "Macros" if sum(1 for _, fn, _ in entries if fn) * 2 \
                > len(entries) else "Constants"
            # The label must not repeat the File topic's own title.
            label = ("%s %s" % (title, keyword.lower()) if title == subject
                     else "%s definitions" % title)
            block += render(indent, keyword, label, entries)

        # Go in after the File topic's comment, before any code.
        spans = dox2nd.find_comment_spans(text)
        span = next((e for _, s, e in spans if s <= topics[0].start() < e), None)
        if span is None:
            continue
        cut = text.find("\n", span) + 1
        new = text[:cut] + "\n" + "\n".join(block) + "\n" + text[cut:].lstrip("\n")
        if dox2nd.strip_comments(new) != dox2nd.strip_comments(original):
            print("REFUSED (code would change): %s" % path, file=sys.stderr)
            continue
        if new == original:
            continue
        files += 1
        listed += len(macros)
        if args.apply:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)
        if args.dry_run:
            print("--- %s : %d macros in %d groups" % (path, len(macros), len(groups)))
            print("\n".join(block[:14]))

    print("\nmacros listed          : %d  in %d files" % (listed, files))
    print("files too large to list: %d" % skipped)
    return 0


if __name__ == "__main__":
    sys.exit(main())
