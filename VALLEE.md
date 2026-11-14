# Vallée

Vallée is firmware for the M-VAVE FM-1, a fork of [Skiff](https://github.com/illuminus5/skiff) 2.6. Everything Skiff
does, Vallée does the same way: the pages, the sequencer, the sounds, projects and saves, with one exception: **the
SLICE engine is left out** (see [Room on the FM-1](#room-on-the-fm-1)). On top of that it adds:

- **An opening**: an animated intro with its own chime when you switch on.
- **THEME XENO**: a tape-machine HOME screen, and a creature for every sound.
- **THE WORLD**: tap HOME on HOME and your tracks' creatures dance for the intro's head in an alien land, everything
  moving to the song, the song drawing sigils and constellations on it.
- **GEN**: a song generator in twelve styles, which writes ordinary steps you then edit in the sequencer.

XENO is the theme on a fresh unit. If you install over Skiff your settings are kept, so pick it once in
**MENU > THEME > XENO** (hold HOME, let go: the MENU).

## The opening

At power-on the whole screen plays a 6.8-second intro. It is our own drawing of the "consciousness energy grid" from
the Gateway Process report (US Army, 1983, declassified).

- **0.0 s:** the void fills with stars, and CONSCIOUSNESS ENERGY GRID types itself in at the top.
- **0.3 s:** three planes at right angles grow from the centre, stippled and ruled into grids, turning in space.
- **0.9 s:** a meditating head comes through in the middle, held where the planes cross.
- **1.6 s:** its third eye opens and sends out rays, and the planes' edges pulse with the bells.
- **2.6 s:** VALLÉE comes up under the grid with the version (the title stays to the end).
- **5.4 s:** everything else has gone quiet; the last bell rings on alone and fades to nothing.
- **6.8 s:** only then does it hand over to HOME. (Any button skips the intro after the first 0.3 s.)

The sound is ours too, after what the Gateway tapes used:

- a **binaural beat**: 110 Hz in the left ear, 116 Hz in the right, a 6 Hz theta beat (heard on headphones)
- a soft 55 Hz underneath, and a **surf** of filtered noise that swells twice
- then, as the third eye opens, six **bells** rising in fourths (the "mystic chord")
- and, as the name comes up, a **theremin**-like glide down

It plays at the MASTER level.

- Press any button to skip it.
- MENU > DISPLAY > ANIM OFF gives the plain splash and no sound. So does the first boot after a crash.

The splash under it reads **Vallée**, its version, and "receiving signal / from the other side". The credits to Skiff
and Felucca, with the licence, are on MENU > ABOUT.

## GEN: the song generator

**Opening it.** On HOME, turn **SELECT** right and GEN opens. If you turned BPM LOCK off (SELECT is the tempo on HOME),
use **SHIFT + SELECT** instead: hold HOME and turn SELECT. Turning SELECT back past GEN's first page of knobs returns to
HOME.

**The knobs.** There are four pages of four knobs, and SELECT moves between them. PRESETS always turns STYLE.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| 1 | STYLE | SEED (1–999) | VIBE (DARK HAZY WARM BRIGHT WEIRD) | ENERGY |
| 2 | PART (LOOP INTRO VERSE CHORUS BRIDGE OUTRO) | KEY (AUTO, C … B) | TEMPO (AUTO, 60–180) | SOUNDS (NEW / KEEP) |
| 3 | DRUMS | BASS | KEYS | LEAD |
| 4 | COLOR | COMPLEX | SWING (AUTO, 0–100) | HUMAN |

DRUMS, BASS, KEYS, LEAD and COLOR are each OFF, LITE, ON or BUSY. OFF leaves that track exactly as it is.

**The buttons.**

- **OCT+** writes the piece into the tracks.
- **OCT+ again** with nothing turned rolls the next seed. Turn a knob first and OCT+ writes the same piece with only that
  changed. Each part of the music has its own random stream, so turning DRUMS doesn't change the bass.
- **OCT−** puts back exactly what the tracks held before GEN wrote over them (steps, sounds and tempo). With nothing to
  put back, OCT− goes HOME.
- Leaving the page keeps what was written. SAVE held still undoes it later, one track at a time ("GEN").

**The panel** shows the seed's sigil (every seed draws its own) and the piece before you write it: style, vibe, part,
key, scale, tempo and its chords, for example `Am9 Dm9 Cmaj9 Bm7b5 E9`.

**The tracks** it writes:

| Track | Role |
|---|---|
| T1 | BASS |
| T2 | KEYS (the chords, voice-led) |
| T3 | LEAD (the hook) |
| T4 | DRUMS |
| T5 | COLOR (an arp, a jangle, a pad, a texture, fills or bells, by style) |
| T6 | PERC (a second kit) |

Each part is four bars (64 steps). Everything is ordinary steps and ordinary sounds, so open SEQ and NOTES and make it
your own.

**Making a whole song.** Keep the seed and the style, step through PART (INTRO, VERSE, CHORUS, BRIDGE, OUTRO), and
write each one. A seed keeps its key, tempo, sounds and hook across parts.

GEN writes into each track's current pattern, so keep a part before writing the next. Skiff's song mode works track
by track: on SEQ > SONG, COPY+ puts a copy of the track's pattern in its list (do it on each track, ALGORITHM picking
the track), and the copies chain into the song (DESIGN.md's SONG section has the details). Or save the part as a
project (SAVE > PROJECT), four slots.

**SOUNDS KEEP** writes notes only, so the sounds and mix you've built stay.

### The styles

The styles are built from what each genre or artist's music shares: tempo, groove, harmony and the kinds of sounds. No
riff, melody or progression from any record is in here, and on a 6-track FM-1 they're evocations, not copies. Their
names are Vallée's own; the "After" column says what each one leans toward.

| STYLE | After | What it writes |
|---|---|---|
| AETHER | Tame Impala | Psychedelic pop, 96–118 BPM: big laid-back drums, a melodic bass, phased synth pads, a hazy synth hook, a 16th arp, dorian/mixolydian harmony |
| LOTUS | Mac DeMarco | Slacker jazz-pop, 72–96 BPM: a lazy rimshot beat behind the grid, major sevenths and ii–V turns, a wobbly chorused keyboard (tape wobble), a jangling pick |
| ZETA | Daft Punk | French house, 118–126 BPM: four on the floor, offbeat filtered stabs ducked under the kick, an octave bass, a robot-voice riff |
| ORACLE | Radiohead | Frantic Kid A electronics (think "Idioteque"), 132–140 BPM: a stuttering drum machine carrying the track, snares and claps ratcheting x2–x4, sub hits under the kick, dark sustained pad chords a bar each (augmented and add9 colours), a chant-like vocal lead on one or two notes, Geiger-counter glitches |
| AKASHA | Brian Eno | Ambient, 60–78 BPM: no beat (ENERGY above half: a soft pulse), drones, cloud pads, bells and keys on loops of their own odd lengths (53 and 37 steps) drifting over each other |
| SIGIL | Steely Dan | The bright jazz-funk of "Peg", 124–132 BPM: a crisp 16th groove with a little lilt and ghosted snares, a popped octave bass, tine electric piano comping in syncopated 16ths, "mu" and 9sus chords over moving basses, stacked background voices holding the changes, a guitar answering at the bar ends |
| LEMURIA | yacht rock | The classic yacht-rock intro groove ("What a Fool Believes"), 116–124 BPM: a piano bouncing on syncopated eighths, landing each chord a 16th ahead of the bar, upper triads over a walking bass, 16th hats accented on the eighths, the kick dancing around the backbeat, a soulful lead and strings |
| ASTRAL | LCD Soundsystem | The slow, lovesick synth-pop of "Oh Baby", 86–100 BPM: a plain drum machine ticking, a pulsing eighth-note analog bass, big synth chords swelling a bar or two at a time, a crooned lead on long notes, a shimmering 16th arp; sweet and sad |
| HEX | the Strokes | Garage rock, 124–146 BPM: straight eighths, the bass on the roots, chords downstroked in eighths, a single-note guitar riff weaving through |
| OBELISK | motorik | Motorik (Neu!, Can, Stereolab), 120–138 BPM: the steady apache beat (kick on 1 and the "and" of 2, snare on 2 and 4, straight eighth hats), one- or two-chord drones, a locked eighth-note bass, a pulsing organ, "ba-ba" choir voices, an analog arp; nothing swings |
| SERAPH | Steve Winwood | Bright eighties blue-eyed soul in the vein of "Valerie", 116–124 BPM: a drum machine with a big gated snare, 16th hats, a synth bass pumping eighths, glossy polysynth chords punched and held, sus chords resolving, a soulful lead, a sparkling arp |
| WRAITH | Mk.gee | The warped, saturated eighties of Mk.gee's "Rockman", 88–100 BPM: a crushed drum machine pumping against everything else, a deep sparse bass, chorused and wobbling guitar-like chords, falsetto lines up high, open add9 / maj7 / sus harmony |

**VIBE** steers the harmony. DARK and HAZY use the minor-side progressions (HAZY adds extensions). WARM and BRIGHT use
the major side (BRIGHT simplifies to triads and may go lydian). WEIRD bends one chord a semitone or a tritone away.

**ENERGY** sets the tempo: from 40 % to 100 % it moves across the style's range (the BPM in its row above); under 40 % it slows further, down to 70 % of the style's slowest at 0. It also sets how busy the drums are, and the fills. **COMPLEX** adds ghost
notes, doubled hats, approach notes and richer chords. **HUMAN** moves velocities and nudges notes off the grid (laid
back for DEMARCO).

## THEME XENO

**HOME is a tape machine**, after the OP-1's tape (like the ORBIT and KARMA home screens). Two reels with turning
hexagram hubs wind the tape from left to right as the pattern plays. The selected track's creature stands on the head and
sings (pink eyes, a halo) on its notes. There's a tape counter and one meter per track. Below the deck, each track is a
lane of tape running past the head.

**Every sound has its own creature** (original pixel art, 12 x 12): each of the 141 factory presets gets one of its
own, and no two are alike. The engine decides the family, from the head, and every member carries the family's mark:

| Engine | Family |
|---|---|
| ANALOG | greys (a dome) |
| FM6 | pyramids with an eye |
| VOICE, WHEEL, NOISE | hooded figures: singers, organ priests, spectres |
| SAMPLE | seers and monks (a face) |
| TRIO, DRUM | tall craniums; skulls |
| LOFI | helmeted travellers with a visor |
| PHASE, DRUM-X | insects |
| GRAIN, PHYS | mantled things from the deep |

The sound decides the rest: what is on the head (antennae, horns, a halo, a wizard's hat, a crown, a flame, long ears,
eyes on stalks), the eyes (black almonds, two glowing, one great eye, a third eye, many, a band of light) and what is
below (a robe, tentacles, legs, wings, a ghost's tail, arms raised, a beard, an orb it floats on). A user preset gets
one from its name. They blink, sing on their track's notes, and sleep while muted.

**Turning PRESETS on HOME** (in any theme) opens a black box with a border in the track's colour: the new sound's
creature six times its size, the sound's name and its engine. It closes by itself about a second after the last
detent.

## THE WORLD

Tap **HOME** while on HOME (where Skiff showed the sound's wave) and you're in THE WORLD: an alien land at dusk where
the creatures of the sounds on your six tracks dance for the head from the opening, which looms huge and ghostly in
the sky over them. Tap HOME again for the tracks.

- **The creatures dance.** Each has its own move, and picks a new one every bar or two from the song: a hop, a sway,
  a shuffle, an orbit, a bow to the head, a leap, all on the beat. Each note its track plays makes it jump, and the
  louder the track the higher it bobs. A muted track's creature sleeps.
- **The head watches.** Its eyes follow whoever just played. Its third eye flashes on the beat and beams down on each
  creature as it plays. Every bar or two it summons one, lifting it up on a twin beam for a moment before letting it
  float back down. When the song gets loud its mouth glows: it's singing along. Behind it a ring of dots turns, pulsing
  on the beat.
- **The land**: the stars twinkle more the louder it gets, and the ground's grid runs toward you with the playhead, a
  ripple spreading over it on each beat.

And the music makes its own art on it, different every time:

- **Sigils.** Every note sends a glowing sigil up from the creature that played it: the sun, the moon, the eye in the
  pyramid, a star, an eye, an ankh, a cross or a spiral, chosen by the note's pitch. They drift to the head's third
  eye, which flares as it takes them in.
- **Constellations.** Notes leave stars in the sky (left to right by pitch, higher or lower by where they fall in the
  bar), each joined to the last star its track left. Over a song they build up into its own star map; the oldest
  fade, the last 32 stay.

With ANIM OFF (MENU) it's a still picture. The themes that already have their own scene there keep it: SKIFF's sea,
CRYPT's creatures, QUARK's detector.

## Room on the FM-1

The FM-1's firmware has to fit a fixed slot of 581,564 bytes. Skiff 2.6 already filled all but about 4.5 KB of it
(its released file: an image of about 577,000 bytes), and Vallée's additions need about 34 KB. So Vallée is built
without Skiff's SLICE engine and its two built-in drum loops (`FELUCCA_SLICE=0`, a build switch Skiff already had),
which frees about 50 KB: with everything (THE WORLD included) the image comes to roughly 565 KB, about 11-16 KB
under the limit (an estimate from
compiling Skiff 2.6 and Vallée the same way for a similar 32-bit chip, scaled by Skiff 2.6's real size; the build
itself measures the real one).

What that means for what you saved under Skiff:

- A song with a SLICE track loads; that track becomes an FM6 sound, its steps, level, pan and sends kept.
- A user preset made on SLICE loads as an FM6 sound, under its own name.
- Everything else (every other engine, projects, user presets, samples you uploaded for SAMPLE) is as it was.

`FELUCCA_SLICE=1 ./build.sh` builds SLICE back in, but then the image does not fit the FM-1: the build stops with
"image exceeds the app slot" and makes no firmware file. That check (tools/build.py) is what stands between a too-big
build and the unit, along with the RAM checks (92 KB of RAM, at least 8 KB of the pool spare).

## Safety

Nothing Vallée adds touches how the FM-1 is updated or recovered: the update loader, the hardware layer, the linker
script, the update and USB code, and the save formats are Skiff 2.6's, byte for byte. So Skiff's ways back all work
as they do for Skiff (README.md, "If the FM-1 does not start"), and one more is in the firmware itself: if it
crashes or hangs within 30 seconds of switching on twice in a row, the next start goes into the chip's own boot mode
(a USB disk named WL82 UBOOT1.00) where it can be written again. After a crash or a hang the intro is skipped.

What the tests run before a release (tests/run_tests.sh):

- Skiff's whole suite: sound, sequencer, UI, saves, MIDI, USB, the editor, the web pages, sanitizer and fuzzer runs.
- GEN: every style x part x vibe x seed written and checked (notes in key and on the chord, steps well formed).
- tests/vallee_stress.c under AddressSanitizer and UndefinedBehaviorSanitizer: GEN with random knobs (and values
  past the knobs' ends), its undo, the songs played; a "monkey" pressing random buttons, knobs and keys over the
  whole UI; the creatures for every engine and sound number.
- The SLICE conversions above, the creatures (all different), the creature box, the intro (its chime rings out,
  then HOME, silent).
- The browser version: the same C code as WebAssembly, booted, played, and run for half an hour of random input.

## Building, the web version, the web installer

The FM-1 firmware is built with JieLi's toolchain. The easiest way is GitHub Actions:

1. Put this source in a GitHub repo of your own.
2. Settings > Pages > Source: **GitHub Actions**.
3. Actions > **Vallée build** > Run workflow.

It builds the firmware, runs every test (host, UI, web, emulator), then makes the release
`vallee-2.6-vallee.fwsc` and the browser emulator, and publishes your site:

- the web installer, `/webapp/installer/` (Chrome or Edge, USB)
- the editor, `/webapp/editor/`
- the emulator, `/try/`

The package and a one-file emulator page (`vallee-web.html`) are kept as downloads on the run.

To build locally, see `BUILDING.md`:

```
./build.sh && sh tests/run_tests.sh        # the development build and the tests
./build.sh --release 2.6-vallee           # the package
EMU_PREVIEW=0 EMU_VERSION=v2.6-vallee web/emu/build_clang.sh   # the emulator: plain clang, no Emscripten
python3 web/emu/bundle_single.py          # .. as one HTML file
python3 web/make_site.py build/vallee-2.6-vallee.fwsc 2.6-vallee site
```

**Flash space.** The FM-1's app slot is 581,564 bytes ([Room on the FM-1](#room-on-the-fm-1)). If the image doesn't
fit, the build stops with "image exceeds the app slot" before anything is made, and nothing reaches your unit. The
build log's last lines give the image's size and the RAM's ("image N B; RAM .data+.bss N B of 94208; pool ...").

Installing is at your own risk, as with every FM-1 firmware. The installer can take the unit back to official V15.

**Source.** Vallée is GPL-3.0. If you share the firmware, share this source with it (your GitHub repo does that). The
ABOUT page still points at Skiff's repo: change `ui_menu.c`'s two lines to your own.

## Code

| File | What |
|---|---|
| `firmware/src/vgen.c` | GEN: the styles, harmony, voicing, the parts' writers, the sheet's knobs, cards and panel |
| `firmware/src/chime.c` | the opening chime |
| `firmware/src/ui_draw.c` `intro_frame` | the opening animation |
| `firmware/src/ui_xeno.c` | XENO's deck and lanes; the creature box |
| `firmware/src/ui_beast.c` | the creatures: their parts and how a sound picks them |
| `firmware/src/ui_world.c` | THE WORLD |
| `tests/ui_test.c` `test_gen`, `test_beasts`, `test_world` | GEN's tests; the creatures (all different) and the box; THE WORLD |
| `tests/vallee_stress.c` | the stress test: GEN fuzzed, a monkey on the whole UI, the creatures (ASan + UBSan) |
| `web/emu/emu_test.mjs` | the intro's test |

## Credits

Vallée is a fork of Skiff by illuminus5, built on Felucca by Leo Kuroshita (Hügelton Instruments), with the FM-1
community's work (README.md has the full credits). GEN's way of working (genre, seed, energy; OCT+ writes, OCT+ again
rolls) comes from JIANT FM's GENE by juanjiant-bit; Vallée's generator code and tables are its own. GPL-3.0. Not
affiliated with M-VAVE, or with any artist a style is named after.
