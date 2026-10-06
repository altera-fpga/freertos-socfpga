#!/usr/bin/env python3
"""Document functions that only exist in a .c file.

The header is the right home for anything a caller can reach, but a function
defined in a .c file and never declared in a header - every `static` helper,
plus a few file-local entry points such as `main` - has no header to live in.
Without a topic those functions are missing from the generated site entirely.

This finds every function defined in a .c file that no header declares, and
gives it a `Function:` topic. Where the function already has a plain comment
the wording is reused; where it has none, a topic is emitted with no prose
rather than inventing a description.

    python3 tools/nd_cfuncs.py --report  drivers samples ...
    python3 tools/nd_cfuncs.py --apply   drivers samples ...
"""

import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dox2nd

ND_TOPIC_LINE = re.compile(
    r"(?m)^[ \t]*(File|Function|Type|Struct|Enum|Constant|Macro|Callback|"
    r"Variable|Group|Section|Topic):[ \t]+(\S.*?)[ \t]*$"
)
HEADER_DECL = re.compile(
    r"(?m)^[^#\n]*?\b(\w+)\s*\([^;{()]*(?:\([^()]*\)[^;{()]*)*\)\s*"
    r"(?:__attribute__\s*\(\([^)]*\)\)\s*)?;"
)
KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "do", "else",
            "case", "defined", "typedef", "struct", "union", "enum"}
# FreeRTOS declares task bodies through a macro; the name is its first argument.
TASK_FUNCTION = re.compile(r"\bportTASK_FUNCTION(?:_PROTO)?\s*\(\s*(\w+)\s*,")


def masked(text, spans):
    """Text with comments and string literals blanked, offsets preserved."""
    buf = list(text)
    for _, s, e in spans:
        for i in range(s, e):
            if buf[i] != "\n":
                buf[i] = " "
    out, i, n = [], 0, len(buf)
    in_str = None
    while i < n:
        ch = buf[i]
        if in_str:
            if ch == "\\":
                out.append(" ")
                if i + 1 < n:
                    out.append(" ")
                    i += 1
                i += 1
                continue
            if ch == in_str:
                in_str = None
                out.append(ch)
            else:
                out.append("\n" if ch == "\n" else " ")
        elif ch in "\"'":
            in_str = ch
            out.append(ch)
        else:
            out.append(ch)
        i += 1
    return "".join(out)


def definitions(text):
    """(name, decl_start) for every function defined at file scope."""
    spans = dox2nd.find_comment_spans(text)
    code = masked(text, spans)
    # An `extern "C" {` wrapper would otherwise put a whole header one level
    # deep, hiding every function in it. Its closing brace is harmless: the
    # depth count below never goes below zero.
    for m in re.finditer(r'extern\s+"[^"\n]*"\s*\{', text):
        code = code[:m.end() - 1] + " " + code[m.end():]
    comment_end = {s: e for _, s, e in spans}
    found, depth, i, n = [], 0, 0, len(code)
    stmt = 0  # where the current top-level statement began
    while i < n:
        ch = code[i]
        if ch == "{":
            if depth == 0:
                # Classify the masked slice: a comment sitting above the
                # function would otherwise be mistaken for the declaration.
                # classify() needs the opening brace to tell a definition
                # from an expression.
                decl = code[stmt:i + 1]
                # Flattened first: classify() does not expect the return type
                # to sit on a line of its own, which is common in this tree.
                flat = re.sub(r"\s+", " ", decl).strip()
                kw, ident = dox2nd.classify(flat)
                macro = TASK_FUNCTION.search(decl)
                if macro:
                    # The name is a macro argument, so it is not followed by
                    # "(" and the token check below does not apply.
                    found.append((macro.group(1),
                                  stmt + (len(decl) - len(decl.lstrip()))))
                elif kw == "Function" and ident not in KEYWORDS:
                    # Reject a name the regex carved out of a longer token.
                    if re.search(r"(?:^|[^\w])%s\s*\(" % re.escape(ident), flat):
                        found.append((ident,
                                      stmt + (len(decl) - len(decl.lstrip()))))
                depth = 1
            else:
                depth += 1
        elif ch == "}":
            depth = max(0, depth - 1)
            if depth == 0:
                stmt = i + 1
        elif ch == ";" and depth == 0:
            stmt = i + 1
        elif ch == "#" and depth == 0 and (i == 0 or code[i - 1] == "\n"):
            nl = code.find("\n", i)
            while nl != -1 and code[nl - 1:nl] == "\\":
                nl = code.find("\n", nl + 1)
            i = nl if nl != -1 else n
            stmt = i + 1
        i += 1
    return found, comment_end


def header_declarations(paths):
    """Every function name any header declares."""
    names = set()
    for path in dox2nd.gather(paths):
        if not path.endswith((".h", ".hpp")):
            continue
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        code = masked(text, dox2nd.find_comment_spans(text))
        for m in HEADER_DECL.finditer(code):
            names.add(m.group(1))
    return names - KEYWORDS


def is_prose(lines):
    """Whether a comment describes anything, or is decoration."""
    text = " ".join(lines).strip()
    if len(re.findall(r"[A-Za-z]{2,}", text)) < 2:
        return False
    # Separator banners: /*------------*/ and friends.
    if not re.search(r"[A-Za-z]{3}", re.sub(r"[-=_*~+#|<>/\\. \t]", "", text)):
        return False
    # Markers left on #endif lines, such as /* if ( x == 1 ) */.
    if re.match(r"(?i)^\(?\s*(#\s*)?(end\s*if|endif|else|elif|if|ifn?def)\b", text):
        return False
    if re.fullmatch(r"[\s()!]*[\w.]+\s*(?:[=!<>]=|[<>])\s*[\w.]+[\s()]*", text):
        return False
    return True


def preceding_comment(text, start, spans):
    """The comment block immediately above `start`, if it describes it."""
    best = None
    for kind, s, e in spans:
        if e > start:
            break
        between = text[e:start]
        if between.strip():
            continue
        if between.count("\n") > 2:
            continue
        best = (kind, s, e)
    if not best:
        return None
    kind, s, e = best
    if not is_prose([l.strip() for l in dox2nd.comment_body(text[s:e], kind)]):
        return None
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--orphans", action="store_true",
                    help="also document header-declared functions that no "
                         "header got around to describing")
    ap.add_argument("--exclude", action="append", default=[], metavar="TEXT",
                    help="leave files whose path contains TEXT alone")
    args = ap.parse_args()

    declared = header_declarations(args.paths)
    stats = dict(files=0, funcs=0, in_header=0, already=0, from_comment=0,
                 bare=0, refused=0)

    # Names that already have a topic anywhere, so a function defined inline in
    # a header is not documented twice.
    documented = set()
    for path in dox2nd.gather(args.paths):
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        documented |= {m.group(2) for m in ND_TOPIC_LINE.finditer(text)
                       if m.group(1) == "Function"}

    for path in dox2nd.gather(args.paths):
        if not path.endswith((".c", ".h")):
            continue
        if any(text in path for text in args.exclude):
            continue
        header = path.endswith(".h")
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        spans = dox2nd.find_comment_spans(text)
        defs, _ = definitions(text)
        if not defs:
            continue
        have = {m.group(2) for m in ND_TOPIC_LINE.finditer(text)
                if m.group(1) == "Function"}

        edits = []
        for name, start in defs:
            stats["funcs"] += 1
            # A function a header declares belongs to that header. But plenty
            # of prototypes were never written up, and the topic has to live
            # somewhere: the definition is also the only place a description
            # can be derived from.
            if name in declared and not header \
                    and (name in documented or not args.orphans):
                stats["in_header"] += 1
                continue
            if header and name in documented - have:
                stats["already"] += 1
                continue
            if name in have:
                stats["already"] += 1
                continue
            prev = preceding_comment(text, start, spans)
            edits.append((name, start, prev))

        if not edits:
            continue
        stats["files"] += 1

        # Apply from the end so earlier offsets stay valid.
        new = text
        for name, start, prev in sorted(edits, key=lambda e: -e[1]):
            line_start = new.rfind("\n", 0, start) + 1
            indent = new[line_start:start]
            indent = indent if not indent.strip() else ""
            if prev:
                kind, s, e = prev
                body = [l.rstrip() for l in dox2nd.comment_body(new[s:e], kind)]
                while body and not body[0].strip():
                    body.pop(0)
                while body and not body[-1].strip():
                    body.pop()
                stats["from_comment"] += 1
                block = render_topic(name, body, indent)
                new = new[:s] + block + new[e:].lstrip(" \t")
            else:
                stats["bare"] += 1
                block = render_topic(name, [], indent)
                new = new[:line_start] + block + new[line_start:]

        if dox2nd.strip_comments(new) != dox2nd.strip_comments(text):
            print("REFUSED (code would change): %s" % path, file=sys.stderr)
            stats["refused"] += 1
            continue
        if args.apply:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)
        if args.report:
            print("%-52s %d" % (path[-50:], len(edits)))

    print("\nfunctions defined in .c files : %d" % stats["funcs"])
    print("  declared in a header        : %d  (documented there)" % stats["in_header"])
    print("  already had a topic         : %d" % stats["already"])
    print("  topic from existing comment : %d" % stats["from_comment"])
    print("  topic with no prose         : %d" % stats["bare"])
    print("files touched                 : %d" % stats["files"])
    if stats["refused"]:
        print("files refused                 : %d" % stats["refused"])
    return 0


def render_topic(name, body, indent):
    pad = indent + "    "
    out = ["%s/*\n" % indent, "%sFunction: %s\n" % (pad, name)]
    if body:
        out.append("\n")
        for line in body:
            out.append((pad + line.strip()).rstrip() + "\n" if line.strip() else "\n")
    out.append("%s*/\n" % indent)
    return "".join(out)


if __name__ == "__main__":
    sys.exit(main())
