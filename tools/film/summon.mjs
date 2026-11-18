// (Vallée) SUMMON's clip: the ceremony on XENO (SUMMONER), a demon and an angel, then TEEN's AURA.
// node tools/film/summon.mjs OUT [teen]
import fs from "fs";
import { device, B, EN, K, rgb } from "./dev.mjs";
const OUT = process.argv[2] || "/tmp/film/summon", teen = process.argv[3] === "teen";
fs.mkdirSync(OUT, { recursive: true });
const fd = fs.openSync(`${OUT}/screen.rgb`, "w");
const d = await device(new URL("../../build/emu/felucca.wasm", import.meta.url).pathname, {
  onFrame: (dv) => fs.writeSync(fd, rgb(dv.screen())),
});
const caps = [], cap = (t, s = "") => caps.push([d.frame, t, s]), wait = (ms) => d.run(ms);
const chord = (ks, ms) => { ks.forEach((k) => d.keyDown(k)); wait(ms); ks.forEach((k) => d.keyUp(k)); };
cap("", "");
wait(7200);
if (teen) { d.down(B.HOME); wait(1000); d.up(B.HOME); wait(400); d.tap(B.OCTUP); wait(400); d.tap(B.HOME); wait(500); }
d.turn(EN.ALGO, 2); wait(200);
d.down(B.EDIT); wait(500); d.key(K.C5); wait(200); d.up(B.EDIT); wait(400);
d.tap(B.EDIT); wait(300);
if (!teen) {
  cap("SUMMON", "a new engine: drifting partials that answer the unit's own signals");
  chord([K.C4, K.G4], 3200);
  cap("OCT+ begins the ceremony", "candles light as the being forms from its feet up");
  d.tap(B.OCTUP);
  chord([K.A3, K.E4], 3400); chord([K.F3, K.C4], 3400); chord([K.C4, K.G4], 3000);
  cap("Every being is different", "its name and body come from that moment's signals");
  wait(2500);
  d.tap(B.HOME); wait(300);
  cap("It lives on its track", "kept with the project in SIGIL and SEAL");
  chord([K.G3, K.D4], 3200);
  d.tap(B.EDIT); wait(300); d.tap(B.EDIT); wait(300);
  d.turn(EN.K3, -127, 4); d.turn(EN.K4, -127, 4);
  d.turn(EN.K4, 13, 40); wait(300);
  cap("1 in 50: a demon", "darker intervals, a low voice");
  chord([K.C3, K["F#3"]], 3800);
  d.turn(EN.K4, 3, 60); wait(300);
  cap("1 in 50: an angel", "bright fifths and octaves");
  chord([K.C4, K.G4, K.C5], 4200);
} else {
  cap("AURA", "the same engine on TEEN: rings instead of a body");
  d.tap(B.OCTUP);
  chord([K.C4, K.G4], 3400); chord([K.A3, K.E4], 3400); chord([K.F3, K.C4], 3400); wait(1500);
}
wait(400);
fs.writeFileSync(`${OUT}/audio.f32`, Buffer.from(new Float32Array(d.audio).buffer));
fs.writeFileSync(`${OUT}/captions.json`, JSON.stringify({ frames: d.frame, caps }));
