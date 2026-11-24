# Porting SPAMPY's sound into Vallée (branch `spampy-engine`)

Goal: Vallée plays SLOOP's engines exactly as SPAMPY uses them, with SPAMPY's factory sounds, drum kits,
level matching and mix, and keeps every Vallée feature. Not merged to main until the owner approves.

Sources: SPAMPY (github.com/Spampy-Droid/SPAMPY-0.0-r949, GPL-3.0, tag v1.1-r058 = the r949 tree) on
SLOOP (github.com/isod89/sloop-fm1, GPL-3.0) on Felucca (Leo Kuroshita, GPL-3.0).

The old version stays installable: branch `vallee-skiff-engine` = release v0.9.1-alpha (commit d8aa0d4).

## Measured (FM-1 toolchain, 2026-11-23)

| | bytes |
|---|---|
| Vallée 0.9.1 release image | 576,828 (slot 581,564) |
| Vallée sound code + presets + samples | ~179,500 (samples 87,011) |
| SPAMPY image | 548,356 |
| SPAMPY sound code + presets + kits + samples | ~321,000 (samples 195,956) |
| Expected free after the swap, samples in the sample area | ~45,000 (30–45 K after edit pages) |

## Decisions

- Direction: SPAMPY's engines move into Vallée (not Vallée's features onto SPAMPY): the engine interface
  (`engine_t`: note_on, render, amp, desc, block, ownenv, done) is the same; the sequencers, step formats
  and track counts are not (SPAMPY 4 parts + a drum track, Vallée 6 parts).
- Samples: PIANO, VIBES, FLUTE, PERC (SPAMPY) and SAX (Vallée) live in the user-sample flash area
  0xA0000..0xDBFFF as one factory block (PERC alone is larger than an 80 KiB slot). The zone table stays in
  the firmware; the block carries a magic + CRC; missing or wrong data plays silent (eng_sample's missing-sample
  path). Written by the installer after the firmware, over the editor protocol (~850 x 256 B, ~30 s).
- Drums: SPAMPY's drum player (drums.c, drum_synth.c, punch.c, 30 synthesised kits + the GM sample kit and
  its four treatments) becomes Vallée's DRUM engine, playable on any track.
- Old projects: engine numbers change; a load maps old engine ids to the nearest new engine and its
  default sound, and never crashes (tests/persistence_test.c, the stress fuzz).
- GEN, THE SPELL, the creatures, SUMMON: GEN picks sounds by name, so the style tables get SPAMPY names;
  creatures follow the loaded sound; SUMMON is rebuilt on the new engines and still follows the chords.

## Phases (each ends with a commit that builds and passes the tests)

- [x] 0. Safety branch `vallee-skiff-engine`; work branch `spampy-engine`; measurements.
- [x] 1a. Factory sample block mechanism (eng_sample.c smp_fact_scan, editor SMP_* slot 127, INFO 56 01,
      tests/factory_smp_test.c); Vallée's own PIANO FLUTE SAX moved there. Image 576,820 -> 490,632 B. CI green.
- [ ] 1b. The block's contents: SPAMPY's PIANO VIBES FLUTE PERC (its gen_samples.py, byte-identical) + SAX
      (with phase 2). The installer writes the block after the firmware (web/fm1ota.js or the installer page).
- [x] 2a. SPAMPY's new engines (PROPHET CZ SID OBERVAVE MORPH DRONE; DRAWBAR waiting) compile in Vallée
      (tools/sloop_import.py -> firmware/src/sloop/), state in Vallée's shared engine areas. Not in the table yet.
- [~] 2b. The table (WIP commit, see Status): SPAMPY's numbering 0..18 kept (its presets, PRESET_MAP, preset_trim, catalogue carry over),
      19 DRUM (SPAMPY's kits), 20 SUMMON: NENGINES 21 (the light-track mask holds 21). Findings from a trial compile
      of SPAMPY's shared engines in Vallée (scratch, not committed):
      - ANALOG PHASE LOFI FORMANT TRIO: drop in; ANALOG needs P_FTYPE (LP BP HP): reuse Vallée's P_AFLT slot
        (MOOG OBERHEIM JUNO go, as expected).
      - FM6: SPAMPY's fm6_core.c (79 lines differ) and its patch set (gen_fm6_patches.py: FM6_EXTRA, the 17 original
        FM-1 voices) inside Vallée's FM6 frame (user patches fm6_own / up_fm6.c), track field pold.
      - DIGITAL: SPAMPY's (24 presets; its catalogue uses TUBE BELL, DX7 BELL) replaces Vallée's FM4->FM6 alias.
      - SPAMPY's catalogue (ui.c BANK[], ~170 sounds by kind BASS KEYS ORGN PAD LEAD PLCK STAB FX) = the factory
        sound list; engines.c PRESET_MAP / preset_data / preset_trim.h carry over with it.
      - Presets with an arp in SPAMPY (left to the track in Vallée): RETRO ARP, NIGHT SEQUENCE, 8BIT ARP,
        MIDNIGHT ARP, SEQ PULSE, DREAM ARP, ORBIT ARP.
- [ ] 2c. Old projects: old engine id -> new (ANALOG 0, DIGITAL/FM6 -> 9, PHASE 2, LOFI 3, SAMPLE 4, VOICE 5, TRIO 6,
      WHEEL 7, GRAIN/PHYS/NOISE -> nearest + its default sound, DRUM/DRUM-X -> 19, SUMMON -> 20), the sound reset to
      the new engine's default, patterns and mix kept; a marker so new projects are not mapped twice.
- [ ] 3. SPAMPY drums as the DRUM engine.
- [ ] 4. SPAMPY's mix stage (levels, glide, master) in fx.c.
- [ ] 5. Rewire GEN, PART SONG, THE SPELL, creatures, EDIT/SAVE/PRESETS; rebuild SUMMON.
- [ ] 6. Tests: size, every page in both themes, every sound and kit, old projects, goldens on purpose.
- [ ] 7. (after approval) installer: Vallée + Vallée Classic.

## Status

Phases 0, 1a, 2a done; 2b in progress (a WIP commit: CI is expected to fail until the goldens are re-recorded).
Done in 2b so far:
- The engine table by SPAMPY's numbers (core.h, engines.c): SPAMPY's ANALOG DIGITAL PHASE LOFI VOICE TRIO WHEEL
  (drawbar) MORPH PROPHET CZ SID OBERVAVE DRONE (sloop/), Vallée's SAMPLE (factory block), FM6, DRUM, SUMMON.
  GRAIN PHYS NOISE (Skiff's) WHEEL (Vallée's) DRUM-X SLICE gone from the table (DRUM-X's UI code still compiled,
  never reached: ENGI_DRUMX 0xFF; remove in a cleanup).
- Reserved numbers 8 10 12 17: a silent engine (never a call through 0), never offered.
- eng_eff (engines.c): the audio never plays PROPHET CZ SID OBERVAVE on a light track, whatever asked (found by
  regress: CZ on track 6 by MIDI wrote past the light area).
- OBERVAVE's per-card tables read past their 8 entries in SPAMPY too (voice 9, 10): wrapped to the 8 cards
  (tools/sloop_import.py PATCHES; found by vallee_stress under ASan).
- The EDIT layer: 15 engines on F3..F5, INIT on G5; SID and SAMPLE by PRESETS' ENG knob.
- Favourites: each engine an 8-byte slice in rows 0, 2..5, clear of the settings in rows 1, 14, 15 (DIGITAL's
  return would have collided with AUTO and MIDI LEARN); the old stars cleared once (FAV_ERA in row 7).
- Projects: 'S' 1 after the tracks marks SPAMPY's numbers; a record without it is mapped (ENG_FROM_SKIFF), DRUM and
  SUMMON kept, the others the new engine's first sound (PROJ_NEW_ENG); entries' sounds too. persistence_test.
- ANALOG's FILTER slot holds SPAMPY's TYPE (LP BP HP).
- GEN's engine constants follow; its sound names are still the old ones (a missing name: the engine's first).
- regress: PROPHET's long releases (Sequential's programs, up to ~60 s) allowed 60 s; every check else unchanged.
Done since:
- Level matching: sloop/sloop_presets.c (SPAMPY's PRESET_MAP, preset_data, preset_trim verbatim) + preset_logical_of;
  preset_fill sets P_ED_FX = the trim (0 for sounds SPAMPY does not number: SAX, DRUM's, SUMMON's); fx.c trk_lvl
  adds it to LEVEL in all four output paths, as SPAMPY's fx.c. (SPAMPY's LOUD, a per-sound boost the user turns on,
  off by default: not ported.)
- 1b: tools/gen_samples_vallee.py: SPAMPY's sets byte for byte (tools/spampy/gen_samples.py over
  assets/samples-spampy) + Vallée's SAX in SPAMPY's empty set 3; SAMPLE's sounds VIBES LOFI FLUTE GM KIT SAX DARK SAX.
  Block 217,753 B (area 245,760).
- FM6: SPAMPY's patches (tools/spampy/gen_fm6_patches.py, byte for byte: F1 F5 F7 F8 kept, the others EMPTY as in
  SPAMPY; 17 voices of the FM-1's own bank, assets/fm1-original: M-VAVE's data, as SPAMPY ships it) and its 22 sounds;
  PATCH F1..F8, OWN, O1..O17 (SPAMPY's PTCH 35..51 translated); fm6_slot_ok / fm6_slot_pk everywhere a factory slot
  was assumed. The FM6 core is the same DSP as SPAMPY's (only where the buses live differs).
- User presets: version 7 = SPAMPY's numbers; an older record is mapped in RAM at load (up_from_skiff), SAMPLE DRUM
  FM6 SUMMON kept (projects keep those four too now); ENG_FROM_SKIFF in core.h.
- regress: PROPHET and FM6 O1..O17 (long releases, bells) 90 s to free; health clean; ASan stress clean.
- The app is built as SPAMPY builds it: -mcpu=r3 -mfprev1 -ffp-contract=off (the FPU: PROPHET's float kernels; no
  soft-float helpers exist to link). PROPHET's kernels (.p5_text) and tables (.p5_tables) at the start of .data in RAM,
  as SPAMPY's app.ld; RAM 96 KiB (POOL -4 KiB: 0x01C20000, 0x54000), the guard's RAM PC window to _guard_ram_end.
  CZ's patch copy in the POOL (RAM was 200 B over).
- FM-1 build (CI run 45324607136): image 528,088 B of 581,564 (53 KB free); RAM .data+.bss 97,640 of 98,304 (664 B
  free: phase 3 puts new state in the POOL); pool 313,144 of 344,064 (31 KB spare); RAM code 29,756 of 32,512.
  Not yet measured on the unit: PROPHET's CPU (it runs on the FPU from RAM, as in SPAMPY).
- Tests moved to the new engines: editor, upreset, persistence, settings, drum, chord, feat13, dualcore (0 of 47
  scenes differ: VOICE's breath noise per track under FELUCCA_CPU2, as Skiff's own fix), project, descdump; the
  editor mock's tables regenerated from desc.json (tools/gen_editor_mock.py). Power-on sounds: JP8 PAD, RHODES,
  CHIP CHORD, DRUM, VIBES, DRUM.
- Every project format of before is mapped once on import (project.c proj_import_any; SKF4 by its mark): a Classic
  SKF3 had kept its old numbers. Old user presets mapped before their values are clamped (they had been clamped by
  the wrong engine's ranges).
- Retired here (kept on Classic): DIGITAL/FM4, PHYS, NOISE, DRUM-X tests, the FELUCCA_FM4=1 UI builds, conv_test.
- Decision for phase 7: the installer converts archives of before 2.0 with Classic's converter (tested there);
  this firmware maps Classic's formats on load. web/conv still builds here (the DIGITAL path Classic's alone).
Next: the rest of the host suite's failures (full run 2026-11-24, /tmp/all2.log; the run stops at its time limit
after the second core's tests, so the web pages and later were not reached):
- ui_test (all three builds): sound-browser checks written for the old factory list (SAMPLE PIANO/PERC aliases,
  LIST BASS, types and unique names, RAVE arp preset, GEN style sounds), PHYS's key on T5, PRESETS elsewhere,
  the undo/redo of loads, user preset version, MIDI after overflow.
- follow_test (chord follow, 6), motion, ratchet, insert (MIX 0 bit for bit), tracks (PHASE PD BRASS block edge),
  midi_control (done: CC74 / CC71 table rebuilt for the new engines), tail (SPEAKER EQ H3/H1 as a note fades: not a
  regression, the output path is unchanged; the test's source was Skiff's ANALOG, whose amplitude path was finer than
  SLOOP's and FM6's (FM6 measures the same on Classic): needs a precise test-only source),
  persistence (FM6 bank -> user presets migration), bank (a DRUM-X favourite row), backup (SKF3 objects: now
  mapped, not kept as they are), robustness, regress goldens (expected: re-record after listening).
Since then fixed: midi_control, follow (TRANS: ACID 303 is an octave down), motion and ratchet (old formats with
the mapping off, PROJ_ERA_TEST), insert (JP8 PAD for Classic's SOFT PAD), robust (a reserved engine refused in a
project; DIGITAL is an engine again; old v4/v5 records mapped to v7 sounds as the bank loads), backup, UI (all but
GEN's style sounds: phase 5).
Found on the way (pre-existing, Classic too, not from the swap): with an INSERT TYPE set the track runs 64 blocks of
tail instead of 16, so a short sound's last few ms differ by rounding at MIX 0 (inaudible; the test's pad hides it).
QUESTION for the user: SPAMPY's PHASE steps its DCW once a block (measured in SPAMPY's own host build: SOFT KEYS
~108 x the in-block change at block edges, CZ LEAD ~14 x, CZ BRASS ~24 x without touching the knob; Skiff fixed the same
in its PHASE, #41, by fading the step). Keep it as SPAMPY's, or apply the fade (a faint buzz gone; not bit-identical to
SPAMPY then)? Kept as SPAMPY's for now (tests/hostsim.c reports, does not fail).
Then re-record the goldens after listening / looking.
Then 3 (drums), 4 (mix), 5 (GEN, SUMMON...), 6 (tests), 7 (installer).
Local host builds: build/gen from a CI-like build + tools/gen_factory_block.py + tools/gen_p5_tables.py.
Do not install this branch on a unit yet (the installer does not write the sample block).
