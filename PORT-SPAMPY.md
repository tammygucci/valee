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
- [ ] 2b. The table: SPAMPY's numbering 0..18 kept (its presets, PRESET_MAP, preset_trim, catalogue carry over),
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

Phases 0, 1a and 2a done (commits on `spampy-engine`, CI run 45310256392 green). Next: phase 2 (the engines),
together with 1b. Notes for whoever resumes:
- Local host builds need build/gen: copy the generated headers from a CI-like build, then run
  tools/gen_factory_block.py on felucca_samples.h (tools/build.py does this in CI).
- run_tests.sh stops without a real firmware image; run the host tests one by one (see the commands in it) and
  let CI (gh workflow run vallee.yml --ref spampy-engine) run the whole suite.
- The installer does not yet write the factory block: until it does, a unit given this branch's firmware would
  play every sampled sound as a sine (safe, not useful). Do not install this branch on a unit yet.
