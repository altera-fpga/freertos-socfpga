#!/usr/bin/env python3
"""Document the struct, union and enum types that cross a file boundary.

Natural Docs prints no prototype for a `Struct:` topic, so a type with no topic
reaches the reader as a bare name in a function signature with nothing behind
it. Every such type gets a topic here, described from two places the source
already tells us about: the fields it declares, and the place the code declares
one - the `struct foo bar` that names it, or the function that takes it.

Types private to a single .c file are left alone, and so is anything that
already has a topic. Where a plain comment already sits above the definition its
wording is reused instead of writing a new sentence.

    python3 tools/nd_structs.py --dry-run drivers samples ...
    python3 tools/nd_structs.py --apply   drivers samples ...
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
import nd_macros
import nd_titles

AGG = re.compile(r"\b(struct|union|enum)\b\s*(\w*)\s*\{")
MEMBER = re.compile(r"(\w+)\s*(?:\[[^\]]*\])?\s*(?::\s*\d+\s*)?[;,]")
FILLER = re.compile(
    r"(?i)^(pad+(ing)?|reserved|rsvd|unused|dummy)[\d_]*$")

# The noun the code itself uses for the type, mapped to how it reads in a
# sentence. Matched against whole name tokens: a substring search would find
# "err" inside "interrupt".
PAIRS = {
    ("cmd", "response"): "Command response",
    ("cmd", "payload"): "Command payload",
    ("req", "payload"): "Request payload",
    ("error", "info"): "Error information",
    ("err", "info"): "Error information",
}
NOUNS = {
    "descriptor": "Descriptor", "descriptors": "Descriptors",
    "desc": "Descriptor", "context": "Context", "cntxt": "Context",
    "ctx": "Context", "payload": "Payload", "response": "Response",
    "responses": "Responses", "resp": "Response", "request": "Request",
    "req": "Request", "command": "Command", "cmd": "Command",
    "config": "Configuration", "configuration": "Configuration",
    "cfg": "Configuration", "parameters": "Parameters", "params": "Parameters",
    "param": "Parameter", "header": "Header", "hdr": "Header",
    "status": "Status", "handle": "Handle", "buffer": "Buffer",
    "buf": "Buffer", "event": "Event", "entry": "Entry", "queue": "Queue",
    "message": "Message", "msg": "Message", "error": "Error information",
    "err": "Error information", "info": "Information", "stats": "Statistics",
    "table": "Table", "instance": "Instance state", "state": "State",
    "data": "Data", "list": "List", "ring": "Ring", "handler": "Handler",
    "trb": "Transfer request block",
}
KIND_WORD = {"struct": "Struct", "union": "Struct", "enum": "Enum"}
# Names that are decoration on the definition, not the type's own name.
NOT_A_NAME = re.compile(r"^(__attribute__|packed|aligned)$")

# A one-line topic this tool wrote earlier: a single paragraph opening with one
# of the sentence shapes below. Anchoring at the start matters - a description
# reused from the source may well mention these words later on, and refreshing
# would then throw away wording that exists nowhere else.
GENERATED = re.compile(
    r"(?m)^([ \t]*)/\*\n[ \t]*(?:Struct|Enum):[ \t]+\w+\n\n"
    r"[ \t]*(\S[^\n]*)\n[ \t]*\*/\n")
GENERATED_BODY = re.compile(
    r"(?:[A-Z][\w /-]*? )?(?:values )?used by the |"
    r"Memory-mapped register layout of the |"
    r"[A-Z][\w /-]*? for the [\w /-]+?(?:, holding |\.)")


def depth_of_braces(code):
    """Brace nesting depth at each opening brace."""
    depth, out = 0, {}
    for i, ch in enumerate(code):
        if ch == "{":
            out[i] = depth
            depth += 1
        elif ch == "}":
            depth = max(0, depth - 1)
    return out


def aggregates(code):
    """(kind, name, start, body) for each file-scope struct, union or enum."""
    depths = depth_of_braces(code)
    out = []
    for m in AGG.finditer(code):
        brace = m.end() - 1
        if depths.get(brace) != 0:
            continue
        end, depth = brace, 0
        while end < len(code):
            if code[end] == "{":
                depth += 1
            elif code[end] == "}":
                depth -= 1
                if depth == 0:
                    break
            end += 1
        semi = code.find(";", end)
        tail = code[end + 1:semi if semi != -1 else end + 1]
        # "} __attribute__((packed)) name;" names the type after the attribute.
        tail = re.sub(r"__attribute__\s*\(\(.*?\)\)", " ", tail, flags=re.S).strip()
        alias = re.match(r"[\*\s]*(\w+)", tail)
        # A typedef is known by its alias; a tagged definition by its tag.
        name = m.group(2)
        if alias and (not name or re.search(
                r"\btypedef\b", code[max(0, m.start() - 40):m.start()])):
            name = alias.group(1)
        if name and not NOT_A_NAME.match(name):
            out.append((m.group(1), name, m.start(), code[brace + 1:end]))
    return out


def members(body, outer_only=True):
    flat, depth = [], 0
    for ch in body:
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth = max(0, depth - 1)
        elif depth == 0 or not outer_only:
            flat.append(ch)
    # Innermost parentheses first, so nested ones such as
    # __attribute__((aligned (0x10000))) come away whole.
    text = "".join(flat)
    while True:
        stripped = re.sub(r"\([^()]*\)", " ", text)
        if stripped == text:
            break
        text = stripped
    text = text.replace("__attribute__", " ")
    names = []
    for m in MEMBER.finditer(text):
        name = m.group(1)
        if name in names or FILLER.match(name) or re.fullmatch(r"\d+", name):
            continue
        names.append(name)
    return names


def fields_of(body):
    """Member names, with padding left out.

    A type built entirely of nested structs or unions has no members of its
    own, so the nested names are used rather than saying nothing.
    """
    return members(body) or members(body, outer_only=False)


def is_register_map(body, fields):
    """Whether the type mirrors a block of hardware registers.

    Register names are spelled in capitals here, which separates a real
    register window from an ordinary struct that happens to hold a volatile
    flag shared with an interrupt handler.
    """
    if len(fields) < 2:
        return False
    upper = [f for f in fields if f.upper() == f and len(f) > 3]
    if len(upper) == len(fields):
        return True
    if len(fields) < 3:
        return False
    return len(upper) * 2 > len(fields) or body.count("volatile") >= 3


def tokens_of(name):
    parts = re.split(r"[^A-Za-z0-9]+", re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name))
    return [p.lower() for p in parts if p]


def noun_for(name):
    words = tokens_of(name)
    for a, b in zip(words, words[1:]):
        if (a, b) in PAIRS:
            return PAIRS[(a, b)]
    # Last token first: in "cmd_parameters" the head noun is "parameters".
    for word in reversed(words):
        if word in NOUNS:
            return NOUNS[word]
    return None


def listed(names, limit=3):
    """"a, b and c", or "a, b and 7 more" when there are too many."""
    if not names:
        return None
    if len(names) <= limit + 1:
        return nd_describe.join(names)
    return "%s and %d more" % (", ".join(names[:limit]), len(names) - limit)


def summary(kind, name, path, body):
    """One sentence: what the type is, and what it holds."""
    fields = fields_of(body)
    component = nd_describe.prose_name(nd_titles.title_for(path))
    noun = noun_for(name)

    if kind == "enum":
        head = "%s values used by the %s" % (noun, component) if noun \
            else "Values used by the %s" % component
        return "%s: %s." % (head, listed(fields, 3)) if fields else head + "."

    if is_register_map(body, fields):
        subject = re.sub(r"(?i)\s+registers?$", "",
                         nd_describe.component_of(nd_titles.title_for(path)))
        head = "Memory-mapped register layout of the %s" % subject
        return "%s: %s." % (head, listed(fields, 3)) if fields else head + "."

    if noun:
        head = "%s for the %s" % (noun, component)
    else:
        head = "Type used by the %s" % component
    if not fields:
        return head + "."
    return "%s, holding %s." % (head, listed(fields, 3))


def declarations(name, code):
    """Variables of this type: [(varname, is_array)]."""
    out = []
    pattern = r"\b(?:struct|union|enum)\s+%s\b|\b%s\b" % (
        re.escape(name), re.escape(name))
    for m in re.finditer(r"(?:%s)\s*\**\s*(\w+)\s*(\[[^\]]*\])?\s*(?=[;,=\)])"
                         % pattern, code):
        if m.group(1) in ("struct", "union", "enum"):
            continue
        out.append((m.group(1), bool(m.group(2))))
    return out


def alias_of(name, code_of):
    """The typedef name callers use for this type, if there is one."""
    pattern = re.compile(
        r"\btypedef\s+(?:struct|union|enum)\s+%s\s*\**\s*(\w+)\s*;"
        % re.escape(name))
    for path in sorted(code_of):
        found = pattern.search(code_of[path])
        if found:
            return found.group(1)
    return None


def usage_note(name, defining, code_of, documented=()):
    """Where the source actually uses the type, as a closing sentence.

    Only headers are searched for callers: in a .c file the same shape as a
    prototype is far more often a call, which would credit the type to
    whatever the surrounding code happened to invoke.
    """
    alias = alias_of(name, code_of)
    if alias:
        # Link it where the alias has a topic of its own to link to.
        shown = "<%s>" % alias if alias in documented else alias
        return "Callers reach it through %s." % shown

    takers = []
    for path, code in sorted(code_of.items()):
        if not path.endswith(".h"):
            continue
        for m in nd_cfuncs.HEADER_DECL.finditer(code):
            decl = m.group(0)
            if re.search(r"\b%s\b" % re.escape(name), decl) \
                    and m.group(1) != name:
                takers.append(m.group(1))
    takers = list(dict.fromkeys(takers))
    if takers:
        return "Passed to %s." % listed(takers[:2], 2)

    for path in sorted(code_of, key=lambda p: p != defining):
        for var, array in declarations(name, code_of[path]):
            if var == name:
                continue
            return "Declared as %s%s in %s." % (
                var, "[]" if array else "", os.path.basename(path))
    return None


def kerneldoc(lines, name, fallback=None):
    """A kernel-doc block rewritten as Natural Docs.

    The vendored FCS library documents its types in kernel-doc style: a
    "struct x - what it is" line followed by "@field: meaning". The wording is
    good, only the markup is wrong, so the fields become a definition list and
    the redundant name line goes away.
    """
    if not any(re.match(r"\s*@\w+", l) for l in lines):
        return None

    head, fields, current = [], [], None
    for line in lines:
        stripped = line.strip()
        at = re.match(r"@(\w+)\s*:\s*(.*)$", stripped)
        if at:
            current = [at.group(1), at.group(2).strip()]
            fields.append(current)
            continue
        bare = re.match(r"@(\w+)\s*$", stripped)
        if bare:
            current = [bare.group(1), ""]
            fields.append(current)
            continue
        if current is not None:
            if stripped:
                current[1] = (current[1] + " " + stripped).strip()
            continue
        head.append(stripped)

    # "struct fcs_hash_256 - structure of vab hash256" says the name twice.
    kept = []
    for line in head:
        named = re.match(r"(?:struct|union|enum)\s+\w+\s*(?:-\s*(.*))?$", line)
        if named:
            line = (named.group(1) or "").strip()
        if line:
            kept.append(line)
    head = kept
    summary = " ".join(head).strip() or (fallback or "")
    if summary and not summary.endswith((".", ":", "!", "?")):
        summary += "."
    if summary:
        summary = summary[0].upper() + summary[1:]

    out = [summary] if summary else []
    described = [(f, d) for f, d in fields if d]
    if described:
        if out:
            out.append("")
        width = max(len(f) for f, _ in described)
        for field, desc in described:
            if not desc.endswith((".", ":", "!", "?")):
                desc += "."
            out.append("%-*s - %s" % (width, field, desc[0].upper() + desc[1:]))
    return out or None


def reusable(text, line_start, spans):
    """The comment above the definition, when it really describes the type.

    Guards against the two things that sit above a definition and are not its
    description: a trailing marker left on the line before it, and a block
    note about the file or the design that happens to come just above.
    """
    prev = nd_cfuncs.preceding_comment(text, line_start, spans)
    if not prev:
        return None
    kind, s, e = prev
    if nd_cfuncs.ND_TOPIC_LINE.search(text[s:e]):
        return None  # a Group heading belongs to the file, not to this type
    own_line = text.rfind("\n", 0, s) + 1
    if text[own_line:s].strip():
        return None  # trails the line above, so it describes that line
    lines = [l.strip() for l in dox2nd.comment_body(text[s:e], kind)
             if l.strip()]
    if not lines or lines[0].startswith(("!<", "<")):
        return None
    if re.match(r"(?i)(note|todo|fixme|xxx|hack|warning)\b[:\s]", lines[0]):
        return None  # an aside to the next maintainer, not a description
    if any(re.match(r"[-*\u2022]\s", l) for l in lines) and not any(
            re.match(r"@\w+", l) for l in lines):
        return None  # a bulleted note, not a one-idea description
    if len(lines) > 4 and not any(re.match(r"@\w+", l) for l in lines):
        return None
    return prev


def opened(sentence, path):
    """A reused comment made to read as a sentence.

    Its first word is often the component spelled in lower case, so the file's
    own name for the component is preferred over capitalising blindly: "iossm
    command request data" becomes "IOSSM ...", not "Iossm ...".
    """
    if not sentence:
        return sentence
    subject = nd_describe.component_of(nd_titles.title_for(path)).split()[:1]
    first = re.match(r"(\w+)", sentence)
    if subject and first and first.group(1).lower() == subject[0].lower():
        return subject[0] + sentence[first.end(1):]
    return sentence[0].upper() + sentence[1:]


def unwritten(text):
    """The file with this tool's own one-line topics taken back out."""
    return GENERATED.sub(
        lambda m: "" if GENERATED_BODY.match(m.group(2)) else m.group(0), text)


def tidy(lines, name):
    """A reused comment with the noise taken off.

    Two habits show up in this tree: repeating "enum foo -" before the
    description, and leaving a bare identifier on a line of its own.
    """
    out = []
    for line in lines:
        named = re.match(r"(?:struct|union|enum)\s+%s\s*(?:-\s*(.*))?$"
                         % re.escape(name), line.strip())
        if named:
            line = (named.group(1) or "").strip()
        if line.strip() and nd_cfuncs.is_prose([line]):
            out.append(line)
    return out or lines


def render(name, kind, body_lines, indent):
    pad = indent + "    "
    out = ["%s/*\n" % indent, "%s%s: %s\n" % (pad, KIND_WORD[kind], name)]
    if body_lines:
        out.append("\n")
        for line in body_lines:
            out.append((pad + line.strip()).rstrip() + "\n" if line.strip()
                       else "\n")
    out.append("%s*/\n" % indent)
    return "".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--refresh", action="store_true",
                    help="rewrite the one-line topics an earlier run wrote")
    ap.add_argument("--include-local", action="store_true",
                    help="also document types no other file names")
    args = ap.parse_args()

    original_of, text_of, code_of, have_of = {}, {}, {}, {}
    for path in dox2nd.gather(args.paths):
        if not path.endswith((".c", ".h")):
            continue
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        original_of[path] = text
        text = unwritten(text) if args.refresh else text
        text_of[path] = text
        code_of[path] = nd_cfuncs.masked(text, dox2nd.find_comment_spans(text))
        have_of[path] = {m.group(3) for m in nd_macros.TOPIC.finditer(text)}

    documented = set().union(*have_of.values()) if have_of else set()
    stats = collections.Counter()

    for path, code in sorted(code_of.items()):
        text = text_of[path]
        spans = dox2nd.find_comment_spans(text)
        edits = []
        for kind, name, start, body in aggregates(code):
            stats["found"] += 1
            if name in have_of[path] or name in documented:
                stats["already"] += 1
                continue
            others = [p for p, c in code_of.items() if p != path and (
                re.search(r"\b(?:struct|union|enum)\s+%s\b" % re.escape(name), c)
                or declarations(name, c))]
            if not others:
                stats["file_local"] += 1
                if not args.include_local:
                    continue
            # A "typedef struct" opener puts the keyword mid-line, so look for
            # the comment from the start of the line the definition begins on.
            line_start = text.rfind("\n", 0, start) + 1
            prev = reusable(text, line_start, spans)
            sentence = summary(kind, name, path, body)
            note = usage_note(name, path, code_of, documented)
            if prev:
                kindc, s, e = prev
                lines = [l.rstrip() for l in
                         dox2nd.comment_body(text[s:e], kindc)]
                while lines and not lines[0].strip():
                    lines.pop(0)
                while lines and not lines[-1].strip():
                    lines.pop()
                converted = kerneldoc(lines, name, sentence)
                if converted:
                    lines = converted
                else:
                    lines = tidy(lines, name)
                    if note and len(lines) == 1:
                        # Say where the type is used, which the source cannot.
                        first = lines[0].rstrip()
                        if not first.endswith((".", "!", "?", ":")):
                            first += "."
                        lines = [opened(first, path) + " " + note]
                stats["from_comment"] += 1
            else:
                lines = [sentence + (" " + note if note else "")]
                stats["written"] += 1
            # Two copies of a vendored type would otherwise get a topic each,
            # and Natural Docs would tell them apart with a "(2)" suffix.
            documented.add(name)
            edits.append((kind, name, start, prev, lines))

        if not edits:
            if args.apply and text != original_of[path]:
                with open(path, "w", encoding="utf-8",
                          errors="surrogateescape") as fh:
                    fh.write(text)
            continue
        new = text
        for kind, name, start, prev, lines in sorted(edits, key=lambda e: -e[2]):
            line_start = new.rfind("\n", 0, start) + 1
            indent = new[line_start:start]
            indent = indent if not indent.strip() else ""
            block = render(name, kind, lines, indent)
            if prev:
                _, s, e = prev
                # render() ends the line, so the old comment's newline would
                # leave a blank one behind.
                rest = new[e:].lstrip(" \t")
                new = new[:s] + block + (rest[1:] if rest[:1] == "\n" else rest)
            else:
                new = new[:line_start] + block + new[line_start:]

        if dox2nd.strip_comments(new) != dox2nd.strip_comments(original_of[path]):
            print("REFUSED (code would change): %s" % path, file=sys.stderr)
            stats["refused"] += 1
            continue
        if new == original_of[path]:
            continue
        stats["files"] += 1
        if args.apply:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)
        if args.report:
            print("%-54s %d" % (path[-52:], len(edits)))
        if args.dry_run:
            print("--- %s" % path)
            for kind, name, _, prev, lines in edits:
                print("    %s: %-28s %s" % (KIND_WORD[kind], name,
                                            "  ".join(lines)[:118]))

    print("\nfile-scope types found    : %d" % stats["found"])
    print("  already had a topic     : %d" % stats["already"])
    print("  private to one file     : %d  (left alone)" % stats["file_local"])
    print("  topic from own comment  : %d" % stats["from_comment"])
    print("  topic described here    : %d" % stats["written"])
    print("files touched             : %d" % stats["files"])
    if stats["refused"]:
        print("files refused             : %d" % stats["refused"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
