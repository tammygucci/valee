// SPDX-License-Identifier: GPL-3.0-only
// (Vallée) One project to a file and back, over the complete backup's objects (fm1backup.js: LIST, GET, PUT; no
// flash addresses). A project names its patterns in the pool every project shares (bank.c), so its file holds the
// patterns its lists name. Loading one into a slot: each of its patterns goes to an identical one already in the
// pool, or else to an empty id nothing names (the music now, the four slots: a slot being replaced too), written
// first; then the project with its lists pointed at them. Nothing is written until every pattern has a place.
//   file: { format: "vallee-project", version: 1, firmware, created, name, project: { size, crc, data },
//           patterns: [{ id, data }] }   (data: base64; a pattern 888 bytes, bank.c BK_REC)
import { BACKUP_CMD, bkU32, bkR32, bkPack, bkUnpack, bkCrc, bkManifest } from "./fm1backup.js";

export const PROJECT_FORMAT = "vallee-project";
const REC = 888, PER = 4, NPAT = 64, NTRK = 6, NENT = 16, PAT_REST = NPAT + 1, CHUNK = 256;
const BANK0 = 16, SLOT0 = 2;
// the project records this reads (project.c): SKF4 (2.0's saves) and SKF3 (Skiff's), where their lists are
const FMT = { 0x534b4634: { size: 2560, ent: 10, stride: 4 }, 0x534b4633: { size: 2048, ent: 698, stride: 2 } };

const b64 = (bytes) => { let s = ""; for (const b of bytes) s += String.fromCharCode(b); return btoa(s); };
const unb64 = (s) => Uint8Array.from(atob(s), (c) => c.charCodeAt(0));
const u32 = (b, o) => (b[o] | b[o + 1] << 8 | b[o + 2] << 16 | b[o + 3] << 24) >>> 0;
const fnv = (b, n) => { let s = 0x811c9dc5; for (let i = 0; i < n; i++) s = Math.imul(s ^ b[i], 16777619) >>> 0; return s; };
function check(rc) {
  if (rc) throw new Error(({ 1: "This firmware can't take it (update it first)", 2: "The unit refused the data",
    3: "Stop playback first", 4: "Flash write failed", 5: "Try again" })[rc] || `Error ${rc}`);
}
// a project record: its format, or an error
export function projectFormat(rec) {
  const f = rec && rec.length >= 8 ? FMT[u32(rec, 0)] : null;
  if (!f || u32(rec, 4) !== f.size || rec.length !== f.size || fnv(rec, f.size - 4) !== u32(rec, f.size - 4))
    throw new Error("Not a project this firmware reads (save it on the unit once, then try again)");
  return f;
}
// the pool ids a project record's lists name (an entry: id + 1, 0 none, PAT_REST a rest)
export function projectPatterns(rec) {
  const f = projectFormat(rec), ids = new Set();
  for (let i = 0; i < NTRK * NENT; i++) {
    const v = rec[f.ent + i * f.stride];
    if (v >= 1 && v <= NPAT) ids.add(v - 1);
  }
  return [...ids].sort((a, b) => a - b);
}
// the record with its lists' ids changed by map (old id -> new) and its hash again
export function remapProject(rec, map) {
  const f = projectFormat(rec), out = Uint8Array.from(rec);
  for (let i = 0; i < NTRK * NENT; i++) {
    const p = f.ent + i * f.stride, v = out[p];
    if (v >= 1 && v <= NPAT) {
      if (!map.has(v - 1)) throw new Error("A pattern has no place");
      out[p] = map.get(v - 1) + 1;
    }
  }
  const s = fnv(out, f.size - 4);
  out[f.size - 4] = s & 255; out[f.size - 3] = s >>> 8 & 255; out[f.size - 2] = s >>> 16 & 255; out[f.size - 1] = s >>> 24;
  return out;
}
export function projectName(rec) {
  const f = projectFormat(rec), at = f.size - 4 - 12;
  return String.fromCharCode(...rec.subarray(at, at + 12)).replace(/\0.*$/s, "").trim();
}

async function manifest(request) {
  return bkManifest(await request([BACKUP_CMD.LIST, []], { timeout: 3000, retries: 0 }));
}
async function getObject(request, o) {
  const bytes = new Uint8Array(o.size);
  for (let off = 0; off < o.size; off += CHUNK) {
    const size = Math.min(CHUNK, o.size - off);
    const a = await request([BACKUP_CMD.GET, [o.id, ...bkU32(off), size & 127, size >>> 7]], { timeout: 1000, retries: 1 });
    check(a[1]);
    if (a[0] !== o.id || bkR32(a, 2) !== off || (a[7] | a[8] << 7) !== size) throw new Error("Unexpected reply");
    bytes.set(bkUnpack(a.slice(9), size), off);
  }
  if (bkCrc(bytes) !== o.crc) throw new Error("The unit changed meanwhile. Try again while stopped.");
  return bytes;
}
async function putObject(request, id, bytes) {
  const ask = async (args) => { const a = await request([BACKUP_CMD.PUT, args], { timeout: 4000, retries: 0 }); check(a[2]); };
  await ask([0, id, ...bkU32(bytes.length), ...bkU32(bytes.length ? bkCrc(bytes) : 0)]);
  try {
    for (let off = 0; off < bytes.length; off += CHUNK) await ask([1, id, ...bkU32(off), ...bkPack(bytes.subarray(off, off + CHUNK))]);
    await ask([2, id]);
  } catch (e) { await request([BACKUP_CMD.PUT, [3, id]], { timeout: 4000, retries: 0 }).catch(() => {}); throw e; }
}
const chunkOf = (id) => BANK0 + Math.floor(id / PER);
const recOf = (chunk, id) => chunk.length ? chunk.subarray(id % PER * REC, (id % PER + 1) * REC) : new Uint8Array(REC);

// slot 0..3 -> the file (null: the slot is empty)
export async function exportProject(request, slot, firmware = "", onProgress = () => {}) {
  const man = await manifest(request), byId = new Map(man.map((o) => [o.id, o]));
  const po = byId.get(SLOT0 + slot);
  if (!po || !po.size) return null;
  const rec = await getObject(request, po), ids = projectPatterns(rec), chunks = new Map(), patterns = [];
  const need = [...new Set(ids.map(chunkOf))];
  for (const [i, c] of need.entries()) { chunks.set(c, await getObject(request, byId.get(c))); onProgress(i + 1, need.length); }
  for (const id of ids) patterns.push({ id, data: b64(recOf(chunks.get(chunkOf(id)), id)) });
  return { format: PROJECT_FORMAT, version: 1, firmware, created: new Date().toISOString(), name: projectName(rec),
    project: { size: rec.length, crc: bkCrc(rec), data: b64(rec) }, patterns };
}

// a file (object or JSON text) checked whole: { name, rec, patterns: Map id -> record }
export function readProject(file) {
  if (typeof file === "string") file = JSON.parse(file);
  if (!file || file.format !== PROJECT_FORMAT || file.version !== 1 || !file.project || !Array.isArray(file.patterns))
    throw new Error("Not a Vallée project file");
  const rec = unb64(file.project.data);
  if (rec.length !== file.project.size || bkCrc(rec) !== file.project.crc) throw new Error("The project file is damaged");
  const ids = projectPatterns(rec), patterns = new Map();
  for (const p of file.patterns) {
    const r = unb64(p.data);
    if (!Number.isInteger(p.id) || p.id < 0 || p.id >= NPAT || r.length !== REC) throw new Error("The project file is damaged");
    patterns.set(p.id, r);
  }
  for (const id of ids) if (!patterns.has(id)) throw new Error("The project file is missing a pattern");
  return { name: projectName(rec), rec, ids, patterns };
}

// the places for a file's patterns in a pool: Map old id -> new id, and the ids to write; throws when too few are free
export function planImport(proj, pool, named) {
  const map = new Map(), writes = new Map(), taken = new Set(named);
  const empty = (r) => !r[0];
  for (const id of proj.ids) {
    const r = proj.patterns.get(id);
    if (!empty(r)) {                                   // the same pattern already there: shared, nothing written
      const same = pool.findIndex((p) => !empty(p) && p.every((v, i) => v === r[i]));
      if (same >= 0) { map.set(id, same); taken.add(same); continue; }
    }
  }
  const free = [];
  for (let i = 0; i < NPAT; i++) if (empty(pool[i]) && !taken.has(i)) free.push(i);
  const left = proj.ids.filter((id) => !map.has(id));
  if (left.length > free.length)
    throw Object.assign(new Error(`PATTERNS FULL: this project needs ${left.length} free patterns, the unit has ${free.length}. ` +
      "Delete unused ones on SAVE › PATTERNS first."), { code: "full", need: left.length, free: free.length });
  for (const id of left) { const n = free.shift(); map.set(id, n); if (!empty(proj.patterns.get(id))) writes.set(n, proj.patterns.get(id)); }
  return { map, writes };
}

// the file into slot 0..3 (it replaces what the slot holds); { name, written, shared }
export async function importProject(request, file, slot, onProgress = () => {}) {
  const proj = readProject(file);                    // every byte checked before the first request
  const man = await manifest(request), byId = new Map(man.map((o) => [o.id, o]));
  if (!byId.has(BANK0)) throw new Error("This firmware has no pattern pool");
  const named = new Set();
  for (const id of [0, 2, 3, 4, 5]) {                // what the music now and the four slots name stays
    const o = byId.get(id);
    if (!o || !o.size) continue;
    let rec; try { rec = await getObject(request, o); projectFormat(rec); } catch (e) { if (/changed/.test(e.message)) throw e; continue; }
    for (const p of projectPatterns(rec)) named.add(p);
  }
  const chunks = new Map(), pool = [];
  for (let c = 0; c < NPAT / PER; c++) chunks.set(BANK0 + c, await getObject(request, byId.get(BANK0 + c)));
  for (let i = 0; i < NPAT; i++) pool.push(recOf(chunks.get(chunkOf(i)), i));
  const { map, writes } = planImport(proj, pool, named);
  const dirty = [...new Set([...writes.keys()].map(chunkOf))];
  let done = 0;
  for (const c of dirty) {                           // the patterns first: the project never names one not there
    const cur = chunks.get(c), out = new Uint8Array(PER * REC);
    if (cur.length) out.set(cur);
    for (const [id, r] of writes) if (chunkOf(id) === c) out.set(r, id % PER * REC);
    await putObject(request, c, out); onProgress(++done, dirty.length + 1);
  }
  await putObject(request, SLOT0 + slot, remapProject(proj.rec, map)); onProgress(dirty.length + 1, dirty.length + 1);
  return { name: proj.name, written: writes.size, shared: proj.ids.length - writes.size };
}
