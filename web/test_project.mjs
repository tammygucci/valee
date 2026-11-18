// SPDX-License-Identifier: GPL-3.0-only
// (Vallée) fm1project.js against the firmware's own backup code (tests/proj_dev.c: editor_backup.c, project.c, bank.c
// over simulated flash): a project to a file, then into another unit's slot, its patterns shared where the same and
// written where free; the pool too full: refused before any write; the loaded project plays the same notes.
//   node web/test_project.mjs <proj_dev binary>
import { spawn } from "child_process";
import { createInterface } from "readline";
import { exportProject, importProject, readProject, projectPatterns } from "./fm1project.js";

let fails = 0;
const ok = (c, what) => { console.log(`project: ${what.padEnd(84)} ${c ? "ok" : "FAIL"}`); if (!c) fails++; };
const BIN = process.argv[2];

function unit(setup) {
  const p = spawn(BIN, [], { stdio: ["pipe", "pipe", "inherit"] }), rl = createInterface({ input: p.stdout }), waiting = [];
  rl.on("line", (l) => waiting.shift()(l));
  const line = (s) => new Promise((res) => { waiting.push(res); p.stdin.write(s + "\n"); });
  const u = {
    request: async ([cmd, args]) => (await line([cmd, ...args].join(" "))).split(" ").filter(Boolean).map(Number),
    load: async (k) => (await line(`load ${k}`)),
    list: async () => (await line("65")),
    close: () => p.stdin.end(),
  };
  return line(`setup ${setup}`).then((r) => { if (r !== "1") throw new Error("setup failed"); return u; });
}

const a = await unit("a");
const fileA = await exportProject(a.request, 0, "test");
ok(fileA && fileA.name === "SONG A" && fileA.patterns.map((p) => p.id).join() === "0,1,2,5", "export: the slot, its name, the four patterns its lists name (one named twice)");
ok(await exportProject(a.request, 2) === null, "export: an empty slot gives nothing");
const aSong = await a.load(0);

const b = await unit("b 6");                       // 58 patterns used, 6 free; B's 0 the same as A's 1
const bBefore = await b.load(1);
const r = await importProject(b.request, JSON.parse(JSON.stringify(fileA)), 2);
ok(r.written === 3 && r.shared === 1, "import: three patterns written, the one the unit already has shared");
const bSong = await b.load(2);
ok(bSong.replace(/ \d+:/g, " :") === aSong.replace(/ \d+:/g, " :") && bSong.startsWith("LOADED"), `import: it loads and plays the same notes (${bSong})`);
ok(await b.load(1) === bBefore, "import: the unit's own project as it was");

const full = await unit("b 2");                    // 2 free: 3 needed
const listBefore = await full.list();
let err = null;
try { await importProject(full.request, fileA, 2); } catch (e) { err = e; }
ok(err && err.code === "full" && err.need === 3 && err.free === 2, "import: the pool too full: refused, saying how many it needs");
ok(await full.list() === listBefore, "import: refused before writing anything");

const again = await importProject(a.request, fileA, 3);
ok(again.written === 0 && again.shared === 4, "import: into the unit it came from, every pattern shared, none written");
ok((await a.load(3)).replace(/^\S+/, "") === aSong.replace(/^\S+/, ""), "import: that copy plays the same");

const bad = JSON.parse(JSON.stringify(fileA)); bad.patterns.pop();
ok((() => { try { readProject(bad); return false; } catch { return true; } })(), "file: one missing a pattern refused");
const bad2 = JSON.parse(JSON.stringify(fileA)); bad2.project.data = bad2.project.data.replace(/^./, (c) => c === "A" ? "B" : "A");
ok((() => { try { readProject(bad2); return false; } catch { return true; } })(), "file: a damaged project refused");
ok(projectPatterns(readProject(fileA).rec).length === 4, "file: read back whole");

for (const u of [a, b, full]) u.close();
console.log(fails ? `project tests: ${fails} FAILED` : "project tests passed");
process.exit(fails ? 1 : 0);
