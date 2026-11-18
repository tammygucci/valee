// (Vallée) SUMMON's clip (web/brand/vallee_summon.mp4): a key starts it, it plays in the song's chords and forms over
// the song (filmed 4x faster, its sound in four 10 s pieces from along the way): its sigil and seal chosen first,
// its name kept on page 2, a key fades it;
// TEEN's AURA.
// node tools/film/summon.mjs OUT [teen]: OUT/screen.rgb (the frames kept), OUT/audio.f32 (theirs), OUT/captions.json
import fs from "fs";
import { device, B, EN, K, rgb } from "./dev.mjs";
const OUT = process.argv[2] || "/tmp/film/summon", teen = process.argv[3] === "teen";
fs.mkdirSync(OUT, { recursive: true });
const fd = fs.openSync(`${OUT}/screen.rgb`, "w");
const SPF = 1470 * 2;                                // stereo samples a frame (44100 / 30)
let skip = 1, kept = 0, audio = [];
const d = await device(new URL("../../build/emu/felucca.wasm", import.meta.url).pathname, {
  onFrame: (dv) => {
    if (dv.frame % skip) return;
    fs.writeSync(fd, rgb(dv.screen())); kept++;
    if (skip === 1) audio.push(dv.audio.slice(dv.audio.length - SPF));
  },
});
const caps = [], cap = (t, s = "") => caps.push([kept, t, s]), wait = (ms) => d.run(ms);
const trim = () => { if (d.audio.length > 44100 * 2 * 20) d.audio.splice(0, d.audio.length - 44100 * 2 * 12); };
const fast = (ms, cuts) => {                         // ms of the song, a frame in four kept; sound: 10 s from each cut
  const start = d.t; skip = 4; const pieces = [];
  for (let t = 0; t < ms; t += 1000) {
    const at = d.t - start; d.run(1000); trim();
    for (const c of cuts) if (at >= c && at < c + 10000) pieces.push(d.audio.slice(d.audio.length - 44100 * 2));
  }
  skip = 1;
  const want = Math.round(ms / 4 / 1000 * 30) * SPF, flat = [];
  for (const p of pieces) for (const x of p) flat.push(x);
  for (let i = 0; i < want; i += SPF) audio.push(flat.slice(i, i + SPF).concat(new Array(Math.max(0, SPF - (flat.length - i))).fill(0)).slice(0, SPF));
};
cap("", "");
wait(7200);
if (teen) { d.down(B.HOME); wait(1000); d.up(B.HOME); wait(400); d.tap(B.OCTUP); wait(400); d.tap(B.HOME); wait(500); }
d.turn(EN.ALGO, 2); wait(200);
d.down(B.EDIT); wait(500); d.key(K.C5); wait(200); d.up(B.EDIT); wait(400);
d.tap(B.EDIT); wait(300);
d.tap(B.PLAY); wait(300);
if (!teen) {
  cap("SUMMON", "an ambient engine with a being in it");
  wait(2000);
  d.tap(B.EDIT); wait(300);
  cap("Choose its sigil and seal", "page 2: your starting point, two symbols");
  d.turn(EN.K3, 20, 60); wait(400); d.turn(EN.K4, 40, 60); wait(1800);
  d.tap(B.EDIT); wait(250); d.tap(B.EDIT); wait(250); d.tap(B.EDIT); wait(300);
  cap("Press any key", "it plays by itself, only ever in the song's chords");
  d.key(K.A3); wait(6000);
  cap("The song shapes it", "what the song plays decides what it becomes (shown 4x faster)");
  fast(156000, [0, 50000, 100000, 146000]);
  cap("The being arrives", "its name in the header; one in fifty is a demon, one in fifty an angel, by chance alone");
  wait(6500);
  d.tap(B.EDIT); wait(300);
  cap("Kept with the song", "BEING: its name; the same sigil and seal over the same song grow it again");
  wait(5000);
  d.tap(B.EDIT); wait(250); d.tap(B.EDIT); wait(250); d.tap(B.EDIT); wait(250);
  cap("A key again: it fades", "");
  d.key(K.C4); wait(4500);
} else {
  cap("AURA", "the same on TEEN: rings close in instead of a body (4x faster)");
  d.key(K.E4); wait(2000);
  fast(156000, [20000, 70000, 120000, 146000]);
  wait(3500);
}
const flat = new Float32Array(audio.length * SPF);
audio.forEach((a, i) => flat.set(a, i * SPF));
fs.writeFileSync(`${OUT}/audio.f32`, Buffer.from(flat.buffer));
fs.writeFileSync(`${OUT}/captions.json`, JSON.stringify({ frames: kept, caps }));
console.log("frames", kept, "audio frames", audio.length, caps.map((c) => c[0]).join(","));
