# Natural Docs Guide

The documentation framework used by this repository: what Natural Docs is, the
hierarchy it builds, the comment style every source file must follow, how the
generator is configured, and how to check and build the output.

---

## 1. What Natural Docs Is

Natural Docs is a documentation generator. It scans source files, finds specially
formatted comments, and produces a browsable HTML reference site with a
navigation menu, a summary for each file, and cross-linked identifiers.

Its defining trait is that comments are written as readable English rather than
as tag soup. Compare the two conventions for the same function:

Doxygen:

```c
/**
 * @brief Opens a GPIO instance.
 * @param[in]  pin     GPIO pin identifier.
 * @param[out] handle  Receives the opened handle.
 * @return GPIO_OK on success.
 */
```

Natural Docs:

```c
/* Function: gpio_open
 *
 * Opens a GPIO instance and prepares it for subsequent operations.
 *
 * Parameters:
 *
 *    pin - GPIO pin identifier.
 *    handle - Output storage that receives the opened or initialized handle.
 *
 * Returns:
 *
 *    GPIO_OK on success or a GPIO status/error code on failure.
 */
```

There are no `@` tags. A topic line names the thing being documented, and plain
headings introduce the sections. The comment stays legible in the source, which
is the point: most people read the header, not the generated site.

**The two systems cannot be mixed.** Natural Docs does not recognize `@brief` or
`@param`, so a file written in Doxygen style contributes nothing to the output.

---

## 2. The Hierarchy

Natural Docs builds a five-level tree. Understanding it is what makes the
difference between comments that merely compile and comments that produce a
well-organized site.

### Level 1 — Project and source folders

The source directories passed to the generator, listed in `SOURCE_DIRS` in
`tools/nd_build.sh`, are what the navigation menu is built from. Natural Docs
generates the menu itself and writes it to `Menu.txt`, and two of its choices
are unhelpful:

- **Branches are sorted alphabetically**, not in `SOURCE_DIRS` order.
- **Small directories are flattened.** A directory only earns its own named
  branch once it holds enough files, so a single-file component such as
  `drivers/ecc/` used to appear as a loose entry sitting beside the groups
  rather than under an "ECC" heading.

Every file belongs in the group for the directory it lives in, so the menu is
not left to Natural Docs. `tools/nd_menu.py` rewrites `Menu.txt` after each
generation into a plain mirror of the source tree: one group per directory,
nested exactly as on disk, nothing loose. It also labels each entry with the
filename, as described in 3.1. `tools/nd_build.sh` runs it and regenerates when
the menu changed, so `make docs` is all you need.

That means `Menu.txt` is derived, not hand-maintained: to move something in the
sidebar, move the file. The project title, subtitle and the index section at
the bottom of `Menu.txt` are preserved and can be edited by hand.

### Level 2 — File

Every source file must open with a `File:` topic. It supplies the page title and
the description shown at the top, and it is the file's entry in the menu — though
the menu label is pinned to the filename, see 3.1.

Despite the name, it is not a container. The File type is declared
`Scope: Always global`, meaning it is global itself and does not change scope for
anything after it.

### Level 3 — Section and Group

These organize the inside of a file.

- `Section:` introduces a major division and is declared `Scope: End`, meaning it
  **resets scope back to the top level** of the file for everything after it. It
  does not capture the topics below it; it releases them.
- `Group:` introduces a heading in the file summary and has no `Scope:` setting
  at all, so it **does not affect scope**. It is purely a visual heading. Use it when section need further division though rarely the case.

Neither one takes ownership of what follows. The only topics that do that are
scope-openers like `Class:` — and `Struct:`, until this project remapped it.

Use `Group:` for organization inside a file, such as separating lifecycle
functions from transfer functions, or heading the prose blocks a sample carries
above its code.

Avoid `Section:` unless you specifically need to close a scope something else
opened. Because it resets scope, any topics that follow the last `Section:` in a
file form their own trailing division, and Natural Docs gives that division a
summary table of its own. The page then opens with a summary of everything and
repeats part of it further down, which is why this project uses `Group:`
throughout. `Group:` is safe here: it never resets scope, so a file keeps one
summary no matter how many headings it has.

Scope never escapes the file. Natural Docs generates one page per source file,
so "top level" means the top of that file's page.

Give every `Group:` a one-line description. Natural Docs prints a group as a
row in the file summary with a description cell beside it, and a heading with
no text under it leaves that cell blank, which reads like something went
missing:

```c
/*
    Group: Structures

    Structures and callback signatures for the DMA driver.
*/
```

Describe what the group actually holds rather than repeating its title: a
heading called "Macros" that contains one callback typedef should say so. A
heading with no topics under it at all should be deleted, not described.

Two things stop the description from reaching the summary. A group whose body
opens with a bullet list or a code block has no leading paragraph for Natural
Docs to lift, so write a sentence before the list. And a group that only
Natural Docs knows about - the "Functions" and "Constants" headings it
generates for a file that declares no groups of its own - cannot be described
at all; give the file real `Group:` topics if you want text there.

`tools/nd_groups.py` fills in the blank ones and removes the empty headings.

### Level 4 — Members

The individual code elements. These are the keywords this repository uses. All
are recognized by Natural Docs out of the box, though `Struct:` needs a
configuration change to behave sensibly in C:

| Keyword | Documents | Scope behaviour |
| --- | --- | --- |
| `Function:` | Functions, and function-pointer typedefs via `Callback:` | none |
| `Type:` | `typedef` aliases | none |
| `Struct:` | `struct` definitions | none (see below) |
| `Enum:` | `enum` definitions | none |
| `Constant:` | `#define` values and constants | none |
| `Macro:` | Function-like macros | none |
| `Callback:` | Function-pointer typedefs (alias of `Function`) | none |

Two behaviours are worth knowing. `Callback:` is not a type of its own; it is an
alias keyword for `Function`, so callbacks appear under a **Functions** heading
in the output rather than a "Callbacks" one. And the `Values:` block under an
enum is an ordinary definition list, which Natural Docs renders as a two-column
table; the C/C++ language definition sets `Enum Values: Under parent`, so members
are referenced by their own name rather than qualified by the enum.

`Struct:` needs explanation. By default it is a keyword of the **Class** type,
which starts a scope — meaning every function declared after a struct gets
nested underneath it, as though the struct owned the API. That is wrong for C.
This project reassigns the keyword to `Type` in `docs/naturaldocs/project/Topics.txt`,
so structs document normally without opening a scope. If you copy these
conventions elsewhere, copy that override too.

### Heading grouping

Natural Docs merges a consecutive run of compatible topics under a single
heading, and names the heading after the mixture — a run of types and enums
becomes one **Types and Enumerations** block rather than alternating headings.
Types, Enumerations, Constants, Macros and Variables all group with each other;
Functions group only with Properties. Nothing you write controls this. It is
purely a function of ordering, which is why the recommended layout for a header
is the file topic, then all types and enums together, then all functions.

### Level 5 — Sections inside a topic body

Within one comment block, in this fixed order: the description prose first, then
`Parameters:`, `Returns:`, `Values:` (enums only), `Notes:`, and `See Also:`.

### Cross-references

Wrapping an identifier in angle brackets turns it into a hyperlink in the
generated HTML:

```c
 *    per - The target peripheral identifier from <reset_periphrl_t>.
```

Use this whenever you mention another documented type or function. It costs
nothing and makes the output genuinely navigable.

---

## 3. The Comment Style

### 3.1 File header

Every `.c`, `.h`, and `.S` file starts with the SPDX block, then a `File:` topic:

```c
/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 */

/* File: GPIO API
 *
 * GPIO public application-facing API wrapper for the SoCFPGA bare-metal
 * software stack.
 *
 * Layer: api
 * Component: GPIO
 */
```

The topic name becomes the page heading, so it is a readable title rather than
the filename: name the component and the role the file plays, as in `GPIO API`,
`I2C Low-Level Driver`, or `Watchdog Timer Sample`. Titles must be unique across
the project, because Natural Docs also uses them as link targets. Where a header
and its source both need a topic, the header takes the plain title and the
source becomes `... Implementation`.

The sidebar still lists real filenames. That is not automatic: Natural Docs
would otherwise reuse the heading there too, so `tools/nd_menu.py` labels each
entry with its filename and marks it `no auto-title`.

`tools/nd_titles.py` derives these titles from the path and can regenerate them
for a whole tree; hand-written titles are worth keeping when the derived one
reads poorly. A file with no topic at all is invisible to the generator and
cannot be placed in the menu, so `tools/nd_stubs.py` gives any such file a
`File:` topic and a one-line description, taken from its own header comment
where it has one.

A title on its own is not a description, and neither is a sentence that only
says the title again. `tools/nd_describe.py` fills those in from what the file
actually holds — the interface it declares, the register map or compile-time
macros it defines, or what it implements and how much of that is file-local. It
only touches a `File:` topic whose description is empty or is pure path-derived
filler, so real prose is never overwritten.

`Layer:` is one of `api`, `middleware`, `drivers`, `platform`, `lib`, `src`, or
`examples`. `Component:` is the hardware block or subsystem. Layer and component
are optional but if the implementation of code is layered then try to inject
layers.

Write a real one-line summary of what the module does. Do not restate how
Natural Docs works — that belongs in this guide, not in 290 copies across the
source tree.

### 3.2 Functions

```c
/* Function: gpio_open
 *
 * Opens a GPIO instance and prepares it for subsequent operations.
 *
 * Parameters:
 *
 *    pin - GPIO pin identifier.
 *    handle - Output storage that receives the opened handle.
 *
 * Returns:
 *
 *    GPIO_OK on success or a GPIO status/error code on failure.
 *
 * Notes:
 *
 *    Must be called before any other GPIO operation on this pin.
 */
gpio_status_t gpio_open(gpio_pin_id_t pin, gpio_handle_t *handle);
```

Rules:

1. The topic name matches the function identifier exactly.
2. The comment block sits **immediately** before the declaration, with no blank
   line and no other declaration in between. Natural Docs binds a comment to the
   code that directly follows it; a gap silently attaches your documentation to
   the wrong prototype.
3. Parameters are indented four spaces as `name - description`, one per line, in
   declaration order, and every parameter is listed.
4. `Returns:` states the success value and the failure convention. Functions
   returning `void` say `None.`
5. `Notes:` is optional. Use it for constraints a caller cannot infer from the
   signature — alignment requirements, blocking behaviour, call ordering. Omit it
   rather than filling it with boilerplate.

### 3.3 Types, structs and enums

```c
/* Type: gpio_handle_t
 *
 * Opaque GPIO handle used to preserve module state between calls.
 */
typedef struct gpio_handle_s* gpio_handle_t;

/* Struct: crc_image_desc_t
 *
 * Describes the image buffer and expected CRC for a verification request.
 */
typedef struct {
    const uint8_t *image;
    size_t actual_size;
} crc_image_desc_t;

/* Enum: gpio_direction_t
 *
 * Pin direction selector.
 *
 * Values:
 *
 *    GPIO_INPUT - Configure the pin as an input.
 *    GPIO_OUTPUT - Configure the pin as an output.
 */
typedef enum { GPIO_INPUT, GPIO_OUTPUT } gpio_direction_t;
```

For a `typedef struct { ... } name_t;` the topic name is the trailing typedef
name, not the tag. Every enum member gets a `Values:` entry.

Natural Docs prints no prototype for a `Struct:` or `Enum:` topic, so the page
never shows the fields. The description is all the reader gets, which makes it
worth naming what the type holds and where the code uses it:

```c
/*
    Struct: i3c_ll_cmd_response_t

    Command response for the I3C low-level driver, holding tid, data_len and
    status. Passed to i3c_ll_read_cmd_responses.
*/
```

Every struct, union, and enum gets a topic, including the ones private to a
single .c file. A public type reaches the reader through a function prototype
whether it is documented or not, and a private one is the state that all the
`static` functions on that page operate on — documenting those functions and
not the structure they share leaves the page half told.

`tools/nd_structs.py` writes the missing ones, reusing a description the
source already carries and otherwise building one from the fields and from the
declaration that names the type. Pass `--include-local` for the private ones.

Watch the page title when you document a type near the top of a file. Natural
Docs titles a page after its first topic, so a `File:` topic sitting below the
includes loses the title the moment something above it gets documented.
`tools/nd_filefirst.py` lifts it back to the top.

### 3.4 Callbacks

```c
/* Callback: gpio_callback_fn
 *
 * Invoked from interrupt context when a configured GPIO edge is detected.
 *
 * Parameters:
 *
 *    param - Caller-supplied context registered with gpio_set_callback.
 */
typedef void (*gpio_callback_fn)(void *param);
```

Document the actual parameters. A `Parameters:` heading followed by "see the
prototype" is an empty section and should either be filled in or removed.

### 3.5 Functions that live only in a .c file

Every function gets a topic. Where that topic goes depends on one thing: does a
header declare the function?

- **Declared in a header.** The topic belongs in the header, above the
  declaration. The definition in the `.c` file gets nothing.
- **Not declared in any header.** There is no header to document it in, so the
  topic goes in the `.c` file, above the definition. That covers every `static`
  helper and file-local entry points such as `main`.

Nothing is skipped for being internal. A `static` helper with no topic is
invisible in the generated site, which is how a driver can look like it has
four functions when it has forty.

Where a function has no description to give, write the topic anyway and leave
the body empty rather than inventing prose:

```c
/*
    Function: i2c_delete_osal_primitives
*/
static void i2c_delete_osal_primitives(i2c_handle_t hi2c)
```

`tools/nd_cfuncs.py` finds every function defined in a `.c` file that no header
declares and adds a topic for it, reusing any comment already sitting above the
definition.

Duplicate names across files are fine and are not something to work around. The
`main` in each sample is a separate topic on its own page; Natural Docs groups
them under one index entry that expands to the individual files.

### 3.6 Do not duplicate between header and source

Document each function once. If a header declares it, that is where the block
goes: repeating it above the definition produces duplicate topics and the two
copies drift apart.

The rule has a failure mode worth knowing about. A tool that skips a function
because a header declares it, run against a header nobody wrote up, leaves the
function documented in neither place. That is how the low-level drivers ended
up with prototypes that appear in no topic anywhere. When that happens, put
the topic above the definition: the body is also the only thing a description
can be derived from. `tools/nd_cfuncs.py --orphans` does this for every
prototype that has no topic in either file, and `--exclude` keeps it away from
vendored trees.

### 3.7 Keep the File topic first

The `File:` topic must be the first topic in the file, immediately after the
licence header. If any other topic precedes it, Natural Docs decides the file
has no File topic, invents a page title from the filename, and files the
earlier topics under a separate heading.

### 3.8 List macros, do not just count them

A header that is nothing but `#define`s used to render as one sentence and an
empty page, because Natural Docs never prints the expansion of a macro. A topic
per macro is not the answer either: it would bury the source under hundreds of
comment blocks and still show only names.

Use a list topic instead. The plural keywords `Macros:` and `Constants:` take a
body of `name - description` lines, and Natural Docs anchors and indexes every
entry, so each name stays linkable:

```c
/*
    Macros: Log levels

    The defines are listed in decreasing order of level.

    CRITICAL - Expands to LogAlways.
    ERROR    - Expands to LogError.
*/
```

`tools/nd_macros.py` writes these for files whose page would otherwise show a
description and nothing else. Each description comes from the macro's own
comment where it has one, and otherwise from what it expands to: a value, an
attribute, a call, or, for a name defined once per `#if` arm, the alternatives.
Register maps are left out — a list of several thousand offsets helps nobody,
and the file description already names the register families. Run it with
`--refresh` to rewrite lists an earlier run produced.

---

## 4. The Generator and Its Configuration

The version available internally is **Natural Docs 1.4**, provided by the ARC
resource `naturaldocs`:

```bash
arc shell naturaldocs -- NaturalDocs -h
```

This matters, because 1.x and 2.x are configured completely differently. Version
1.4 is a Perl program driven entirely by command-line arguments — it has no
`Project.txt`. Everything is passed as flags:

```
NaturalDocs -i [source dir]  (repeatable)
            -o HTML [output dir]
            -p [project dir]
```

`tools/nd_build.sh` wraps this, passing one `-i` per architectural layer in the
order they should appear in the menu, and `make docs` wraps that in turn.

The **project directory** given with `-p` is not something you write. Natural
Docs creates and owns it, generating three files on the first run:

| File | Purpose | Tracked? |
| --- | --- | --- |
| `Topics.txt` | Keyword and topic-type definitions | **Yes** — carries project overrides |
| `Languages.txt` | Language and file-extension mappings | **Yes** — carries project overrides |
| `Menu.txt` | The generated navigation menu | No — regenerated, holds absolute paths |

It also creates a `Data/` subdirectory caching parse results so later runs only
reprocess changed files.

`Topics.txt` and `Languages.txt` start out as pure comment templates with a
single active `Format: 1.4` line, but this project adds two overrides that both
matter:

- **`Topics.txt`** reassigns the `struct` keyword from `Class` to `Type`, which
  stops structs from swallowing the functions declared after them.
- **`Languages.txt`** adds the `.S` extension to C/C++. Neither C/C++
  (`c cpp h hpp cxx hxx`) nor Assembly (`asm`) claims it by default, so the
  assembly sources were being skipped silently.

Because those two files now hold real configuration, they are committed and
`make docs-clean` leaves them alone; only `html/`, `Data/` and `Menu.txt` are
removed.

The one file worth knowing about is `Menu.txt`. Natural Docs regenerates it but
preserves manual edits, so if you want a specific ordering or nicer group titles
in the sidebar, edit it and commit it. Without that, ordering follows the `-i`
order from `nd_build.sh`, which is already arranged from the application-facing
API down to the hardware.

**If you ever move to Natural Docs 2.x** the comment style in section 3 carries
over unchanged, but the configuration does not. Version 2 replaces all the flags
with a `Project.txt` inside a config folder, generates the menu automatically
instead of using `Menu.txt`, and is a C# program requiring Mono 4.0+ on Linux.

---

## 5. Running It

No installation is needed; the ARC resource supplies the tool and, being Perl,
it needs no Mono.
The implementation is specific to the project and may change based upon Makefile
```bash
make docs          # generate HTML into docs/naturaldocs/html
make docs-check    # verify comment conventions (no generator needed)
make docs-clean    # remove generated HTML and project data
```

Then open `docs/naturaldocs/html/index.html`.

A full build of this repository parses 585 files and produces about 720 pages in
roughly twenty seconds.

If `NaturalDocs` is already on your `PATH`, skip the ARC wrapper:

```bash
make docs ARC=
```

`make docs-check` is pure Python with no dependencies and does not need the
generator at all, which makes it the piece worth wiring into CI.

---

## 6. Adopting This in Another Repository

1. Copy `tools/nd_build.sh`, `tools/nd_titles.py`, `tools/nd_stubs.py`,
   `tools/nd_describe.py`, `tools/nd_cfuncs.py`, `tools/nd_funcdocs.py`,
   `tools/nd_macros.py`, `tools/nd_structs.py`, `tools/nd_groups.py`,
   `tools/nd_filefirst.py`, `tools/nd_menu.py`, and this guide.
2. Edit `SOURCE_DIRS` in `nd_build.sh` to match that project's layout, ordered
   as you want the menu to read.
3. Copy the `docs`, `docs-check`, and `docs-clean` targets from the `Makefile`.
4. Add `docs/naturaldocs/html/` and `docs/naturaldocs/project/` to `.gitignore`.
5. Convert files bottom-up: run `nd_stubs.py` then `nd_titles.py` to get a
   `File:` topic on everything, `nd_cfuncs.py` to document the functions that
   no header declares, `nd_funcdocs.py` to fill the bodies of those topics,
   `nd_macros.py` to list the macros of files that define nothing else,
   `nd_structs.py --include-local` to document the types, `nd_filefirst.py`
   to keep the `File:` topics on top, `nd_groups.py` to describe the group
   headings, and `nd_describe.py` last,
   so the file descriptions reflect what the pages ended up holding. Then
   write the public headers by hand, and finish with
   `nd_cfuncs.py --orphans` and `nd_funcdocs.py` again to pick up whatever
   the headers did not cover.
6. Run `make docs-check` until clean, then generate and read the HTML. The output
   exposes structural mistakes that are invisible in the source.

---

## 7. Review Checklist

- [ ] File opens with SPDX block, then a `File:` topic carrying a readable,
      project-unique title, with `Layer:` and `Component:`.
- [ ] Every public function, type, struct, enum, and callback in a header has a
      topic.
- [ ] Topic names match their identifiers exactly.
- [ ] Comment blocks sit immediately before the code they document.
- [ ] Every parameter is listed as `name - description`; `Returns:` is present.
- [ ] Enum members are documented under `Values:`.
- [ ] Related identifiers are cross-referenced with `<angle brackets>`.
- [ ] No `@brief`, `@param`, or other Doxygen tags.
- [ ] Descriptions say something specific; no filler sentences. A `File:` topic
      that only repeats its own title is not described.
- [ ] The `File:` topic is the first topic in the file.
- [ ] Headings inside a file use `Group:`, not `Section:`, so the page renders
      a single summary.
- [ ] Every function a header declares has its topic in that header, and every
      function it does not, including `static` helpers and functions defined
      inline in a header, has one where it is defined.
- [ ] No function is documented in neither place: a prototype the header never
      described gets its topic above the definition.
- [ ] A page that would show only a description lists its contents instead,
      as `Macros:` or `Constants:` entries.
- [ ] Every struct, union, and enum has a topic that names the fields it
      holds and where the code declares one, private ones included.
- [ ] Every `Group:` heading has a description, and no heading is left with
      nothing under it.
