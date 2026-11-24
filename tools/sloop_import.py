#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""(Vallée) Import SLOOP's engines as SPAMPY uses them into firmware/src/sloop/.

    python3 tools/sloop_import.py <SPAMPY checkout>

SPAMPY (github.com/Spampy-Droid/SPAMPY-0.0-r949, GPL-3.0) on SLOOP (github.com/isod89/sloop-fm1, GPL-3.0) on
Felucca. Only the descriptions are rewritten, never the sound code: a parameter's initializer
{label, fmt, min, max, def, names, unit} becomes Vallée's {label, names, min, max, def, fmt, unit} (unit "ct" ->
U_CT ...), an engine's positional initializer becomes designated, .macro (HOME's knobs) becomes .knob, .fil_page and
.color (SPAMPY's own UI) are left out. Every file
keeps its own copyright lines; a header names where it came from.
"""
import re
import sys
from pathlib import Path

FILES = ["eng_analog.c", "eng_digital.c", "eng_phase.c", "eng_lofi.c", "eng_formant.c", "eng_trio.c",
         "preset_trim.h", "eng_prophet.c", "eng_cz.c", "eng_sid.c", "eng_obervave.c", "eng_wavetable.c", "eng_drone.c",
         "eng_drawbar.c", "prophet_patch.c", "obv_native_core.h", "obv_native_dsp.h", "obv_native_params.h",
         "obv_native_tables.h", "obv_selected.h", "cz_patch.h", "cz_selected.h", "p5_selected.h", "sid_tables.h",
         "x0x/fastmath.h"]
UNITS = {'0': '0', '"ct"': 'U_CT', '"st"': 'U_ST', '"Hz"': 'U_HZ', '"ms"': 'U_MS'}
ENGINE_FIELDS = ["name", "page_title", "edit", "presets", "npresets", "fil_page", "note_on", "render", "color",
                 "knob", "poly", "amp", "desc", "block", "ownenv", "done"]          # SPAMPY's engine_t order

PD = re.compile(r'\{\s*("[^"]*")\s*,\s*(F_[A-Z0-9_]+)\s*,\s*([^,{}]+?)\s*,\s*([^,{}]+?)\s*,\s*([^,{}]+?)\s*,'
                r'\s*([^,{}]+?)\s*,\s*([^,{}]+?)\s*\}')


def pdesc(m):
    unit = UNITS.get(m.group(7).strip())
    if unit is None:
        raise SystemExit(f"sloop_import: unknown unit {m.group(7)}")
    return f"{{{m.group(1)}, {m.group(6)}, {m.group(3)}, {m.group(4)}, {m.group(5)}, {m.group(2)}, {unit}}}"


def split_top(body):
    """the top-level comma-separated items of an initializer body"""
    items, depth, cur, q = [], 0, "", False
    for ch in body:
        if ch == '"':
            q = not q
        if not q:
            if ch in "{(":
                depth += 1
            elif ch in "})":
                depth -= 1
            elif ch == "," and depth == 0:
                items.append(cur.strip())
                cur = ""
                continue
        cur += ch
    if cur.strip():
        items.append(cur.strip())
    return items


def engine(m):
    head, body = m.group(1), m.group(2)
    items = split_top(re.sub(r"/\*.*?\*/", "", body, flags=re.S))
    if items and all(i.startswith(".") for i in items):
        out = [re.sub(r"^\.macro\b", ".knob", i) for i in items]
    else:
        out = [f".{ENGINE_FIELDS[k]} = {v}" for k, v in enumerate(items)]
    # SPAMPY's FILTER page and accent colour are its own UI's: Vallée has neither field
    out = [i for i in out if not re.match(r"\.(fil_page|color)\b", i)]
    return head + "{\n    " + ",\n    ".join(out) + ",\n};"


ARP_ON = []                                      # presets whose arp SPAMPY switches on (Vallée: the track's)


def xp_pairs(args, where):
    """XP(P_A + 1, va, P_B + 1, vb) / .x = {...} -> .more = {{P_A, va}, {P_B, vb}}"""
    a = [x.strip() for x in split_top(args) if x.strip()]
    if len(a) % 2 or len(a) > 8:
        raise SystemExit(f"sloop_import: {where}: extras {args}")
    pairs = []
    for i in range(0, len(a), 2):
        pid = re.sub(r"\s*\+\s*1$", "", a[i])
        if pid == a[i]:
            raise SystemExit(f"sloop_import: {where}: extra id without + 1: {a[i]}")
        pairs.append(f"{{{pid}, {a[i + 1]}}}")
    return ".more = {" + ", ".join(pairs) + "}"


def presets(t, f):
    def xp(m):
        return xp_pairs(m.group(1), f)
    t = re.sub(r"\bXP\(((?:[^()]|\([^()]*\))*)\)", xp, t)
    t = re.sub(r"\.x\s*=\s*\{([^{}]*)\}", xp, t)

    def arp(m):
        if m.group(1).split(",")[0].strip() not in ("0",):
            name = re.findall(r'\{\s*"([^"]*)"', t[max(0, m.start() - 300):m.start()])
            ARP_ON.append(f"{f}: {name[-1] if name else '?'} ARP({m.group(1)})")
        return ""
    t = re.sub(r",?\s*\bARP\(([^()]*)\)", arp, t)
    t = re.sub(r",?\s*\bPAT\(([^()]*)\)", "", t)
    return t


# small fixes where Vallée already has the same thing (asserted to match exactly once)
PATCHES = {
    # (RAM: Vallée's .data/.bss is fuller than SPAMPY's) CZ's per-part patch copy in the POOL (zeroed at boot as .bss)
    "eng_cz.c": [(r"static uint8_t cz_patch\[NPART\]\[144\];",
                  'static uint8_t cz_patch[NPART][144] __attribute__((section(".pool")));')],
    # OBERVAVE's per-card tables have 8 entries (the original's 8 voice cards); SPAMPY reads them by its voice number
    # (up to 9 there, 15 in Vallée): past the tables. The 8 cards again for voices 9..16 (as many as the original had)
    "eng_obervave.c": [(r"pan=\(OB_PAN_CARD\[card\]", "pan=(OB_PAN_CARD[card%OB_NV]")],
    "obv_native_core.h": [(r"OB_VINT_ENV\[OB_card\]", "OB_VINT_ENV[OB_card % OB_NV]"),
                          (r"OB_VINT_TUNE8\[OB_card\]", "OB_VINT_TUNE8[OB_card % OB_NV]"),
                          (r"OB_VINT_CUT8\[OB_card\]", "OB_VINT_CUT8[OB_card % OB_NV]")],
    "eng_prophet.c": [(r"static uint32_t cents_inc\(int32_t pitch16,int32_t ct,int32_t fine\)\n\{\n.*?\n\}\n",
                       "/* (cents_inc: Vallée's dsp.c, the same) */\n")],
}


def main(src):
    src = Path(src) / "firmware" / "src"
    dst = Path(__file__).resolve().parents[1] / "firmware" / "src" / "sloop"
    for f in FILES:
        t = (src / f).read_text()
        t = PD.sub(pdesc, t)
        t = presets(t, f)
        for pat, rep in PATCHES.get(f, []):
            t, n = re.subn(pat, rep, t, flags=re.S)
            if n != 1:
                raise SystemExit(f"sloop_import: {f}: patch {pat[:40]} matched {n} times")
        t = re.sub(r"(static const engine_t ENG_\w+\s*=\s*)\{(.*?)\n?\};", engine, t, flags=re.S)
        out = dst / f
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(f"/* (Vallée) from SPAMPY r949 / SLOOP 2.7, firmware/src/{f}: tools/sloop_import.py (the "
                       "descriptions in Vallée's layout; the sound code as it is) */\n" + t)
    print(f"sloop_import: {len(FILES)} files into {dst}")
    for a in ARP_ON:
        print("  arp on in SPAMPY (left to the track in Vallée):", a)


if __name__ == "__main__":
    main(sys.argv[1])
