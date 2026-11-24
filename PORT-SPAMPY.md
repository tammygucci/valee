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
- [ ] 1. Factory sample block in the sample area (eng_sample.c, editor protocol, installer, tests).
- [ ] 2. SLOOP engines + SPAMPY presets + level trims replace Skiff's; old-id mapping.
- [ ] 3. SPAMPY drums as the DRUM engine.
- [ ] 4. SPAMPY's mix stage (levels, glide, master) in fx.c.
- [ ] 5. Rewire GEN, PART SONG, THE SPELL, creatures, EDIT/SAVE/PRESETS; rebuild SUMMON.
- [ ] 6. Tests: size, every page in both themes, every sound and kit, old projects, goldens on purpose.
- [ ] 7. (after approval) installer: Vallée + Vallée Classic.

## Status

Phase 0 done. Next: phase 1.
