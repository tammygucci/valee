// SPDX-License-Identifier: GPL-3.0-only
// (Vallée) The feature tour (web/brand/vallee_demo.mp4), filmed from the browser build: see tools/film/make.sh.
// Films Vallée in the browser build: OUT/screen.rgb (240x240 RGB a frame, 30 fps), OUT/audio.f32 (stereo 44.1 kHz),
// OUT/captions.json ([frame, title, sub] from that frame on)
import fs from "fs";
import { device, B, EN, K, rgb } from "./dev.mjs";
const OUT = process.argv[2] || "build/film";
fs.mkdirSync(OUT, { recursive: true });
const fd = fs.openSync(`${OUT}/screen.rgb`, "w");
const d = await device("build/emu/felucca.wasm", {
  onFrame: (dv) => fs.writeSync(fd, rgb(dv.screen())),
});
const caps = [];
const cap = (title, sub = "") => caps.push([d.frame, title, sub]);
const wait = (ms) => d.run(ms);

// 1. the opening
cap("", "");
wait(2600);
cap("VALLÉE", "a song machine for the M-VAVE FM-1");
wait(4700);

// 2. GEN
d.turn(EN.SELECT, 1); wait(500);
cap("It writes the song", "Twelve styles. Pick one, pick a seed.");
for (let i = 0; i < 5; i++) { d.turn(EN.PRESETS, 1); wait(420); }
d.turn(EN.PRESETS, -5, 60); wait(300);
d.turn(EN.K2, 26, 18); wait(500);
d.turn(EN.SELECT, 1); wait(300);
cap("A whole song in one press", "intro, verses, choruses, a bridge, an outro");
d.turn(EN.K1, 6, 90); wait(600);
d.tap(B.OCTUP); wait(700);
d.tap(B.PLAY); wait(2400);

// 3. HOME: the sounds and their creatures
d.turn(EN.SELECT, -1); wait(150); d.turn(EN.SELECT, -1); wait(400);
cap("Its own sounds", "Moog, Juno and Oberheim voices, its own drum kits, a creature for every one");
wait(2200);
d.turn(EN.ALGO, 1); wait(300); d.turn(EN.PRESETS, 1); wait(1300); d.turn(EN.PRESETS, -1); wait(1500);
d.turn(EN.ALGO, -1); wait(1200);

// 4. THE SPELL
d.tap(B.HOME); wait(300);
cap("Then you play it", "Every white key is a part of the song");
wait(2600);
cap("A: the chorus", "it comes in on the next bar, always in time");
d.key(K.A3); wait(5600);
cap("Hold a key past the song", "repeats, tape stop, filter sweeps, freeze");
d.keyDown(K.G5); wait(1300); d.keyUp(K.G5); wait(1200);
d.keyDown(K.A4); wait(1500); d.keyUp(K.A4); wait(1400);
cap("G#: the next part", "one key walks the song");
d.key(K["G#3"]); wait(4600);
cap("A#: a new take", "the part rewritten while it plays");
d.key(K["A#3"]); wait(5400);
cap("The Alchemist conjures", "with your six tracks, and summons a figure: a different one for every song");
wait(7000);

// 5. ALEMBIC
for (let g = 0; g < 6; g++) { d.tap(B.GLO, 50); wait(40); }
cap("ALEMBIC", "the whole mix through four vessels: drive, motion, space, grain");
wait(600);
d.turn(EN.K1, 3, 120); d.turn(EN.K2, 70, 14); wait(700);
d.turn(EN.K3, 2, 120); d.turn(EN.K4, 70, 14); wait(1800);
d.tap(B.GLO); wait(500);
d.turn(EN.K1, 2, 120); d.turn(EN.K2, 50, 14); wait(600);
d.turn(EN.K3, 1, 120); d.turn(EN.K4, 70, 14); wait(2600);

// 6. TEEN
cap("Two looks", "XENO, and TEEN: the same screens, kept plain");
d.down(B.HOME); wait(1000); d.up(B.HOME); wait(500);
d.tap(B.OCTUP); wait(700);
d.tap(B.HOME); wait(3300);
d.tap(B.HOME); wait(800);
d.key(K.A3); wait(3600);
cap("END", "");
wait(5000);

fs.closeSync(fd);
fs.writeFileSync(`${OUT}/audio.f32`, Buffer.from(new Float32Array(d.audio).buffer));
fs.writeFileSync(`${OUT}/captions.json`, JSON.stringify({ frames: d.frame, caps }));
console.log("frames", d.frame, "seconds", d.frame / 30);
