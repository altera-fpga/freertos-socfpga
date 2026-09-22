#!/usr/bin/env python3
"""Describe files whose `File:` topic is a bare title.

Some files came out of the Doxygen conversion with a title and nothing else,
so their page opens with a heading and no explanation of what the file is for.
This fills those in from what the file actually contains: whether it declares
an interface, defines a register map, carries compile-time macros, or
implements something.

Only files with an empty or placeholder description are touched; anything with
real prose is left alone.

    python3 tools/nd_describe.py --dry-run drivers samples ...
    python3 tools/nd_describe.py --apply   drivers samples ...
"""

import argparse
import collections
import os
import re
import sys
import textwrap

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dox2nd
import nd_cfuncs
import nd_stubs

TOPIC = re.compile(
    r"(?m)^[ \t]*(File|Functions?|Types?|Struct|Enum|Constants?|Macros?|"
    r"Callback|Variables?|Group|Section):[ \t]+(\S.*?)[ \t]*$"
)
# Names that mark a #define as part of a register map rather than a setting.
REGISTER_NAME = re.compile(
    r"(?i)_(ofst|offset|addr|address|mask|shift|reg|bit|pos|width)$|^reg_|_reg_")
PLACEHOLDER = re.compile(r"(?i)\bbrief\b|group_\w+")

# Role words nd_titles appends to a title; the component is what is left.
ROLES = [
    "Sample Entry Point", "Entry Point", "Low-Level Driver", "CLI Commands",
    "Driver", "Library", "Sample", "Registers", "Definitions", "Utilities",
    "Helpers", "Configuration", "Demo", "OS Abstraction",
]


def plain(title):
    """The title without the markers nd_titles adds to keep titles unique."""
    return re.sub(r"\s*\(\d+\)$", "", re.sub(r"\s+Implementation$", "", title)).strip()


def component_of(title):
    """The subject alone: "QSPI Flash Adapter Driver" -> "QSPI Flash Adapter"."""
    name = plain(title)
    for role in ROLES:
        if name.endswith(" " + role):
            name = name[: -len(role) - 1].strip()
            break
    return re.sub(r"\s+Configs?$", "", name).strip()


def prose_name(title):
    """The title as it reads mid-sentence: "I2C Low-Level Driver" -> "I2C
    low-level driver"."""
    name = plain(title)
    for role in ROLES:
        if name.endswith(" " + role) and name != role:
            return name[: -len(role)] + role.lower()
    return name


def count(n, singular, plural=None):
    return "%d %s" % (n, singular if n == 1 else (plural or singular + "s"))


def join(parts):
    if len(parts) == 1:
        return parts[0]
    return "%s and %s" % (", ".join(parts[:-1]), parts[-1])


def without_bodies(text, code):
    """`code` with every function body blanked, offsets preserved.

    A header full of inline wrappers otherwise looks like it declares every
    kernel function those wrappers call.
    """
    out = code
    for _, offset in nd_cfuncs.definitions(text)[0]:
        brace = code.find("{", offset)
        if brace < 0:
            continue
        depth = 0
        for i in range(brace, len(code)):
            if code[i] == "{":
                depth += 1
            elif code[i] == "}":
                depth -= 1
                if depth == 0:
                    out = out[:brace + 1] + " " * (i - brace - 1) + out[i:]
                    break
    return out


def profile(path, text):
    code = nd_cfuncs.masked(text, dox2nd.find_comment_spans(text))
    outer = without_bodies(text, code)
    names = re.findall(r"(?m)^\s*#\s*define\s+(\w+)", code)
    # The include guard is not a constant anyone cares about.
    guard = re.search(r"#\s*ifndef\s+(\w+)\s*\n\s*#\s*define\s+\1\b", code)
    if guard and guard.group(1) in names:
        names.remove(guard.group(1))
    return dict(
        defines=len(names),
        registerish=sum(1 for n in names if REGISTER_NAME.search(n)),
        fnmacros=len(re.findall(r"(?m)^\s*#\s*define\s+\w+\(", code)),
        conditionals=len(re.findall(r"(?m)^\s*#\s*(?:if|ifdef|ifndef|elif)\b", code)),
        types=len(re.findall(r"(?m)^\s*typedef\b", code)),
        enums=len(re.findall(r"\benum\b[^;{]*\{", code)),
        structs=len(re.findall(r"(?m)^\s*(?:struct|union)\s+\w+\s*\{", code)),
        declarations=len(set(nd_cfuncs.HEADER_DECL.findall(outer))),
        inline=len(nd_cfuncs.definitions(text)[0]),
        tables=len(re.findall(
            r"(?m)^\s*static\s+(?:const\s+)?[\w\s\*]+\w+\s*(?:\[[^\]]*\])?\s*=", code)),
    )


def describe(path, text, declared_in_headers):
    """(sentence, inventory) for this file, or (None, None).

    The inventory is the part after the colon, so it can also be grafted onto
    a description that already exists but only repeats the title.
    """
    title = TOPIC.search(text).group(2)
    comp = component_of(title)
    subject = prose_name(title)
    p = profile(path, text)

    if path.endswith((".c", ".S", ".s")):
        defs = [n for n, _ in nd_cfuncs.definitions(text)[0]]
        if not defs:
            return None, None
        local = [n for n in defs if n not in declared_in_headers]
        inventory = count(len(defs), "function")
        if len(local) == len(defs):
            inventory += ", local to this file" if len(defs) == 1 \
                else ", all of them local to this file"
        elif local:
            inventory += ", %d of them local to this file" % len(local)
        return "Implements the %s: %s." % (subject, inventory), inventory

    if p["defines"] >= 10 and p["registerish"] >= 0.5 * p["defines"]:
        inventory = count(p["defines"], "definition")
        return ("Register offsets, masks and bit positions for the %s: %s."
                % (comp, inventory), inventory)

    if p["fnmacros"] >= 10 and p["declarations"] == 0:
        parts = [count(p["fnmacros"], "function-like macro")]
        if p["defines"] - p["fnmacros"] > 0:
            parts.append(count(p["defines"] - p["fnmacros"], "constant"))
        # No backticks: Natural Docs 1.4 has no inline code markup and would
        # print them verbatim.
        inventory = ("%s, guarded by #ifdef so a build selects the variant "
                     "that suits its compiler and configuration" % join(parts))
        return "Defines %s." % inventory, inventory

    # A config header is mostly knobs; the few declarations in it are hooks
    # the application is expected to supply, not an interface it calls.
    if p["defines"] >= 10 and re.search(r"(?i)config", os.path.basename(path)):
        parts = [count(p["defines"], "definition")]
        if p["declarations"]:
            parts.append(count(p["declarations"], "declaration"))
        inventory = join(parts)
        return "Compile-time settings: %s." % inventory, inventory

    if p["declarations"] or p["inline"]:
        parts = []
        if p["declarations"]:
            parts.append(count(p["declarations"], "function"))
        if p["inline"]:
            parts.append(count(p["inline"], "inline function"))
        if p["types"]:
            parts.append(count(p["types"], "type"))
        if p["defines"]:
            parts.append(count(p["defines"], "constant"))
        inventory = join(parts)
        return "Declares the %s interface: %s." % (subject, inventory), inventory

    if p["tables"]:
        inventory = count(p["tables"], "table")
        return ("Compile-time configuration data for the %s: %s."
                % (comp, inventory), inventory)

    if p["types"] or p["enums"] or p["structs"]:
        parts = []
        if p["types"]:
            parts.append(count(p["types"], "type"))
        if p["structs"]:
            parts.append(count(p["structs"], "structure"))
        if p["enums"]:
            parts.append(count(p["enums"], "enumeration"))
        inventory = join(parts)
        return "Type definitions for the %s: %s." % (comp, inventory), inventory

    if p["defines"]:
        inventory = count(p["defines"], "definition")
        return ("Compile-time settings for the %s: %s."
                % (comp, inventory), inventory)
    return None, None


def names_in(path, text):
    """(kind, names) for the thing this file is mostly made of."""
    code = nd_cfuncs.masked(text, dox2nd.find_comment_spans(text))
    macros = re.findall(r"(?m)^\s*#\s*define\s+(\w+)", code)
    guard = re.search(r"#\s*ifndef\s+(\w+)\s*\n\s*#\s*define\s+\1\b", code)
    if guard and guard.group(1) in macros:
        macros.remove(guard.group(1))
    defined = [n for n, _ in nd_cfuncs.definitions(text)[0]]
    functions = defined or sorted(set(
        nd_cfuncs.HEADER_DECL.findall(without_bodies(text, code))))
    tables = re.findall(
        r"(?m)^\s*static\s+(?:const\s+)?[\w\s\*]*?(\w+)\s*(?:\[[^\]]*\])?\s*=", code)
    types = re.findall(r"(?m)^\s*}\s*(\w+)\s*;", code)
    for kind, names in (("definitions", macros), ("functions", functions),
                        ("tables", tables), ("types", types)):
        if len(names) >= max(2, len(functions) if kind == "definitions" else 0):
            if names:
                return kind, names
    return None, []


def contents_sentence(path, text):
    """A sentence naming what is in the file, for pages that show nothing else."""
    kind, names = names_in(path, text)
    if not names:
        return None
    if len(names) <= 10:
        return "They are %s." % join(names)
    families = collections.Counter()
    for n in names:
        parts = n.lstrip("_").split("_")
        families["_".join(parts[:2])] += 1 if len(parts) > 1 else 0
    top = [f for f, c in families.most_common() if c >= 3][:6]
    # Naming a handful of families is only honest if they cover the file.
    if not top or sum(families[f] for f in top) < 0.4 * len(names):
        return None
    rest = sum(1 for f, c in families.items() if c and f not in top)
    tail = ", plus %d smaller groups" % rest if rest > 2 else ""
    return ("The %s fall into the %s families%s."
            % (kind, join(["%s_*" % f for f in top]), tail))


def is_generated(flat, filler):
    """Whether this description is one we wrote, and may rewrite."""
    if not re.search(r"\b\d+ (?:function|type|constant|definition|table|"
                     r"structure|enumeration|declaration|function-like macro)",
                     flat):
        return False
    leads = ("Declares the ", "Implements the ", "Register offsets, ",
             "Compile-time settings", "Compile-time configuration data for ",
             "Type definitions for ", "Defines ", "Data tables for ")
    return flat.startswith(leads) or flat.startswith(filler.rstrip("."))


def paragraphs_of(lines):
    """The blank-line separated paragraphs of a topic body."""
    out, current = [], []
    for line in lines:
        if line.strip():
            current.append(line)
        elif current:
            out.append(current)
            current = []
    if current:
        out.append(current)
    return out


def file_topic_body(lines):
    """(line of the File topic, first line after its description, the text).

    The description runs from just after the `File:` line to the next topic
    or the end of the comment, whichever comes first.
    """
    start = next((i for i, l in enumerate(lines) if TOPIC.match(l)), None)
    if start is None or TOPIC.match(lines[start]).group(1) != "File":
        return None
    end = start + 1
    while end < len(lines):
        if TOPIC.match(lines[end]) or "*/" in lines[end]:
            break
        end += 1
    return start, end, "\n".join(lines[start + 1:end]).strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    declared = nd_cfuncs.header_declarations(args.paths)
    written = skipped = 0

    for path in dox2nd.gather(args.paths):
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        lines = text.split("\n")
        found = file_topic_body(lines)
        if not found:
            continue
        start, end, body = found
        flat = re.sub(r"\s+", " ", body)
        sentence, inventory = describe(path, text, declared)
        if not sentence:
            skipped += 1
            continue

        # A description made only from the path repeats the title and says
        # nothing else, so keep its wording but graft the inventory on.
        filler = nd_stubs.derived_description(path)
        grafted = "%s: %s." % (filler.rstrip("."), inventory)
        if not flat or PLACEHOLDER.search(flat):
            description = sentence
        elif flat == filler:
            description = (sentence
                           if filler.startswith(("Definitions for ",
                                                 "Implementation of "))
                           else grafted)
        elif is_generated(flat, filler):
            description = grafted if flat.startswith(filler.rstrip(".")) \
                else sentence
        else:
            description = None  # the file's own prose; keep it verbatim

        # On a page with no member topics the description is all the reader
        # gets, so name what is inside as well as counting it.
        # A page with no member topics shows the description and nothing else,
        # so name the contents there. Once they are listed on the page that
        # sentence only repeats it, and comes back out.
        kinds = set(m.group(1) for m in TOPIC.finditer(text))
        listing = contents_sentence(path, text)
        wanted = listing if (kinds <= {"File", "Group", "Section"}
                             and len(flat) < 240) else None
        stale = re.sub(r"\s+", " ", listing) if listing and not wanted else None

        indent = re.match(r"[ \t]*", lines[start]).group(0)
        wrap = lambda para: textwrap.wrap(para, width=76,
                                          initial_indent=indent,
                                          subsequent_indent=indent)
        block = []
        if description is not None:
            block += [""] + wrap(description)
        else:
            for para in paragraphs_of(lines[start + 1:end]):
                joined = re.sub(r"\s+", " ", " ".join(para).strip())
                if stale and joined == stale:
                    continue
                if wanted and joined == re.sub(r"\s+", " ", wanted):
                    continue  # re-added below, in its canonical place
                block += [""] + para
        if wanted:
            block += [""] + wrap(wanted)
        while block and not block[-1].strip():
            block.pop()
        new = "\n".join(lines[:start + 1] + block + lines[end:])
        if new == text:
            continue

        if dox2nd.strip_comments(new) != dox2nd.strip_comments(text):
            print("REFUSED (code would change): %s" % path, file=sys.stderr)
            continue
        written += 1
        if args.apply:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)
        if args.dry_run:
            print("%-44s %s" % (path[-42:], " | ".join(
                p for p in (description or "(kept)", extra) if p)))

    print("\ndescriptions written : %d" % written)
    print("nothing to say about : %d" % skipped)
    return 0


if __name__ == "__main__":
    sys.exit(main())
