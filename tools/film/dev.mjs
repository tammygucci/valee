// SPDX-License-Identifier: GPL-3.0-only
// A headless Vallée (build/emu/felucca.wasm) to drive and film.
import fs from "fs";
export const B = { FX: 0, SEL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11, OCTDN: 12, OCTUP: 13 };
export const EN = { SELECT: 0, ALGO: 1, PRESETS: 2, K1: 3, K2: 4, K3: 5, K4: 6 };
// keys F3..G5, bit k
const NAMES = ["F3","F#3","G3","G#3","A3","A#3","B3","C4","C#4","D4","D#4","E4","F4","F#4","G4","G#4","A4","A#4","B4","C5","C#5","D5","D#5","E5","F5","F#5","G5"];
export const K = Object.fromEntries(NAMES.map((n, i) => [n, i]));

export async function device(wasmPath, { onFrame } = {}) {
  const bytes = fs.readFileSync(wasmPath);
  const { instance } = await WebAssembly.instantiate(bytes, {});
  const ex = instance.exports, mem = ex.memory;
  ex._initialize();
  ex.web_nor_erase();
  ex.web_boot();
  ex.web_master(800);
  const FPS = 30, SR = 44100;
  const d = { ex, mem, t: 0, audio: [], frame: 0, btn: 0, keys: 0 };
  let acc = 0;   // samples rendered toward the next frame
  const nextFrameAt = () => Math.round((d.frame + 1) * SR / FPS);
  let total = 0;
  d.screen = () => new Uint16Array(mem.buffer, ex.web_screen(), 240 * 240).slice();
  d.run = (ms) => {
    const n = Math.round(ms * SR / 1000);
    let k = 0;
    while (k < n) {
      const m = Math.min(512, n - k, nextFrameAt() - total);
      ex.web_render(m);
      const l = new Float32Array(mem.buffer, ex.web_out_l(), m), r = new Float32Array(mem.buffer, ex.web_out_r(), m);
      for (let i = 0; i < m; i++) d.audio.push(l[i], r[i]);
      total += m; k += m;
      if (total >= nextFrameAt()) { d.frame++; if (onFrame) onFrame(d); }
    }
    d.t += ms;
  };
  d.setBtn = (mask) => { d.btn = mask; ex.web_buttons(mask); };
  d.tap = (b, hold = 70) => { d.setBtn(d.btn | (1 << b)); d.run(hold); d.setBtn(d.btn & ~(1 << b)); d.run(60); };
  d.down = (b) => d.setBtn(d.btn | (1 << b));
  d.up = (b) => d.setBtn(d.btn & ~(1 << b));
  d.turn = (role, n, gap = 45) => { for (let i = 0; i < Math.abs(n); i++) { ex.web_enc(role, Math.sign(n)); d.run(gap); } };
  d.keyDown = (k) => { d.keys |= 1 << k; ex.web_keys(d.keys); };
  d.keyUp = (k) => { d.keys &= ~(1 << k); ex.web_keys(d.keys); };
  d.key = (k, hold = 120) => { d.keyDown(k); d.run(hold); d.keyUp(k); d.run(40); };
  return d;
}

export function rgb(fb) {   // RGB565, the panel's byte order -> RGB bytes
  const out = Buffer.alloc(240 * 240 * 3);
  for (let i = 0; i < fb.length; i++) {
    let v = fb[i];
    v = ((v & 0xff) << 8) | (v >> 8);
    const r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
    out[i * 3] = (r << 3) | (r >> 2); out[i * 3 + 1] = (g << 2) | (g >> 4); out[i * 3 + 2] = (b << 3) | (b >> 2);
  }
  return out;
}
export function ppm(path, fb) {
  fs.writeFileSync(path, Buffer.concat([Buffer.from("P6\n240 240\n255\n"), rgb(fb)]));
}
