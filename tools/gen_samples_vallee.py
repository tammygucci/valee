#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""(Vallée, the SPAMPY sound) The factory sample sets: SPAMPY's, byte for byte, and Vallée's SAX.

    python3 tools/gen_samples_vallee.py build/gen/felucca_samples.h

SPAMPY's sets (tools/spampy/gen_samples.py over assets/samples-spampy: PIANO, VIBES, FLUTE and the GM kit PERC, at
SPAMPY's set numbers 0 2 5 7) and its three SAMPLE sounds (VIBES, LOFI FLUTE, GM KIT); Vallée's SAX (tools/gen_samples.py
over assets/samples-cc0) in SPAMPY's empty set 3 (SAX's number in Vallée too), its data after SPAMPY's, its sounds SAX
and DARK SAX after SPAMPY's. Every set plays itself (SMP_SET_ORIG: no aliases). tools/gen_factory_block.py then moves the
data to the sample area.
"""
import importlib.util
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SAX_SET = 3


def load_import():
    spec = importlib.util.spec_from_file_location("si", ROOT / "tools" / "sloop_import.py")
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def section(src, start, end="};\n"):
    a = src.index(start)
    b = src.index(end, a) + len(end)
    return a, b


def zones(src):
    a, b = section(src, "static const smp_zone_t SMP_ZONES[] = {")
    return re.findall(r"\{([^{}]*)\}", src[a:b].split("{", 1)[1])


def data(src):
    m = re.search(r"static const uint8_t SMP_DATA\[(\d+)\] = \{(.*?)\};", src, re.S)
    return bytes(int(x, 0) for x in re.findall(r"0x[0-9a-fA-F]+|\d+", m.group(2)))


def main(out):
    gen = ROOT / "build" / "gen"
    gen.mkdir(parents=True, exist_ok=True)
    sp_h, va_h = gen / "samples_spampy.h", gen / "samples_vallee.h"
    subprocess.run([sys.executable, "-I", str(ROOT / "tools" / "spampy" / "gen_samples.py"), str(sp_h)], check=True)
    subprocess.run([sys.executable, str(ROOT / "tools" / "gen_samples.py"), str(va_h)], check=True)
    sp, va = sp_h.read_text(), va_h.read_text()

    # Vallée's SAX: its set's zones and their data
    vsets = re.findall(r'\{"([A-Z ]+)", (\d+), (\d+)\}', va[va.index("static const smp_set_t SMP_SETS[]"):])
    sax = [s for s in vsets if s[0] == "SAX"][0]
    vz, vd = zones(va), data(va)
    sz0, snz = int(sax[1]), int(sax[2])
    sax_z = vz[sz0:sz0 + snz]
    offs = [int(z.split(",")[0]) for z in sax_z]
    lo = min(offs)
    hi = max(int(z.split(",")[0]) + (int(z.split(",")[1]) + 1) // 2 for z in sax_z)
    sax_data = vd[lo:hi]

    spd = data(sp)
    base = len(spd) + (-len(spd) % 4)
    newdata = spd + b"\x00" * (base - len(spd)) + sax_data
    spz = zones(sp)
    nz0 = len(spz)
    moved = []
    for z in sax_z:
        f = [x.strip() for x in z.split(",")]
        f[0] = str(int(f[0]) - lo + base)
        moved.append(", ".join(f))

    out_s = sp
    # data
    m = re.search(r"static const uint8_t SMP_DATA\[(\d+)\] = \{(.*?)\};", out_s, re.S)
    rows = ",\n".join(", ".join(str(b) for b in newdata[i:i + 32]) for i in range(0, len(newdata), 32))
    out_s = out_s[:m.start()] + f"static const uint8_t SMP_DATA[{len(newdata)}] = {{\n{rows}\n}};" + out_s[m.end():]
    # zones
    a, b = section(out_s, "static const smp_zone_t SMP_ZONES[] = {")
    body = out_s[a:b].rstrip()[:-2].rstrip()
    out_s = out_s[:a] + body + "\n" + "".join(f"    {{{z}}},  /* (Vallée) SAX */\n" for z in moved) + "};\n" + out_s[b:]
    # sets: SAX in set 3
    a, b = section(out_s, "static const smp_set_t SMP_SETS[] = {")
    sets = re.findall(r'\{"([A-Z ]+)", (\d+), (\d+)\}', out_s[a:b])
    if sets[SAX_SET][0] != "EMPTY":
        raise SystemExit("gen_samples_vallee: SPAMPY's set 3 is not empty")
    sets[SAX_SET] = ("SAX", str(nz0), str(snz))
    out_s = out_s[:a] + "static const smp_set_t SMP_SETS[] = {\n" + "".join(
        f'    {{"{n}", {z0}, {nz}}},\n' for n, z0, nz in sets) + "};\n" + out_s[b:]
    names = ", ".join(f'"{n}"' for n, _, _ in sets)
    out_s = re.sub(r"#define SMP_SET_NAMES_INIT .*\n", f"#define SMP_SET_NAMES_INIT {names}\n", out_s)
    out_s = re.sub(r"static const char \*const SMP_SET_NAMES\[\] = \{.*?\};", f"static const char *const SMP_SET_NAMES[] = {{{names}}};", out_s)
    # presets: SPAMPY's (their extras in Vallée's form), then Vallée's SAX sounds
    si = load_import()
    a, b = section(out_s, "static const preset_t SMP_PRESET_TABLE[] = {")
    sp_presets = si.presets(out_s[a:b], "felucca_samples.h")
    va_a, va_b = section(va, "static const preset_t SMP_PRESET_TABLE[] = {")
    vrows = [r for r in va[va_a:va_b].splitlines() if re.match(r'\s*\{"(SAX|DARK SAX)"', r)]
    if len(vrows) != 2:
        raise SystemExit("gen_samples_vallee: Vallée's SAX sounds not found")
    vrows = [re.sub(r"\{(\d+), ", lambda m: "{%d, " % SAX_SET, r, count=1) for r in vrows]
    body = sp_presets.rstrip()[:-2].rstrip()
    table = body + "\n" + "\n".join(vrows) + "\n};\n"
    n = len(re.findall(r'^\s*\{"', table, re.M))
    nsets = len(sets)
    out_s = out_s[:a] + table + (f"#define SMP_NPRESETS {n}\n#define SMP_NALIAS 0   /* (every set plays itself) */\n"
                                 f"static const uint8_t SMP_SET_ORIG[] = {{{', '.join(str(i) for i in range(nsets))}}};\n"
                                 ) + out_s[b:]
    out_s = ("/* (Vallée, the SPAMPY sound) tools/gen_samples_vallee.py: SPAMPY's sets (tools/spampy/gen_samples.py) and "
             "Vallée's SAX in set 3 */\n") + out_s
    Path(out).write_text(out_s)
    print(f"samples (Vallée): SPAMPY {len(spd)} B + SAX {len(sax_data)} B = {len(newdata)} B, {nsets} sets, {n} sounds")


if __name__ == "__main__":
    main(sys.argv[1])
