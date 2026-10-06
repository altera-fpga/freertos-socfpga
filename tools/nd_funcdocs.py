#!/usr/bin/env python3
"""Fill in function topics that carry a name and nothing else.

A topic with an empty body lists a function in the summary without saying
anything about it. This writes a description for the ones that can be
described honestly:

  * the FreeRTOS application hooks, whose contract is fixed by the kernel;
  * the entry-point family every sample repeats (main, samples_main,
    run_samples, prvSetupHardware), described from that file's own body so the
    task names and setup steps are the real ones;
  * any other function whose body is a short sequence of calls, which becomes
    a sentence naming those calls, cross-linked where the target is documented.

Functions with a body too involved to summarize mechanically are left alone
rather than given filler.

    python3 tools/nd_funcdocs.py --dry-run samples ...
    python3 tools/nd_funcdocs.py --apply   samples ...
"""

import argparse
import os
import re
import sys
import textwrap

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dox2nd
import nd_cfuncs
import nd_describe
import nd_titles

TOPIC = re.compile(
    r"(?m)^([ \t]*)(File|Function|Type|Struct|Enum|Constant|Macro|Callback|"
    r"Variable|Group):[ \t]+(\S.*?)[ \t]*$")

# Calls that are noise in a summary of what a function does.
IGNORED_CALLS = {
    "if", "for", "while", "switch", "return", "sizeof", "do", "else",
    "printf", "sprintf", "snprintf", "puts", "putchar", "memset", "memcpy",
    "strcpy", "strlen", "strcmp", "configASSERT", "assert", "va_start",
    "va_end", "offsetof", "container_of", "sizeof", "NULL",
    "PRINT", "PRINTF", "PRINT_ERR", "LOG", "printk", "pvPortMalloc",
    "vPortFree", "malloc", "free", "taskDISABLE_INTERRUPTS",
    "taskENTER_CRITICAL", "taskEXIT_CRITICAL",
    "pdMS_TO_TICKS", "pdTICKS_TO_MS", "portYIELD_FROM_ISR", "pdTRUE",
    "pdFALSE", "pdPASS",
    "defined",  # from #if defined(...), not a call
}

# The kernel defines what these are for, so the wording is the same everywhere.
HOOKS = {
    "vApplicationTickHook": (
        "Tick hook, called by the kernel from the RTOS tick interrupt.",
        "It runs in an interrupt context on every tick, so it must return "
        "quickly and may only use the interrupt-safe API. FreeRTOS calls it "
        "only when configUSE_TICK_HOOK is 1.", None, None),
    "vApplicationIdleHook": (
        "Idle hook, called repeatedly from the idle task.",
        "It must never attempt to block or suspend, because the idle task has "
        "to stay ready to run. FreeRTOS calls it only when configUSE_IDLE_HOOK "
        "is 1.", None, None),
    "vApplicationMallocFailedHook": (
        "Called when the kernel cannot allocate memory.",
        "pvPortMalloc() calls this when it has no block large enough, which "
        "happens while creating a task, queue, timer or semaphore once the "
        "heap set by configTOTAL_HEAP_SIZE is exhausted. FreeRTOS calls it "
        "only when configUSE_MALLOC_FAILED_HOOK is 1.", None, None),
    "vApplicationStackOverflowHook": (
        "Called when the kernel detects that a task has overrun its stack.",
        "Stack checking, and therefore this hook, is only active when "
        "configCHECK_FOR_STACK_OVERFLOW is set.",
        [("xTask", "Handle of the task whose stack overflowed."),
         ("pcTaskName", "Name of that task.")], None),
    "vApplicationDaemonTaskStartupHook": (
        "Called once by the timer service task before it processes its first "
        "command.",
        "It runs in a task context, so unlike the interrupt-level hooks it may "
        "use the blocking API.", None, None),
    "vApplicationGetIdleTaskMemory": (
        "Supplies the statically allocated memory the idle task runs from.",
        "The kernel calls this when configSUPPORT_STATIC_ALLOCATION is 1, "
        "because it will not allocate the idle task itself.",
        [("ppxIdleTaskTCBBuffer", "Set to the task control block to use."),
         ("ppxIdleTaskStackBuffer", "Set to the stack to use."),
         ("pulIdleTaskStackSize", "Set to the length of that stack, in words.")],
        None),
    "vApplicationGetTimerTaskMemory": (
        "Supplies the statically allocated memory the timer service task runs "
        "from.",
        "The kernel calls this when configSUPPORT_STATIC_ALLOCATION is 1, "
        "because it will not allocate the timer task itself.",
        [("ppxTimerTaskTCBBuffer", "Set to the task control block to use."),
         ("ppxTimerTaskStackBuffer", "Set to the stack to use."),
         ("pulTimerTaskStackSize", "Set to the length of that stack, in words.")],
        None),
    "vAssertCalled": (
        "Called when a configASSERT() check fails.",
        "The kernel has detected a condition it cannot continue from, so this "
        "does not return to the caller.", None, None),
}


def link(name, documented):
    return "<%s>" % name if name in documented else name


def join(parts):
    if not parts:
        return ""
    if len(parts) == 1:
        return parts[0]
    return "%s and %s" % (", ".join(parts[:-1]), parts[-1])


def body_of(text, code, offset):
    """(body_text, body_code) for the function whose declaration starts here."""
    open_brace = code.find("{", offset)
    if open_brace < 0:
        return None, None
    depth, i = 0, open_brace
    while i < len(code):
        if code[i] == "{":
            depth += 1
        elif code[i] == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace + 1:i], code[open_brace + 1:i]
        i += 1
    return None, None


def parameters_of(code, offset):
    """[(type, name)] from the definition's parameter list."""
    start = code.find("(", offset)
    if start < 0:
        return []
    depth, i = 0, start
    while i < len(code):
        if code[i] == "(":
            depth += 1
        elif code[i] == ")":
            depth -= 1
            if depth == 0:
                break
        i += 1
    inner = code[start + 1:i].strip()
    if not inner or inner == "void":
        return []
    out = []
    for part in re.split(r",(?![^(]*\))", inner):
        part = part.strip()
        m = re.search(r"(\w+)\s*(\[[^\]]*\])?$", part)
        if m:
            out.append((part[:m.start(1)].strip(), m.group(1)))
    return out


def top_level_calls(body_code, own_name, any_depth=False):
    """Names called in the body, in order, without repeats."""
    depth, out = 0, []
    for m in re.finditer(r"[{}]|\b(\w+)\s*\(", body_code):
        if m.group(0) in "{}":
            depth += 1 if m.group(0) == "{" else -1
            continue
        name = m.group(1)
        if (depth != 0 and not any_depth) or name == own_name \
                or name in IGNORED_CALLS:
            continue
        if name not in out:
            out.append(name)
    return out


def return_type(code, offset, name):
    head = code[offset:code.find("(", offset)]
    return re.sub(r"\b%s\b\s*$" % re.escape(name), "", head).strip()


def never_returns(body_code):
    tail = body_code[-260:]
    return bool(re.search(r"(?:for\s*\(\s*;\s*;\s*\)|while\s*\(\s*1\s*\))\s*"
                          r"(?:\{\s*\}|;)?\s*$", tail.strip()))


def returned_literal(body_code):
    m = re.search(r"return\s+([A-Za-z0-9_]+)\s*;", body_code)
    return m.group(1) if m else None


def unused(name, body_code):
    """Whether the body casts the parameter away and never uses it.

    The cast alone is not enough: it is often in one arm of an #if, with the
    other arm passing the parameter on.
    """
    casts = re.findall(r"\(\s*void\s*\)\s*\(?\s*%s\s*\)?\s*;" % re.escape(name),
                       body_code)
    if not casts:
        return False
    return len(re.findall(r"\b%s\b" % re.escape(name), body_code)) == len(casts)


def declaration_comment(name, text, code, spans, definition_offset):
    """Prose from the comment above this function's forward declaration.

    Files that declare their file-local functions up front often explain them
    there, which is a better description than anything derived from the body.
    """
    for m in nd_cfuncs.HEADER_DECL.finditer(code):
        if m.group(1) != name:
            continue
        if abs(m.start() - definition_offset) < 4:
            continue
        # The same shape matches a call statement, and the comment above a
        # call describes the call. A declaration names a return type first.
        lead = code[m.start():m.start(1)].strip()
        if not re.fullmatch(r"[A-Za-z_]\w*(?:[\s*]+[A-Za-z_]\w*)*[\s*]*", lead):
            continue
        if lead.split()[0] in nd_cfuncs.KEYWORDS:
            continue
        above = [(kind, s, e) for kind, s, e in spans
                 if e <= m.start() and not text[e:m.start()].strip()]
        if not above:
            continue
        kind, start, end = above[-1]
        lines = [l.strip() for l in dox2nd.comment_body(text[start:end], kind)]
        lines = [l for l in lines if l.strip()]
        if not lines or not nd_cfuncs.is_prose(lines):
            continue
        sentence = re.sub(r"\s+", " ", " ".join(lines)).strip()
        if len(re.findall(r"[a-z]{3,}", sentence)) < 4:
            continue
        if not sentence.endswith((".", "!", "?")):
            sentence += "."
        return sentence[0].upper() + sentence[1:]
    return None


def sample_subject(path):
    """The sample or component this file belongs to, for the prose."""
    title = nd_titles.title_for(path)
    comp = nd_describe.component_of(title)
    if "/samples/" in path or re.search(r"(?i)\bsample\b", title):
        return comp + " sample"
    return comp


# A topic this tool wrote before it knew to name the register. Nobody writes
# this sentence by hand, so emptying it and deriving it again loses nothing.
MACRO = r"(?:RD|WR)_[A-Z0-9_]*REG\w*|(?:ENABLE|DISABLE)_[A-Z0-9_]*BIT\w*"
WEAK_BODY = re.compile(
    r"(?m)^([ \t]*)/\*\n([ \t]*Function:[ \t]+\w+\n)\n"
    r"[ \t]*(?:Wrapper around|Calls) (?:%s)"
    r"(?:(?:,| and) (?:%s))*\.\n\1\*/\n" % (MACRO, MACRO))

REG_REF = re.compile(r"\b(RD|WR)_REG32\s*\(\s*\(?\s*\w+\s*\+\s*([A-Z]\w*)")
BIT_CALL = re.compile(
    r"^(ENABLE|DISABLE)_BIT\s*\(\s*\(?\s*\w+\s*\+\s*([A-Z]\w*)\)?\s*,\s*(.+)\)$")
SET_BIT = re.compile(r"^\w+\s*\|=\s*(.+)$")
CLEAR_BIT = re.compile(r"^\w+\s*&=\s*~\s*(.+)$")
# A local that only exists to carry the register value through.
SCRATCH = re.compile(r"^(?:const\s+)?\w+\s+\*?\w+$|^return\s+\w+$")


INITIALIZER = re.compile(r"\{([^{}]*)\}", re.S)
LITERALS = re.compile(r'"((?:[^"\\]|\\.)*)"')
_cli_tables = {}


def cli_command(name, path):
    """The command a CLI handler implements, and the help the CLI prints.

    The interpreter table pairs each handler with the word the user types and
    a line of help. That line is the description, written by the people who
    wrote the command.
    """
    table = os.path.join(os.path.dirname(path), "cli_app.c")
    if table not in _cli_tables:
        try:
            _cli_tables[table] = open(table, encoding="utf-8",
                                      errors="surrogateescape").read()
        except OSError:
            _cli_tables[table] = ""
    for m in INITIALIZER.finditer(_cli_tables[table]):
        entry = m.group(1)
        if not re.search(r"\.pxCommandInterpreter\s*=\s*%s\b"
                         % re.escape(name), entry):
            continue
        word = re.search(r'\.pcCommand\s*=\s*"([^"]*)"', entry)
        helped = re.search(r"\.pcHelpString\s*=\s*((?:\s*\"(?:[^\"\\]|\\.)*\")+)",
                           entry)
        if not word or not helped:
            continue
        summary = "".join(LITERALS.findall(helped.group(1)))
        summary = re.sub(r"\\[rnt]", " ", summary)
        summary = re.sub(r"\s+", " ", summary).strip()
        # The help line opens with the command word, which the sentence
        # already names.
        summary = re.sub(r"^%s\b[\s:-]*" % re.escape(word.group(1)), "",
                         summary).strip()
        if not nd_cfuncs.is_prose([summary]):
            continue
        return word.group(1), summary.rstrip(".")
    return None


def bit_label(expr, reg):
    """The name of the bit an expression picks out of a register."""
    names = re.findall(r"[A-Z][A-Z0-9_]{2,}", expr)
    if not names:
        return None
    name = names[-1]
    for suffix in ("_POS", "_MASK", "_OFST", "_OFFSET"):
        if name.endswith(suffix):
            name = name[:-len(suffix)]
            break
    if name.startswith(reg + "_"):
        name = name[len(reg) + 1:]
    return name or None


def register_access(body_code):
    """A sentence for a body that only touches one register.

    The low-level drivers are full of these. Saying "wrapper around RD_REG32"
    of each in turn tells the reader nothing, where the register and the bit
    are the whole content of the function.
    """
    statements = [re.sub(r"\s+", " ", s.strip())
                  for s in body_code.split(";") if s.strip()]
    body = [s for s in statements if not SCRATCH.match(s)]
    if not body or len(body) > 4:
        return None

    touches = [(s, m) for s, m in ((s, REG_REF.search(s)) for s in body) if m]
    if not touches:
        if len(body) == 1:
            bit = BIT_CALL.match(body[0])
            if bit:
                label = bit_label(bit.group(3), bit.group(2))
                return "%s the %s bit in the %s register." % (
                    "Sets" if bit.group(1) == "ENABLE" else "Clears",
                    label, bit.group(2)) if label else None
        return None

    written = [m.group(2) for _, m in touches if m.group(1) == "WR"]
    read = [m.group(2) for _, m in touches if m.group(1) == "RD"]

    # Read, modify, write the same register: the bit is what to report.
    if len(read) == 1 and read[0] in written:
        reg = read[0]
        changes = [s for s in body if not REG_REF.search(s)]
        sets = [bit_label(m.group(1), reg) for m in
                (SET_BIT.match(s) for s in changes) if m]
        clears = [bit_label(m.group(1), reg) for m in
                  (CLEAR_BIT.match(s) for s in changes) if m]
        others = sorted({r for r in written if r != reg})
        tail = (", and writes %s" % join(others)) if others else ""
        if sets and not clears and all(sets):
            return "Sets the %s bit in the %s register%s." % (
                join(sets), reg, tail)
        if clears and not sets and all(clears):
            return "Clears the %s bit in the %s register%s." % (
                join(clears), reg, tail)
        return ("Reads the %s register, updates it and writes it back%s."
                % (reg, tail))

    if len(touches) == 1:
        statement, ref = touches[0]
        if ref.group(1) == "WR":
            return "Writes the %s register." % ref.group(2)
        if statement.startswith("(void)"):
            return ("Reads the %s register, discarding the value."
                    % ref.group(2))
        return "Reads the %s register." % ref.group(2)
    return None


def describe(name, path, text, code, offset, documented, spans):
    """(summary, detail, [(param, text)], returns) or None."""
    body, body_code = body_of(text, code, offset)
    if body is None:
        return None
    params = parameters_of(code, offset)
    calls = top_level_calls(body_code, name)
    subject = sample_subject(path)

    if name in HOOKS:
        summary, detail, canned, ret = HOOKS[name]
        listed = canned
        if listed is None:
            listed = [(p, "Not used.") for _, p in params if unused(p, body_code)]
            listed = listed or None
        return summary, detail, listed, ret

    own_words = declaration_comment(name, text, code, spans, offset)
    if own_words and name not in ("main", "samples_main", "prvSetupHardware"):
        listed = [(p, "Not used.") for _, p in params if unused(p, body_code)]
        steps = [link(c, documented) for c in calls if c in documented]
        detail = ("It calls %s." % join(steps)) if steps else ""
        return own_words, detail, listed or None, None

    if name == "main":
        rtype = return_type(code, offset, name)
        returns_something = "*" in rtype or not re.search(r"\bvoid\b", rtype)
        steps = [link(c, documented) for c in calls]
        summary = "Entry point for the %s." % subject
        detail = ""
        if steps:
            detail = ("Everything the sample does starts here: it calls %s."
                      % join(steps))
        if never_returns(body_code):
            detail += (" The loop at the end never falls through, so this is "
                       "the last frame on the stack for the life of the "
                       "system.")
            ret = ("Never returns. The scheduler owns the processor from here "
                   "on, and the sample runs until the device is reset.")
        else:
            lit = returned_literal(body_code)
            ret = ("%s, which the sample is not expected to reach." % lit
                   if lit else None)
        return summary, detail.strip(), None, ret if returns_something else None

    if name == "samples_main":
        # The task name is a string literal, so read it from the real text.
        created = re.findall(r"xTaskCreate\w*\s*\(\s*(\w+)\s*,\s*\"([^\"]*)\"", body)
        starts = "vTaskStartScheduler" in body_code
        parts = []
        if len(created) == 1:
            parts.append("creates the \"%s\" task, which runs %s"
                         % (created[0][1], link(created[0][0], documented)))
        elif created:
            parts.append("creates the %s tasks, which run %s"
                         % (join(['"%s"' % t for _, t in created]),
                            join([link(f, documented) for f, _ in created])))
        if starts:
            parts.append("then starts the FreeRTOS scheduler")
        if not parts:
            return None
        summary = "Sets the %s running: %s." % (subject, "; ".join(parts))
        detail = ""
        if created and starts and re.search(r"if\s*\(", body_code):
            detail = ("The scheduler is only started if the task was created, "
                      "so a sample that runs out of heap stops here instead of "
                      "running with nothing to do.")
        return summary, detail, None, None

    if name == "run_samples":
        ran = [link(c, documented) for c in calls
               if not c.startswith(("vTask", "xTask"))]
        tail = ("It then suspends itself, leaving the scheduler to run the "
                "remaining tasks."
                if "vTaskSuspend" in calls else "")
        if not ran:
            return None
        listed = [(p, "Not used.") for _, p in params if unused(p, body_code)]
        return ("Task that carries out the %s: runs %s." % (subject, join(ran)),
                tail, listed or None, None)

    if name == "prvSetupHardware":
        steps = [link(c, documented) for c in calls]
        if not steps:
            return None
        return ("Brings the platform up before any task runs.",
                "Calls %s, in that order." % join(steps), None, None)

    # Anything else only gets a description when its body is short enough that
    # naming the calls really does describe it.
    command = cli_command(name, path)
    if command:
        word, help_line = command
        return ("Implements the %s command of the CLI sample: %s."
                % (word, help_line), "", None, None)

    touched = register_access(body_code)
    if touched:
        listed = [(p, "Not used.") for _, p in params if unused(p, body_code)]
        return touched, "", listed or None, None

    if body_code.count(";") > 12:
        return None
    if len(calls) <= 1:
        # A short body whose real work sits inside an if, such as the ISR-safe
        # variants the OSAL wrappers pick between.
        nested = top_level_calls(body_code, name, any_depth=True)
        if 2 <= len(nested) <= 4:
            calls = nested
    if not calls or len(calls) > 4:
        return None
    steps = [link(c, documented) for c in calls]
    listed = [(p, "Not used.") for _, p in params if unused(p, body_code)]
    detail = ""
    if never_returns(body_code):
        detail = "It does not return; the body loops for the life of the task."
    # A body that is one statement and one call is a wrapper, and reads better
    # said that way.
    if len(steps) == 1 and body_code.count(";") == 1:
        return ("Wrapper around %s." % steps[0], detail, listed or None, None)
    # Calls in opposite arms of an #if are alternatives, not a sequence.
    switch = re.search(r"#\s*if(?:def)?\s+(?:defined\s*\(\s*)?(\w+)", body)
    if switch and len(steps) == 2 and re.search(r"#\s*else", body):
        return ("Calls %s or %s, depending on %s."
                % (steps[0], steps[1], switch.group(1)),
                detail, listed or None, None)
    return ("Calls %s." % join(steps), detail, listed or None, None)


def render(indent, summary, detail, params, returns):
    # Text lifted from the source can carry a comment terminator, which would
    # end the topic early and drop the rest of the block into the code.
    summary, detail, returns = [t and t.replace("*/", "").strip()
                                for t in (summary, detail, returns)]
    params = [(p, v.replace("*/", "").strip()) for p, v in params or []]
    out = [""]
    step = indent + "    "
    for para in [summary, detail]:
        if not para:
            continue
        out += textwrap.wrap(para, width=78, initial_indent=indent,
                             subsequent_indent=indent)
        out.append("")
    if params:
        out.append(indent + "Parameters:")
        for pname, ptext in params:
            out += textwrap.wrap("%s - %s" % (pname, ptext), width=78,
                                 initial_indent=step,
                                 subsequent_indent=step + "    ")
        out.append("")
    if returns:
        out.append(indent + "Returns:")
        out += textwrap.wrap(returns, width=78, initial_indent=step,
                             subsequent_indent=step)
        out.append("")
    return out[:-1]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--only", help="only touch paths containing this string")
    ap.add_argument("--refresh", action="store_true",
                    help="redo the topics that only name a register macro")
    args = ap.parse_args()

    documented = set()
    for path in dox2nd.gather(args.paths):
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        for m in TOPIC.finditer(text):
            if m.group(2) in ("Function", "Callback", "Macro"):
                documented.add(m.group(3))

    filled = left = touched = 0
    for path in dox2nd.gather(args.paths):
        if args.only and args.only not in path:
            continue
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        original = text
        if args.refresh:
            text = WEAK_BODY.sub(r"\1/*\n\2\1*/\n", text)
        code = nd_cfuncs.masked(text, dox2nd.find_comment_spans(text))
        offsets = dict(nd_cfuncs.definitions(text)[0])
        spans = dox2nd.find_comment_spans(text)
        lines = text.split("\n")

        edits = []
        ms = list(TOPIC.finditer(text))
        for i, m in enumerate(ms):
            if m.group(2) != "Function":
                continue
            limit = ms[i + 1].start() if i + 1 < len(ms) else len(text)
            span = next(((s, e) for _, s, e in spans if s <= m.start() < e), None)
            stop = min(span[1] if span else limit, limit)
            if re.sub(r"\*/\s*$", "", text[m.end():stop]).strip():
                continue
            name = m.group(3)
            if name not in offsets:
                left += 1
                continue
            described = describe(name, path, text, code, offsets[name],
                                 documented, spans)
            if not described:
                left += 1
                continue
            summary, detail, params, returns = described
            block = render(m.group(1), summary, detail, params, returns)
            edits.append((text.count("\n", 0, m.start()), block, name, summary))

        if not edits and text == original:
            continue
        for lineno, block, _, _ in reversed(edits):
            lines[lineno + 1:lineno + 1] = block
        new = "\n".join(lines)
        if dox2nd.strip_comments(new) != dox2nd.strip_comments(original):
            print("REFUSED (code would change): %s" % path, file=sys.stderr)
            continue
        filled += len(edits)
        touched += 1
        if args.apply:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)
        if args.dry_run:
            for _, block, name, _ in edits:
                print("--- %s : %s" % (path, name))
                print("\n".join(block))

    print("\nfunction topics described : %d  in %d files" % (filled, touched))
    print("left empty on purpose     : %d" % left)
    return 0


if __name__ == "__main__":
    sys.exit(main())
