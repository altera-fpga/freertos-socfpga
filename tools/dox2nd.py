#!/usr/bin/env python3
"""One-shot migration: rewrite Doxygen comments as Natural Docs topics.

Run from the repository root:

    python3 tools/dox2nd.py --check   drivers samples ...   # report only
    python3 tools/dox2nd.py --apply   drivers samples ...   # rewrite in place

Only comment spans are ever rewritten. --apply refuses to write a file whose
non-comment text would change, so a parser mistake cannot corrupt code.
"""

import argparse
import os
import re
import sys

C_EXTENSIONS = (".c", ".h", ".S", ".s")

# Doxygen inline markup that carries no meaning once the tags are gone.
INLINE_EMPHASIS = re.compile(r"[@\\](?:c|p|b|e|a)\s+(\S+)")
REF_QUOTED = re.compile(r"[@\\]ref\s+(\w+)\s+\"([^\"]*)\"")
REF_BARE = re.compile(r"[@\\]ref\s+(\w+)")
# `@see` mid-sentence is a reference; at the start of a line it opens a
# See Also section and is handled by the command parser instead.
SEE_INLINE = re.compile(r"(?<=\S)([ \t]+)[@\\](?:see|sa)[ \t]+(\w+)")
AUTOLINK = re.compile(r"(?<![\w#])#(\w+)")

REF_OPEN, REF_CLOSE = "\x00REF:", "\x00"

# Topic keywords Natural Docs already understands, used to spot hand-written
# Natural Docs comments that must be preserved rather than converted.
ND_TOPIC = re.compile(
    r"^\s*(File|Function|Group|Section|Type|Struct|Enum|Constant|Macro|"
    r"Callback|Variable|Topic|Title|About|Class):\s*(\S+)",
    re.MULTILINE,
)


def find_comment_spans(text):
    """Return [(kind, start, end)] for every comment, skipping string bodies."""
    spans = []
    i, n = 0, len(text)
    while i < n:
        ch = text[i]
        if ch in "\"'":
            quote = ch
            i += 1
            while i < n:
                if text[i] == "\\":
                    i += 2
                    continue
                if text[i] == quote:
                    i += 1
                    break
                if text[i] == "\n" and quote == "'":
                    break
                i += 1
            continue
        if ch == "/" and i + 1 < n:
            if text[i + 1] == "*":
                end = text.find("*/", i + 2)
                end = n if end == -1 else end + 2
                spans.append(("block", i, end))
                i = end
                continue
            if text[i + 1] == "/":
                end = text.find("\n", i)
                end = n if end == -1 else end
                spans.append(("line", i, end))
                i = end
                continue
        i += 1
    return spans


def strip_comments(text):
    """Text with every comment removed, for the code-preservation check."""
    out, prev = [], 0
    for _, start, end in find_comment_spans(text):
        out.append(text[prev:start])
        out.append(" ")
        prev = end
    out.append(text[prev:])
    return re.sub(r"\s+", " ", "".join(out)).strip()


DOX_COMMAND = re.compile(
    r"^[ \t]*\*?[ \t]*[@\\](brief|param|return|returns|retval|note|warning|"
    r"details|file|defgroup|ingroup|addtogroup|section|subsection|struct|enum|"
    r"func|fn|typedef|var|def|class|see|sa|attention|pre|post)\b",
    re.MULTILINE,
)


def is_doxygen(text, kind):
    """True for documentation blocks, excluding the trailing `<` member form."""
    if kind == "block":
        if text.startswith(("/**<", "/*!<")):
            return False
        if text.startswith("/**") and text.rstrip().rstrip("/").rstrip("*") == "/":
            return False    # a /****...****/ banner rule, not documentation
        if text.startswith("/**") or text.startswith("/*!"):
            return True
        # Plain /* */ blocks that still carry Doxygen commands must be
        # converted too, otherwise Doxygen syntax survives the migration.
        return bool(DOX_COMMAND.search(re.sub(r"^/\*[*!]?", "", text)))
    if text.startswith(("///<", "//!<")):
        return False
    if text.startswith("///") or text.startswith("//!"):
        return True
    return bool(DOX_COMMAND.search(re.sub(r"^//[/!]?", "", text)))


def comment_body(text, kind):
    """Strip comment delimiters and per-line decoration."""
    if kind == "block":
        inner = re.sub(r"^/\*[*!]?", "", text)
        if inner.endswith("*/"):
            inner = inner[:-2]
    else:
        inner = "\n".join(
            re.sub(r"^//[/!]?", "", line.strip()) for line in text.splitlines()
        )
    lines = []
    for line in inner.splitlines():
        line = line.rstrip()
        stripped = line.lstrip()
        if stripped.startswith("*") and not stripped.startswith("*/"):
            stripped = stripped[1:]
            if stripped.startswith(" "):
                stripped = stripped[1:]
            lines.append(stripped.rstrip())
        else:
            lines.append(line.strip())
    while lines and not lines[0].strip():
        lines.pop(0)
    while lines and not lines[-1].strip():
        lines.pop()
    joined = "\n".join(lines)
    # Doxygen renders HTML, Natural Docs does not. A trailing <br> is redundant
    # once the text is a list item or its own line; mid-line it means a break.
    joined = re.sub(r"[ \t]*<br\s*/?>[ \t]*$", "", joined, flags=re.MULTILINE)
    joined = re.sub(r"[ \t]*<br\s*/?>[ \t]*", "\n\n", joined)
    lines = joined.split("\n")

    # An inline command whose target wrapped onto the next line must be
    # rejoined before the per-line cleaning can recognise it.
    joined = re.sub(
        r"[@\\](ref|see|sa|c|p|b|e|a)[ \t]*\n[ \t]*(\S)",
        lambda m: "@%s %s" % (m.group(1), m.group(2)),
        "\n".join(lines),
    )
    return joined.split("\n")


def clean_inline(line):
    line = REF_QUOTED.sub(lambda m: m.group(2), line)
    line = SEE_INLINE.sub(lambda m: m.group(1) + REF_OPEN + m.group(2) + REF_CLOSE, line)
    line = REF_BARE.sub(lambda m: REF_OPEN + m.group(1) + REF_CLOSE, line)
    line = AUTOLINK.sub(lambda m: REF_OPEN + m.group(1) + REF_CLOSE, line)
    line = INLINE_EMPHASIS.sub(lambda m: m.group(1), line)
    line = re.sub(r"[@\\](?:code|verbatim)\b\s*\{?\.?\w*\}?", "(start code)", line)
    line = re.sub(r"[@\\](?:endcode|endverbatim)\b", "(end code)", line)
    line = re.sub(r"[@\\](?:cond|endcond|f\$)\b.*", "", line)
    return line.rstrip()


class Block:
    """A parsed Doxygen comment."""

    def __init__(self):
        self.file = None
        self.brief = []
        self.body = []
        self.params = []          # (name, description-lines)
        self.returns = []
        self.notes = []
        self.see = []
        self.defgroup = None      # (identifier, title)
        self.addtogroup = None
        self.group_open = False
        self.group_close = False
        self.had_command = False

    def is_empty(self):
        return not any(
            (self.brief, self.body, self.params, self.returns,
             self.notes, self.see, self.file, self.defgroup)
        )


def parse_block(lines):
    blk = Block()
    sink = blk.brief
    param = None

    for raw in lines:
        line = clean_inline(raw)
        stripped = line.strip()
        # Leading whitespace is meaningful for nested lists, so keep it.
        content = line.rstrip()

        m = re.match(r"^[@\\](\w+)\b[ \t]*(.*)$", stripped)
        if m:
            cmd, rest = m.group(1).lower(), m.group(2).strip()
            blk.had_command = True
            param = None

            if cmd == "file":
                blk.file = rest or True
                sink = blk.brief
            elif cmd in ("brief", "short"):
                sink = blk.brief
                if rest:
                    sink.append(rest)
            elif cmd in ("details", "detail"):
                sink = blk.body
                if rest:
                    sink.append(rest)
            elif cmd == "param":
                pm = re.match(r"^(?:\[[^\]]*\]\s*)?(\w+)\s*(.*)$", rest)
                if pm:
                    param = (pm.group(1), [pm.group(2).strip()] if pm.group(2) else [])
                    blk.params.append(param)
                    sink = None
            elif cmd in ("return", "returns", "retval", "result"):
                sink = blk.returns
                if rest:
                    sink.append(rest)
            elif cmd in ("note", "warning", "attention", "remark", "remarks", "pre", "post"):
                sink = blk.notes
                label = {"warning": "Warning: ", "attention": "Warning: "}.get(cmd, "")
                if rest:
                    sink.append(label + rest)
            elif cmd in ("see", "sa"):
                sink = blk.see
                if rest:
                    sink.append(rest)
            elif cmd == "defgroup":
                gm = re.match(r"^(\S+)\s*(.*)$", rest)
                if gm:
                    blk.defgroup = (gm.group(1), gm.group(2).strip() or gm.group(1))
                sink = blk.body
            elif cmd == "addtogroup":
                gm = re.match(r"^(\S+)\s*(.*)$", rest)
                if gm:
                    blk.addtogroup = gm.group(1)
                sink = blk.body
            elif cmd == "ingroup":
                sink = blk.body
            elif cmd == "section":
                sm = re.match(r"^(\S+)\s+(.*)$", rest)
                blk.body.append(("SECTION", sm.group(2) if sm else rest))
                sink = blk.body
            elif cmd in ("subsection", "subsubsection", "par"):
                sm = re.match(r"^(\S+)\s+(.*)$", rest)
                blk.body.append(("HEADING", sm.group(2) if sm else rest))
                sink = blk.body
            elif cmd in ("struct", "enum", "union", "func", "fn", "typedef",
                         "var", "def", "class", "data", "name"):
                sink = blk.body
            elif cmd in ("{", "}"):
                pass
            else:
                # Unknown command: keep its text so nothing is silently lost.
                sink = blk.body if sink is None else sink
                if rest:
                    sink.append(rest)
            continue

        if stripped in ("@{", "/*!", "@}"):
            continue
        if stripped == "@{":
            blk.group_open = True
            continue

        if param is not None:
            if stripped:
                param[1].append(stripped)
            else:
                param = None
                sink = blk.body
            continue

        if sink is None:
            sink = blk.body
        if sink is blk.brief and stripped == "" and blk.brief:
            sink = blk.body
            continue
        sink.append(content)

    if "@{" in "\n".join(lines):
        blk.group_open = True
    if re.search(r"^\s*[@\\]\}", "\n".join(lines), re.MULTILINE):
        blk.group_close = True
    return blk


# --- identifying what the comment documents ---------------------------------

def next_code(text, pos):
    """Return the code that follows a comment, skipping intervening comments."""
    spans = {s: e for _, s, e in find_comment_spans(text)}
    i = pos
    n = len(text)
    while i < n:
        if text[i].isspace():
            i += 1
            continue
        if i in spans:
            i = spans[i]
            continue
        break
    # Large enums and structs need a window wide enough to reach the closing
    # brace, otherwise the trailing typedef name is never seen.
    return text[i:i + 60000], i


def match_brace(text, start):
    depth, i, n = 0, start, len(text)
    while i < n:
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def classify(code):
    """Return (topic_keyword, identifier) for the declaration in `code`."""
    code = code.lstrip()
    if not code:
        return None, None
    # A comment sitting inside an initialiser or just before a closing brace
    # documents nothing declarable.
    if code[0] in "})],;":
        return None, None

    m = re.match(r"#\s*define\s+(\w+)\s*\(", code)
    if m:
        return "Macro", m.group(1)
    m = re.match(r"#\s*define\s+(\w+)\s+\S", code)
    if m:
        return "Constant", m.group(1)
    if re.match(r"#\s*(include|ifndef|ifdef|if|endif|else|pragma|define)\b", code):
        return None, None

    m = re.match(r"typedef\s+(?:const\s+)?(?:struct|union|enum)\b[^{;]*\{", code)
    if m:
        close = match_brace(code, code.index("{"))
        if close != -1:
            tail = re.match(r"\s*(\w+)\s*(?:\[[^\]]*\])?\s*;", code[close + 1:])
            if tail:
                kind = "Enum" if "enum" in m.group(0) else "Struct"
                return kind, tail.group(1)
        return None, None

    m = re.match(r"typedef\s+[^;()]*\(\s*\*\s*(\w+)\s*\)\s*\(", code)
    if m:
        return "Callback", m.group(1)

    m = re.match(r"typedef\s+[^;{]*?(\w+)\s*(?:\[[^\]]*\])?\s*;", code)
    if m:
        return "Type", m.group(1)

    m = re.match(r"(?:struct|union)\s+(\w+)\s*\{", code)
    if m:
        return "Struct", m.group(1)
    m = re.match(r"enum\s+(\w+)\s*\{", code)
    if m:
        return "Enum", m.group(1)

    # Function declaration or definition.
    m = re.match(
        r"(?:(?:static|extern|inline|__inline|const|volatile|unsigned|signed|"
        r"struct|union|enum|register|__attribute__\s*\(\([^)]*\)\))\s+)*"
        r"[\w \t\*&:]+?(\w+)\s*\(",
        code,
    )
    if m:
        ident = m.group(1)
        after = code[m.end() - 1:]
        close = after.find(")")
        if close != -1:
            rest = after[close + 1:].lstrip()
            if rest.startswith(";") or rest.startswith("{") or rest.startswith("__"):
                if ident not in ("if", "for", "while", "switch", "return", "sizeof"):
                    return "Function", ident

    m = re.match(r"(?:extern\s+)?(?:const\s+)?[\w \t\*]+?(\w+)\s*(?:\[[^\]]*\])?\s*(?:=|;)", code)
    if m and m.group(1) not in ("return", "typedef"):
        return "Variable", m.group(1)

    return None, None


# --- rendering ---------------------------------------------------------------

def wrap(text, width, indent):
    words, lines, cur = text.split(), [], ""
    for w in words:
        if cur and len(cur) + 1 + len(w) > width:
            lines.append(indent + cur)
            cur = w
        else:
            cur = (cur + " " + w).strip()
    if cur:
        lines.append(indent + cur)
    return lines


def flatten(entries):
    """Turn parsed body entries into (kind, text) prose paragraphs."""
    out = []
    for item in entries:
        if isinstance(item, tuple):
            out.append(item)
        else:
            out.append(("TEXT", item))
    return out


def render(blk, keyword, ident, indent, allow_sections):
    """Produce the Natural Docs comment text for one parsed block."""
    pad = indent + "    "
    width = 78 - len(pad)
    lines = ["/*"]

    if keyword and ident:
        lines.append(pad + "%s: %s" % (keyword, ident))
        lines.append("")

    def emit_prose(entries):
        blank = False
        buffer = []

        def flush():
            if buffer:
                lines.extend(wrap(" ".join(buffer), width, pad))
                buffer.clear()

        for kind, text in entries:
            if kind == "SECTION":
                flush()
                if lines and lines[-1] != "":
                    lines.append("")
                lines.append(pad + ("Section: %s" % text if allow_sections else "%s:" % text))
                lines.append("")
                continue
            if kind == "HEADING":
                flush()
                if lines and lines[-1] != "":
                    lines.append("")
                lines.append(pad + "%s:" % text.rstrip(":"))
                lines.append("")
                continue
            if not text.strip():
                flush()
                if lines and lines[-1] != "":
                    lines.append("")
                continue
            if re.match(r"^\s*[-*+]\s+|^\s*\d+[.)]\s+|^\s*\(?(start|end) code\)", text):
                flush()
                lines.append((pad + text).rstrip())
                blank = True
                continue
            buffer.append(text.strip())
        flush()
        return blank

    prose = flatten(blk.brief) + ([("TEXT", "")] if blk.brief and blk.body else []) + flatten(blk.body)
    emit_prose(prose)

    while lines and lines[-1] == "":
        lines.pop()

    if blk.params:
        lines.append("")
        lines.append(pad + "Parameters:")
        lines.append("")
        for name, desc in blk.params:
            text = " ".join(d for d in desc if d.strip()).strip()
            first = "%s%s - %s" % (pad + "    ", name, text) if text else "%s%s -" % (pad + "    ", name)
            wrapped = wrap(first.strip(), width - 8, pad + "        ")
            if wrapped:
                wrapped[0] = pad + "    " + wrapped[0].strip()
            lines.extend(wrapped)

    if blk.returns:
        lines.append("")
        lines.append(pad + "Returns:")
        lines.append("")
        for chunk in blk.returns:
            if not chunk.strip():
                continue
            if re.match(r"^\s*[-*+]\s+", chunk):
                lines.append(pad + "    " + chunk)
            else:
                lines.extend(wrap(chunk, width - 4, pad + "    "))

    if blk.notes:
        lines.append("")
        lines.append(pad + "Notes:")
        lines.append("")
        for chunk in blk.notes:
            if not chunk.strip():
                continue
            lines.extend(wrap(chunk, width - 4, pad + "    "))

    if blk.see:
        lines.append("")
        lines.append(pad + "See Also:")
        lines.append("")
        for chunk in blk.see:
            if chunk.strip():
                lines.extend(wrap(chunk, width - 4, pad + "    "))

    while lines and lines[-1] == "":
        lines.pop()
    lines.append(indent + "*/")

    if len(lines) == 2:
        return None
    return "\n".join(lines)


def existing_nd_topics(text):
    """Topic names already documented by hand-written Natural Docs comments."""
    names = set()
    for kind, start, end in find_comment_spans(text):
        body = text[start:end]
        if kind != "block" or is_doxygen(body, kind):
            continue
        for m in ND_TOPIC.finditer(body):
            names.add(m.group(2))
    return names


def convert(text, path, header_topics=None, collect_only=False):
    """Rewrite the Doxygen comments in `text`. Returns (new_text, topics, stats)."""
    filename = os.path.basename(path)
    is_source = path.endswith((".c", ".S", ".s"))
    preexisting = existing_nd_topics(text)
    topics = set(preexisting)
    stats = {"converted": 0, "dropped": 0, "plain": 0}

    pieces, prev = [], 0
    spans = find_comment_spans(text)
    seen_file_topic = "File" in {m.group(1) for m in ND_TOPIC.finditer(text)}

    # Doxygen groups are declared once and populated by @addtogroup regions,
    # whereas Natural Docs headings are positional. Learn the titles up front so
    # each @addtogroup can become a heading in the right place, and fold the
    # module-level @defgroup prose into the File topic instead of duplicating it.
    group_titles = {}
    populated = set()
    module_block = None
    module_span = None
    has_file_block = False
    for kind, start, end in spans:
        chunk = text[start:end]
        if not is_doxygen(chunk, kind):
            continue
        parsed = parse_block(comment_body(chunk, kind))
        if parsed.file is not None:
            has_file_block = True
        if parsed.addtogroup:
            populated.add(parsed.addtogroup)
        if parsed.defgroup:
            group_titles[parsed.defgroup[0]] = parsed.defgroup[1]
            if parsed.group_open and module_span is None:
                module_block, module_span = parsed, (start, end)

    idx = 0
    while idx < len(spans):
        kind, start, end = spans[idx]
        body = text[start:end]

        if not is_doxygen(body, kind):
            idx += 1
            continue

        # Merge a run of consecutive /// or //! lines into one comment.
        group_end = end
        if kind == "line":
            j = idx + 1
            while j < len(spans):
                k2, s2, e2 = spans[j]
                between = text[group_end:s2]
                if k2 == "line" and is_doxygen(text[s2:e2], k2) and between.strip() == "":
                    group_end = e2
                    j += 1
                    continue
                break
            body = text[start:group_end]
            idx = j - 1

        line_start = text.rfind("\n", 0, start) + 1
        indent = text[line_start:start]
        starts_line = not indent.strip()
        if not starts_line:
            indent = ""
        line_end = text.find("\n", group_end)
        ends_line = not text[group_end:line_end if line_end != -1 else len(text)].strip()
        own_line = starts_line and ends_line

        lines = comment_body(body, kind)
        blk = parse_block(lines)

        # A multi-line topic cannot be spliced into the middle of a line of
        # code, so documentation sitting inline is left for manual review.
        if not own_line and not blk.is_empty():
            idx += 1
            continue

        code, _ = next_code(text, group_end)
        keyword, ident = classify(code)

        # Internal helpers stay out of the generated reference, so their
        # documentation becomes an ordinary comment with no topic keyword.
        if keyword == "Function" and re.match(r"\s*static\b", code):
            keyword, ident = None, None

        # Decide which topic, if any, this block becomes.
        drop = False
        is_module = module_span == (start, group_end)

        # A module-level @defgroup with no @file anywhere is the file's own
        # description, so it becomes the File topic rather than a heading.
        defgroup_is_file = (
            blk.defgroup is not None and not has_file_block and not seen_file_topic
            and blk.defgroup[0] not in populated
        )

        if blk.file is not None or (is_module and not seen_file_topic) or defgroup_is_file:
            if seen_file_topic:
                drop = True
            else:
                keyword, ident = "File", filename
                seen_file_topic = True
                if module_block is not None and not is_module:
                    # Carry the module description into the file topic.
                    if blk.body or blk.brief:
                        blk.body.append("")
                    blk.body.extend(module_block.brief + [""] + module_block.body)
        elif is_module:
            drop = True
        elif blk.addtogroup:
            keyword = "Group"
            ident = group_titles.get(blk.addtogroup, blk.addtogroup)
        elif blk.defgroup:
            # A bare declaration is redundant once its @addtogroup region
            # supplies the heading at the position the members actually appear.
            if blk.defgroup[0] in populated:
                drop = True
            elif blk.brief or blk.body:
                keyword, ident = "Group", blk.defgroup[1]
            else:
                drop = True
        elif blk.is_empty():
            # Scaffolding such as a lone `@}`; it documents nothing, and must
            # not adopt the declaration that happens to follow it.
            drop = True
        elif keyword and ident:
            if ident in preexisting:
                drop = True
            elif is_source and header_topics and ident in header_topics:
                keyword, ident = None, None   # documented in the header already

        if collect_only:
            if not drop and keyword and ident:
                topics.add(ident)
            idx += 1
            continue

        if drop:
            stats["dropped"] += 1
            replacement = None
        else:
            replacement = render(blk, keyword, ident, indent, allow_sections=(keyword == "File"))
            if replacement is None:
                stats["dropped"] += 1
            elif keyword:
                stats["converted"] += 1
                topics.add(ident)
            else:
                stats["plain"] += 1

        pieces.append(text[prev:start])
        if replacement is None and not own_line:
            # Mid-line comment: drop it but keep the tokens either side apart.
            if (start and not text[start - 1].isspace()
                    and group_end < len(text) and not text[group_end].isspace()):
                pieces.append(" ")
            prev = group_end
        elif replacement is None:
            # Remove the comment and the blank line it leaves behind.
            tail = group_end
            while tail < len(text) and text[tail] in " \t":
                tail += 1
            if tail < len(text) and text[tail] == "\n":
                tail += 1
                while tail < len(text) and text[tail] in " \t":
                    nxt = text.find("\n", tail)
                    if nxt == -1 or text[tail:nxt].strip():
                        break
                    tail = nxt + 1
            if pieces and pieces[-1].endswith("\n\n") and indent == "":
                pieces[-1] = pieces[-1][:-1]
            prev = tail
        else:
            pieces.append(replacement)
            prev = group_end
        idx += 1

    pieces.append(text[prev:])
    result = "".join(pieces)

    # Without a File topic Natural Docs titles the page after whatever topic
    # comes first, which reads as nonsense in the menu. Give such files a bare
    # File topic; the filename is a title, not invented documentation.
    if stats["converted"] and not re.search(r"(?m)^[ \t]*File:[ \t]", result):
        first = re.search(
            r"(?m)^([ \t]*)/\*\n[ \t]*(?:Function|Struct|Enum|Type|Constant|"
            r"Macro|Callback|Variable|Group|Section):",
            result,
        )
        if first:
            pad = first.group(1)
            header = "%s/*\n%s    File: %s\n%s*/\n\n" % (pad, pad, filename, pad)
            result = result[:first.start()] + header + result[first.start():]

    # Trailing member comments carry no Natural Docs meaning; keep the prose
    # but drop the Doxygen marker and any commands inside so no Doxygen syntax
    # survives.
    def detag(inner):
        inner = clean_inline(inner).strip()
        inner = re.sub(r"^[@\\](?:brief|details?|short)\s+", "", inner)
        inner = re.sub(r"^[@\\]note\s+", "Note: ", inner)
        inner = re.sub(r"^[@\\](?:warning|attention)\s+", "Warning: ", inner)
        return inner

    result = re.sub(
        r"/\*[!*]<(.*?)\*/",
        lambda m: "/* " + detag(m.group(1)) + " */",
        result,
        flags=re.S,
    )
    result = re.sub(
        r"//[!/]<(.*)",
        lambda m: "// " + detag(m.group(1)),
        result,
    )

    # Dropped Doxygen scaffolding can leave long runs of empty lines behind.
    result = re.sub(r"\n{4,}", "\n\n", result)

    return result, topics, stats


def resolve_refs(text, known):
    def repl(m):
        name = m.group(1)
        return "<%s>" % name if name in known else name
    return re.sub(REF_OPEN + r"(\w+)" + REF_CLOSE, repl, text)


def gather(paths):
    files = []
    for root in paths:
        if os.path.isfile(root):
            files.append(root)
            continue
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [d for d in dirnames if d not in (".git", "build")]
            for name in sorted(filenames):
                if name.endswith(C_EXTENSIONS):
                    files.append(os.path.join(dirpath, name))
    return sorted(set(files))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    files = gather(args.paths)

    # Pass 1: learn which names the headers own, so sources do not restate them.
    header_topics = set()
    for path in files:
        if not path.endswith(".h"):
            continue
        try:
            text = open(path, encoding="utf-8", errors="surrogateescape").read()
        except OSError:
            continue
        _, topics, _ = convert(text, path, collect_only=True)
        header_topics |= topics

    # Pass 2: rewrite, recording the topics that actually survived.
    pending, emitted = [], set()
    for path in files:
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        new, topics, stats = convert(text, path, header_topics=header_topics)
        emitted |= topics
        pending.append((path, text, new, stats))

    # Pass 3: only link to identifiers that really ended up documented, so no
    # reference renders as literal angle brackets.
    changed, failed, totals = [], [], {"converted": 0, "dropped": 0, "plain": 0}
    for path, text, new, stats in pending:
        new = resolve_refs(new, emitted)
        if new == text:
            continue
        if strip_comments(text) != strip_comments(new):
            failed.append(path)
            continue
        for k in totals:
            totals[k] += stats[k]
        changed.append(path)
        if args.apply:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)

    print("files scanned      : %d" % len(files))
    print("files changed      : %d" % len(changed))
    print("topics converted   : %d" % totals["converted"])
    print("blocks kept as prose: %d" % totals["plain"])
    print("blocks dropped     : %d" % totals["dropped"])
    if failed:
        print("\nREFUSED (code text would change) : %d" % len(failed))
        for path in failed:
            print("   " + path)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
