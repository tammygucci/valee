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
- [ ] 3. SPAMPY drums as the DRUM engine. DECIDED (user, 2026-11-24): 16 lanes, as SPAMPY's (old projects may change;
      they still open safely: an old 8-lane hit maps by its GM note to SPAMPY's lane, LANE_OF_GM: kick 36 -> KICK,
      38 SNARE, 39 CLAP, 42 HAT, 46 OPEN HAT, 45 LOW TOM, 37 RIM, 56 COWBELL).
      3a data model: NLANE 16; step_t's hit / acc 16 bits (packed: 15 bytes, no padding); the ~126 hit / acc sites
         audited; RAM (.bss +~1.7 KB) moved to the pool (31 KB free); every stored format (patterns, projects, user
         patterns, editor protocol, backups) a new version, old 8-lane bytes mapped on read.
         DONE (2026-11-24): step_t packed (#pragma pack, 15 B); a step holds notes or hits, never both (every drum
         note is a lane's: LANE_OF_GM; recording, step_to_grid, the editor's STEP_SET enforce it), so step_put9
         keeps lanes 9..16 in the note bytes; bank records BK_VER4 (older: LANE_FROM8 on read); FUN8 / the
         converter's formats keep 8 lanes (step_put9_old, lanes_to8); editor INFO 5A 01 16, STEP_SET 21 bytes,
         replies 3 bytes longer; GEN maps its 8 rows through lanes_from8; duck: KICK and KICK 2; lane mutes 16-bit
         by grid lane; gen_w and pat_load's buffer in the pool; web: 16 lane names / LANE_OF (hex), toGrid.
         Interim until 3b/3c: the drum voices are still Skiff's 8 (dv_lane, DV_LANE_NOTE, dv_lane_name: the LANES /
         LANE MIX pages and P_LN / P_LTUNE / P_LPAN stay 8 DV lanes); black keys 1..8 and the grid show the page of
         8 lanes holding the selected one (KLANE, lane_view); the web editor reads lanes 1..8 only (no 20-byte
         steps yet); user-preset grid records (old format) 8 lanes.
      3b engine: drums.c, drum_synth.c, punch.c (GM sample kit + 4 treatments, 30 synth kits, punch, per-lane TUNE /
         DECAY / BODY) as Vallée's DRUM engine on any track; voices in Vallée's model.
         DONE (2026-11-24): eng_drum.c rewritten on SPAMPY's drums.c (its sample kit and treatments, its voice logic:
         6 voices, the hat choke, a taken voice's tail) and drum_synth.c (verbatim but the SYN1..4 bank; the noise
         seed the part's own, so both cores render the same); tools/spampy/gen_drumkits.py + drumkit_levels.json
         byte-identical output. KIT 0..34 (808 at power-on, TRK_DEF; an old project's DRUM track: 808), a sound per
         kit; HOME's knobs KIT DIST DLY REV; SPAMPY's keys (a white key a lane F3..G5, a black key its left one's).
         The 16 lanes' parameters: P_LN8.. P_LPAN8.. P_LTUNE8.. P_LDECAY8.. and P_LBODY0..15 before P_E0 (SIDs
         149..196); TUNE -8..7 st, DECAY -4..3, BODY -4..4 (SPAMPY's units; an old project's lanes reset).
         LANES: LANE LEVEL TUNE DECAY; LANE MIX: LANE LEVEL PAN BODY; both show the page of 8 lanes holding the
         selected one. SKF4 layout 2 (writes): no DRUM-X kit, an FM6 track's patch after its parameters, the lane
         SIDs only on a DRUM track (room for 16 lanes on every track); layout 1 still read. User presets keep the
         first 176 SIDs (UP_NSID): lanes 9..16's DECAY and every BODY at their defaults in a user sound.
         Editor DESC sends up to 40 names (DRUM's KIT 35). MIDI OUT: two tracks on one channel never stack a note.
         NOT taken (ask the owner): punch.c is SPAMPY's PUNCH-IN FX (hold FX + a key: the whole mix through a
         loop / filter / stop), not part of the drums; SYN1..SYN4 (the editor's DRUM SYNTH page, a flash bank);
         user sample kits USR1..4 (the FM-1's sample area holds the factory block).
      3c screens and features: the grid and LANES / LANE MIX on 16 lanes; keys F3..G5 the 16 lanes (SPAMPY's
         lane_of_key); GEN's drum writer, THE SPELL, SUMMON; the web editor and emulator; both themes reviewed.
         DONE (2026-11-24): 16-lane screens (OPEN HAT, COWBELL fit; the lane page follows the LANE knob: a key
         toggle was tried and dropped, it flipped pages while finger drumming), the web editor and its emulator on
         20-byte steps (INFO 5A 01 16). GEN / THE SPELL / SUMMON on more lanes: with phase 5.
      3e PUNCH-IN FX (the owner: yes; SYN1..4 and user kits: no, for now). DONE (2026-11-24): punch.c, SPAMPY's 27,
         as the FX layer's keys (perform.c keeps its macros, latch, THE SPELL's keys: 14 of the 27). No pool for its
         64 KB ring: the SLICER's sl_buf borrowed while FX is held or an effect plays, at 22.05 kHz (0.74 s, as
         SPAMPY's); FREEZE grains of the ring (SPAMPY's holds its own reverb: phase 4). FX's black keys no longer
         mute (GLO's do). tests/perform_test.c rewritten after SPAMPY's punch_test.c.
      3d verify: tests, CI, renders, old projects, RAM / CPU budgets. DONE (2026-11-25): a v0.9.1 unit's flash
         (tests/v091_flash.bin.gz: its project saved by v0.9.1-alpha's own code, persistence_test.c old_unit_test)
         loads: every sound on today's engines (DRUM and DRUM-X as DRUM 808), notes, the 8 lanes as 16, 97 BPM,
         plays 6 s without a clip. RAM: .data+.bss 95,960 / 98,304, pool 319,864 / 344,064 (unchanged by the FX),
         RAM code 28,068 / 32,512. CPU (host): the FX layer's worst effect (KILL MID) 35 ns a sample, a DRUM part
         51, the heaviest part (PROPHET, 8 notes) 840.
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
  midi_control (done: CC74 / CC71 table rebuilt for the new engines), tail (done: its source is a host-only exact
  sine at reserved engine 8, tests/host_sine.h + engines.c HOST_TEST_ENGINE; each case's limit Classic's own
  measurement with that source + 1.5 dB: Classic and this branch measure the same, the way out is unchanged),
  persistence (FM6 bank -> user presets migration), bank (a DRUM-X favourite row), backup (SKF3 objects: now
  mapped, not kept as they are), robustness, regress goldens (expected: re-record after listening).
Since then fixed: midi_control, follow (TRANS: ACID 303 is an octave down), motion and ratchet (old formats with
the mapping off, PROJ_ERA_TEST), insert (JP8 PAD for Classic's SOFT PAD), robust (a reserved engine refused in a
project; DIGITAL is an engine again; old v4/v5 records mapped to v7 sounds as the bank loads), backup, UI (all but
GEN's style sounds: phase 5).
Found on the way (pre-existing, Classic too, not from the swap): with an INSERT TYPE set the track runs 64 blocks of
tail instead of 16, so a short sound's last few ms differ by rounding at MIX 0 (inaudible; the test's pad hides it).
DECIDED (user, 2026-11-24): PHASE gets Skiff's #41 fade (tools/sloop_import.py patch): block-edge steps from up to
~115 x to under 3 x (SOFT KEYS 2.9, CZ BRASS 1.5, CZ STRING knob turned 1.9: the TRACKS check fails over 2 again).
Not bit-identical to SPAMPY's PHASE only while DCW moves. The 7 arp sounds leave the arp as the track's (decided).
Then re-record the goldens after listening / looking.
STATE 2026-11-24, later (CI run 45459611864: ALL GREEN, the first on the new engine; deploy skipped, nothing
published): image 528,224 B of 581,564; RAM 97,704 of 98,304. Goldens re-recorded after review (UI: every changed
screen compared with Classic's in both themes; audio: 19 changed, all on swapped engines, the rest bit for bit as
Classic's, 205 new, health clean, one / two cores the same). Found in the review and fixed: DIGITAL's dead FM4 OP
pages, CZ-1's empty EDIT 1, the PROPHET / OBERVAVE key labels, the new engines' icons. Host tests built with
-ffp-contract=off (CI's newer compiler fused PROPHET's float math). target_budget: SPAMPY's engines measured
(p5_samples 7446, obv_render 2664, cz_native_render 1403, sid_render 1148, the rest under 300).
Decided: PHASE's #41 fade on; arp sounds leave the arp to the track.
NEXT: phase 3 (SPAMPY's sampled drums as DRUM), 4 (its mix stage), the rest of 5 (PART SONG, THE SPELL's takes,
creatures by sound, SUMMON rebuilt, DRUM-X code removed), 6 (emulator walk, old projects on a unit), 7 (installer).
Then 3 (drums), 4 (mix), 5 (GEN, SUMMON...), 6 (tests), 7 (installer).
Local host builds: build/gen from a CI-like build + tools/gen_factory_block.py + tools/gen_p5_tables.py.
Do not install this branch on a unit yet (the installer does not write the sample block).
