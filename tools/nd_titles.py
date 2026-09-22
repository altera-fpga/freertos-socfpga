#!/usr/bin/env python3
"""Give every Natural Docs `File:` topic a readable title.

Natural Docs uses the File topic as the page heading, so a topic named after
the file produces headings like "socfpga_i2c_ll.h". This derives a title from
the component and the role the file plays: "I2C Low-Level Driver".

    python3 tools/nd_titles.py --dry-run  drivers samples ...
    python3 tools/nd_titles.py --apply    drivers samples ...

The sidebar keeps real filenames; see tools/nd_menu_filenames.py.
"""

import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dox2nd

FILE_TOPIC = re.compile(r"(?m)^([ \t]*)File:[ \t]+(\S.*?)[ \t]*$")

# Words that must not be title-cased into "I2c" or "Usb".
ACRONYMS = {
    "i2c": "I2C", "i3c": "I3C", "spi": "SPI", "qspi": "QSPI", "xspi": "XSPI",
    "uart": "UART", "gpio": "GPIO", "dma": "DMA", "ecc": "ECC", "fcs": "FCS",
    "gic": "GIC", "seu": "SEU", "smmu": "SMMU", "mmu": "MMU", "ccu": "CCU",
    "iossm": "IOSSM", "rsu": "RSU", "ros": "ROS", "sdm": "SDM", "mmc": "MMC",
    "usb": "USB", "otg": "OTG", "fpga": "FPGA", "hps": "HPS", "cli": "CLI",
    "crc": "CRC", "xgmac": "XGMAC", "eth": "Ethernet", "fat": "FAT",
    "osal": "OSAL", "api": "API", "noc": "NoC", "aes": "AES", "sip": "SIP",
    "psci": "PSCI", "srk": "SRK", "ecdsa": "ECDSA", "uuid": "UUID",
    "id": "ID", "io": "IO", "ip": "IP", "pr": "PR", "sd": "SD", "tx": "TX",
    "rx": "RX", "led": "LED", "ddr": "DDR", "ocram": "OCRAM", "sof": "SOF",
    "rbf": "RBF", "jtag": "JTAG", "adc": "ADC", "pll": "PLL", "isr": "ISR",
    "irq": "IRQ", "trb": "TRB", "xhci": "xHCI", "dwc3": "DWC3", "hcd": "HCD",
    "ehci": "EHCI", "ohci": "OHCI", "tcp": "TCP", "udp": "UDP", "ipv4": "IPv4",
    "phy": "PHY", "icmp": "ICMP", "plat": "Platform", "cfg": "Configuration",
    "mgr": "Manager", "intf": "Interface", "desc": "Descriptor",
    "crc32": "CRC32", "sdcard": "SD Card", "def": "Definitions",
    "socfpga": "SOCFPGA", "mmio": "MMIO",
}

# Directory or token spellings that are clearer written out.
COMPONENTS = {
    "clk_mngr": "Clock Manager", "reset_mngr": "Reset Manager",
    "reboot_mngr": "Reboot Manager", "sys_mngr": "System Manager",
    "sdm_mailbox": "SDM Mailbox", "fpga_manager": "FPGA Manager",
    "usb_otg": "USB OTG", "usb3": "USB 3.0", "sdmmc": "SD/MMC",
    "wdt": "Watchdog Timer", "multithread_samples": "Multithreading",
    "fatfs": "FatFs", "ethernet": "Ethernet", "console": "Console",
    "bridge": "Bridge", "timer": "Timer", "common": "Common",
    "librsu": "RSU Library", "libfcs": "FCS Library", "cli_app": "CLI",
    "freertos": "FreeRTOS", "portable": "Portable Layer",
    "mbox": "Mailbox", "mngr": "Manager", "clk": "Clock", "rst": "Reset",
    "freertosplus": "FreeRTOS+", "main_full": "Full Demo",
    "main_freertosplus_basic": "FreeRTOS+ Basic", "utils": "Utilities",
    # Layout directories, spelled out for the sidebar headings.
    "os": "OS", "hal": "HAL", "src": "Source", "lib": "Library",
    "unit-test": "Unit Tests", "priv_include": "Private Include",
    "platform_include": "Platform Include", "linux_example": "Linux Example",
}

# Filename markers that describe what the file is, longest first.
ROLE_SUFFIX = [
    ("_reg", "Registers"),
    ("_ll", "Low-Level Driver"),
    ("_defines", "Definitions"),
    ("_config", "Configuration"),
    ("_utils", "Utilities"),
    ("_helper", "Helpers"),
]

NOISE = {"socfpga", "arm", "altera", "intel", "lib", "priv"}

# Directories that describe layout rather than a component.
STRUCTURAL_DIRS = {
    "include", "src", "lib", "hal", "common", "apps", "samples", "drivers",
    "platform", "platform_include", "dependency", "minimal", "portable",
    "freertos", "libfcs", "librsu", "demo", "socfpga", "utils", "host",
    "priv_include", "linux", "linux-aarch64",
}


# Directories that never distinguish one copy of a header from another. Port
# names such as host, linux or freertos are deliberately absent: they are
# exactly what tells the duplicated OSAL headers apart.
LAYOUT_DIRS = {
    "include", "src", "lib", "hal", "common", "dependency", "platform_include",
    "priv_include", "utils", "toolchain", "os", "app", "apps", "arch", "demo",
    "libfcs", "librsu", "fcs", "rsu", "osal", "drivers", "samples", "minimal",
}


def variant_of(path):
    """The port or board directory that tells near-identical files apart."""
    dirs = os.path.dirname(path).replace("\\", "/").split("/")
    for name in reversed(dirs):
        if name and name.lower() not in LAYOUT_DIRS and name.lower() not in NOISE:
            return pretty(name)
    return ""


# Names that must survive CamelCase splitting in one piece.
COMPOUNDS = re.compile(
    r"(?i)(freertos\+tcp|freertos\+|freertos|socfpga|lwhps2fpga|hps2fpga|"
    r"aarch64|xhci|dwc3|fatfs|ipv4|ipv6)"
)


def split_words(token):
    """Split snake_case and CamelCase alike: EventGroupsDemo -> event groups demo."""
    out = []
    for chunk in re.split(r"[_\-]+", token):
        if not chunk:
            continue
        if chunk.lower() in ACRONYMS or chunk.isupper():
            out.append(chunk)
            continue
        for piece in COMPOUNDS.split(chunk):
            if not piece:
                continue
            if COMPOUNDS.fullmatch(piece):
                out.append(piece)
            else:
                out.extend(re.findall(r"[A-Z]+(?![a-z])|[A-Z][a-z0-9]*|[a-z0-9]+", piece))
    return [w for w in out if w]


def pretty(token):
    token = token.strip("_-")
    if not token:
        return ""
    low = token.lower()
    if low in COMPONENTS:
        return COMPONENTS[low]
    if low in ACRONYMS:
        return ACRONYMS[low]
    return " ".join(
        COMPONENTS.get(w.lower()) or ACRONYMS.get(w.lower()) or w.capitalize()
        for w in split_words(token)
    )


def area_of(path):
    parts = path.replace("\\", "/").split("/")
    if parts[0] == "drivers":
        return "driver"
    if parts[0] == "samples":
        return "sample"
    if parts[0] in ("fcs", "rsu"):
        return "library"
    if parts[0] == "osal":
        return "osal"
    return "demo"


def component_of(path):
    """The hardware block or subsystem the file belongs to."""
    parts = path.replace("\\", "/").split("/")
    directory = parts[-2] if len(parts) > 1 else ""
    stem = os.path.splitext(parts[-1])[0]
    tokens = [t for t in split_words(stem) if t.lower() not in NOISE]

    if directory and directory.lower() not in STRUCTURAL_DIRS:
        comp = pretty(directory)
        dir_words = {w.lower() for w in split_words(directory)}
        extra = [t for t in tokens if t.lower() not in dir_words]
        return comp, extra

    if tokens:
        return pretty(tokens[0]), tokens[1:]
    return pretty(stem), []


def dedupe(title):
    """Drop repeated words such as "Timer Timer Driver"."""
    words, seen = [], set()
    for word in title.split():
        key = word.lower()
        if key in seen:
            continue
        seen.add(key)
        words.append(word)
    return " ".join(words)


def title_for(path):
    stem = os.path.splitext(os.path.basename(path))[0]
    low = stem.lower()
    area = area_of(path)

    # Command-line front-ends are named after the component they drive.
    if low.startswith("cli_"):
        rest = "_".join(split_words(stem)[1:])
        if rest.lower() in ("utils", "util"):
            return "CLI Utilities"
        if rest.lower() in ("app", "application"):
            return "CLI Application"
        return dedupe("%s CLI Commands" % pretty(rest))

    # Demo and third-party sources read best as just their name.
    if area == "demo":
        return dedupe(" ".join(pretty(t) for t in split_words(stem)
                               if t.lower() not in NOISE))

    comp, extra = component_of(path)

    # Every sample ships a main.c; name it for what it is.
    if low == "main":
        return dedupe("%s Sample Entry Point" % comp) if area == "sample" \
            else dedupe("%s Entry Point" % comp)

    role = None
    for marker, name in ROLE_SUFFIX:
        if low.endswith(marker):
            role = name
            extra = [t for t in extra if t.lower() != marker.strip("_")]
            break

    if role is None:
        if area == "sample" or "sample" in low:
            role = "Sample"
        else:
            role = {"driver": "Driver", "library": "Library", "osal": ""}[area]

    extra = [t for t in extra if t.lower() not in ("sample", "samples")]
    qualifier = " ".join(pretty(t) for t in extra if pretty(t))

    title = dedupe(" ".join(p for p in (comp, qualifier) if p))
    # Do not append a role the name already states.
    if role and role.split()[0].lower() not in title.lower():
        title = "%s %s" % (title, role)
    return title.strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    targets = []
    for path in dox2nd.gather(args.paths):
        text = open(path, encoding="utf-8", errors="surrogateescape").read()
        m = FILE_TOPIC.search(text)
        if m:
            targets.append((path, text, m))

    # Headers describe the interface, so they claim the plain title and the
    # matching source file becomes "... Implementation".
    order = sorted(targets, key=lambda t: (0 if t[0].endswith(".h") else 1, t[0]))

    chosen, used = {}, {}
    for path, _, _ in order:
        base = title_for(path)
        title = base
        if title in used:
            variant = variant_of(path)
            candidates = []
            if path.endswith((".c", ".S", ".s")):
                candidates.append(base + " Implementation")
            if variant and variant.lower() not in base.lower():
                candidates.append(dedupe("%s %s" % (variant, base)))
            candidates.append(base + " Definitions")
            for candidate in candidates:
                if candidate not in used:
                    title = candidate
                    break
            else:
                n = 2
                while "%s (%d)" % (base, n) in used:
                    n += 1
                title = "%s (%d)" % (base, n)
        used[title] = path
        chosen[path] = title

    changed = 0
    for path, text, m in targets:
        title = chosen[path]
        if m.group(2) == title:
            continue
        new = text[:m.start()] + "%sFile: %s" % (m.group(1), title) + text[m.end():]
        changed += 1
        if args.apply:
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
                fh.write(new)
        if args.dry_run:
            print("%-62s %-34s -> %s" % (path, m.group(2), title))

    print("\nfiles with a File topic : %d" % len(targets))
    print("titles rewritten        : %d" % changed)
    print("distinct titles         : %d" % len(set(chosen.values())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
