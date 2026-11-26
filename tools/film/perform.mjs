// SPDX-License-Identifier: GPL-3.0-only
// (Vallée) The performance (web/brand/vallee_perform.mp4): GEN writes a song, then THE SPELL plays it from the keys.
// Filmed from the browser build as film.mjs films the tour: OUT/screen.rgb, OUT/audio.f32, OUT/captions.json.
import fs from "fs";
import { device, B, EN, K, rgb } from "./dev.mjs";
const OUT = process.argv[2] || "build/film_perform";
fs.mkdirSync(OUT, { recursive: true });
const fd = fs.openSync(`${OUT}/screen.rgb`, "w");
let rec = false;
const d = await device("build/emu/felucca.wasm", { onFrame: (dv) => { if (rec) fs.writeSync(fd, rgb(dv.screen())); } });
const caps = [];
let f0 = 0;
const cap = (title, sub = "") => caps.push([d.frame - f0, title, sub]);
const wait = (ms) => d.run(ms);
const hold = (k, ms) => { d.keyDown(k); wait(ms); d.keyUp(k); };

// unfilmed: past the opening, GEN set to ZETA, seed 5, a whole song
wait(7200);
d.turn(EN.SELECT, 1); wait(200);
d.turn(EN.PRESETS, 2, 80); d.turn(EN.K2, 4, 40); wait(200);
d.turn(EN.SELECT, 1); d.turn(EN.K1, 6, 40); wait(300);
rec = true; f0 = d.frame;

cap("One press", "GEN writes a whole song: ZETA, seed 5");
wait(1200);
d.tap(B.OCTUP); wait(900);
d.tap(B.PLAY); wait(1500);
d.turn(EN.SELECT, -1); wait(120); d.turn(EN.SELECT, -1); wait(500);
d.tap(B.HOME); wait(400);
cap("THE SPELL", "the song's parts on the white keys, from F");
wait(4600);
cap("A: straight to the chorus", "it comes in on the next bar, always in time");
d.key(K.A3); wait(7600);
cap("A#: a new take", "the chorus rewritten while it plays");
d.key(K["A#3"]); wait(7200);
cap("Hold an effect key", "LOW: the filter closes; G takes it back to the verse");
d.keyDown(K.C5); wait(2200); d.key(K.G3); wait(1000); d.keyUp(K.C5); wait(4400);
cap("Loop into the bridge", "LOOP 32 held, D pressed");
d.keyDown(K.E5); wait(1100); d.key(K.D4); wait(800); d.keyUp(K.E5); wait(5200);
cap("Reverse", "the mix played backwards while the key is down");
hold(K.D5, 1800); wait(3600);
cap("STOP, then the last chorus", "the tape winding down");
d.keyDown(K.A4); wait(1500); d.key(K.E4); wait(300); d.keyUp(K.A4); wait(7000);
cap("F: the outro", "and the figure the Alchemist summoned, whole");
d.key(K.F4); wait(8500);
cap("END", "");
wait(5000);

rec = false;
fs.closeSync(fd);
fs.writeFileSync(`${OUT}/audio.f32`, Buffer.from(new Float32Array(d.audio.slice(d.audio.length - Math.round((d.frame - f0) * 44100 / 30) * 2)).buffer));
fs.writeFileSync(`${OUT}/captions.json`, JSON.stringify({ frames: d.frame - f0, caps }));
console.log("frames", d.frame - f0, "seconds", (d.frame - f0) / 30);
