#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""(Skiff XENO) The browser emulator as one HTML file, nothing beside it: build/emu's page with ../panel.js, the font,
worklet.js (a Blob URL) and felucca.wasm (base64) written into it. For sharing the emulator where only a single page
can go (an attachment, a gist, a hosted artifact). Run after web/emu/build.sh or web/emu/build_clang.sh.

  web/emu/bundle_single.py [OUT.html]          (default: build/vallee-web.html)
"""
import base64
import re
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EMU = ROOT / "build" / "emu"


def swap(page, old, new):
    if old not in page:
        raise SystemExit(f"bundle_single: {old[:60]!r} not in the page (web/emu/index.html changed?)")
    return page.replace(old, new, 1)


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "vallee-web.html"
    page = (EMU / "index.html").read_text(encoding="utf-8")
    panel = (ROOT / "build" / "panel.js").read_text(encoding="utf-8")
    worklet = (EMU / "worklet.js").read_text(encoding="utf-8")
    wasm = base64.b64encode((EMU / "felucca.wasm").read_bytes()).decode()
    font = base64.b64encode((EMU / "fonts" / "DotGothic16-subset.woff").read_bytes()).decode()
    assert "</script" not in panel and "</script" not in worklet
    page = swap(page, 'url("fonts/DotGothic16-subset.woff")', f'url("data:font/woff;base64,{font}")')
    page = swap(page, '<script src="../panel.js"></script>', "<script>\n" + panel + "\n</script>")
    page = swap(page, 'fetch("felucca.wasm").then((r) => { if (!r.ok) throw new Error(`felucca.wasm: ${r.status}`); '
                      'return r.arrayBuffer(); })',
                'Promise.resolve(Uint8Array.from(atob(document.getElementById("wasm").textContent), '
                '(c) => c.charCodeAt(0)).buffer)')
    page = swap(page, 'function workletURLs() { return ["worklet.js"]; }',
                'function workletURLs() { return [URL.createObjectURL(new Blob([' + json.dumps(worklet) +
                '], { type: "text/javascript" })), "worklet.js"]; }')
    page = page.replace('href="fonts/OFL.txt"', 'href="https://openfontlicense.org"')
    page = page.replace('href="../webapp/installer/"', 'href="https://github.com/illuminus5/skiff"')
    page = page.replace('href="../webapp/editor/"', 'href="https://illuminus5.github.io/skiff/webapp/editor/"')
    page = page.replace('href="../learn/"', 'href="https://illuminus5.github.io/skiff/learn/"')
    page = swap(page, "</body>", f'<script type="application/octet-stream" id="wasm">{wasm}</script>\n</body>')
    # (Vallée) the processor inline too, for when no worklet can load (a page opened from a file: an opaque origin,
    # no blob module): index.html's processorNode runs it on the page's thread, its exports plain declarations
    plain = re.sub(r"^export\s+", "", worklet, flags=re.M)
    page = swap(page, '<script type="module">', "<script>\nfunction feluccaWorklet() {\n" + plain +
                "\n}\n</script>\n<script type=\"module\">")
    out.write_text(page, encoding="utf-8", newline="\n")
    print(f"bundle_single: {out} ({out.stat().st_size} B)")


if __name__ == "__main__":
    main()
