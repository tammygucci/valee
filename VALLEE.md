# Vallée

Vallée is firmware for the M-VAVE FM-1, a fork of [Skiff](https://github.com/illuminus5/skiff) 2.6. Everything Skiff
does, Vallée does the same way: the pages, the sequencer, the sounds, projects and saves, with one exception: **the
SLICE engine is left out** (see [Room on the FM-1](#room-on-the-fm-1)). On top of that it adds:

- **An opening**: an animated intro with its own chime when you switch on.
- **THEME XENO**, and **TEEN** (the same screens in teenage engineering's colours): a tape-machine HOME screen, and a
  creature for every sound. Skiff's other themes are gone (~36 KB freed).
- **THE SPELL**: tap HOME on HOME: the song's map on top, and the Alchemist (the intro's head) conjuring with your six
  tracks' elements a figure the song summons, different for every song.
- **GEN**: a song generator in twelve styles, which writes ordinary steps you then edit in the sequencer.
- **Performing the song**: THE SPELL's keys are its parts, in on the bar with a transition; the next part, a new take
  of a part while it plays, and your arrangement recorded as you play it.
- **Its own sounds**: MOOG, OBERHEIM and JUNO filters and presets, and the kits DEAD, TAPE and CRUSH, which GEN's
  styles are voiced with.

Vallée's first start sets the look it is made for, once, on a new unit and over Skiff's settings alike: THEME XENO,
LARGE OFF (its big cards hide the deck and the creatures), ANIM ON, PLAYHEAD THEME, BPM LOCK ON (SELECT on HOME opens
GEN) and HINTS ON. Everything else you set under Skiff is kept, and the MENU can change any of these afterwards.

## Vintage synths: MOOG, OBERHEIM, JUNO

ANALOG has a **FILTER** of four kinds (EDIT > FILTER, on an ANALOG track: FILTER, then CUTOFF RES DRIVE again):

| FILTER | What it is |
|---|---|
| CLASSIC | ANALOG's own 12 dB low-pass, as always |
| MOOG | a 24 dB low-pass; the input driven warm into it, the low end giving way as RES rises |
| OBERHEIM | a 12 dB state-variable low-pass, its resonance gentle (it sings, it does not whistle), a little warmth after |
| JUNO | a 24 dB low-pass, clean, its resonance up to self-oscillation (with the CHORUS send up: the Juno's shimmer) |

WAVE SYNC keeps CLASSIC's filter. Twenty-one presets use them (PRESETS on an ANALOG track): MOOG BASS, MOOG LEAD, MOOG
SUB, MOOG FUNK, MOOG TAURUS, MOOG SWEEP, MOOG FLUTE; OBERHEIM, OBERHEIM PAD, OBERHEIM STR, OBERHEIM BASS, OBERHEIM
LEAD, OBERHEIM KEY; JUNO PAD, JUNO STRINGS, JUNO BASS, JUNO HOOVER, JUNO BRASS, JUNO KEYS, JUNO ARP, JUNO SWEEP. Voiced
after those instruments by ear; no circuit or sample of theirs is in here, and Vallée is not affiliated with Moog,
Oberheim or Roland.

## Kits: DEAD, TAPE, CRUSH

A kit for the DRUM engine: **KIT DEAD**, and the preset **DEAD DRUMS** (SAVE > PRESETS, or PRESETS on a DRUM
track). A seventies studio kit muted with towels: a short fat kick with no click, a deep snare whose body thumps and
whose wires stay low and dark, a short clap, hats taped down, a floor tom and (on the BELL lane) a rack tom, both dead,
and a woody cross-stick on RIM. Synthesized, like Skiff's other kits (no samples: there is no room for them, and the
sampled packs it is modelled on are not free to copy). EDIT > LANES and LANE MIX set each piece's level, tune, decay
and pan as on any kit.

Two more, put together from the model kits' pieces (as Skiff's DEEP, TIGHT and PERC are) and retuned:

- **KIT TAPE**, preset **TAPE KIT**: a warm seventies soul session with the towels off. DEAD's kick let ring, with a
  felt beater; a deep snare with its wires on; soft noise hats, the open one long; a ringing tom; the cross-stick; a
  ride cymbal on the BELL lane.
- **KIT CRUSH**, preset **CRUSH KIT**: a drum machine run too hot. A long kick driven hard, a snappy snare, eight
  claps, a bright metal hat, a short rim, a cowbell on BELL; the preset adds DIST CRUSH.

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
| 2 | PART (LOOP INTRO VERSE CHORUS BRIDGE OUTRO SONG) | KEY (AUTO, C … B) | TEMPO (AUTO, 60–180) | SOUNDS (NEW / KEEP) |
| 3 | DRUMS | BASS | KEYS | LEAD |
| 4 | COLOR | COMPLEX | SWING (AUTO, 0–100) | HUMAN |

DRUMS, BASS, KEYS, LEAD and COLOR are each OFF, LITE, ON or BUSY. OFF leaves that track exactly as it is.

GEN starts with **COMPLEX 0 and HUMAN 0** (COLOR ON, SWING on AUTO): the plainest version of each style. Turn them
up when you want more.

**The song's code.** The same knobs write the same song on every Vallée of this version, so a song can be passed on
as text. The panel shows it: the style, the vibe, the part and the seed (AETHER / HAZY SONG #27), and under them, in
the accent colour, only the knobs you set away from where GEN starts: `E80` ENERGY, `IN A` KEY, `120BPM` TEMPO,
`KEEP` SOUNDS, `DR3 BA1 KY0 LD2 CO0` the parts' amounts (0 OFF .. 3 BUSY), `X40` COMPLEX, `S20` SWING, `H30` HUMAN.
No third line: everything else is as GEN starts.

**Its sounds.** Each style picks every track's sound from three, the seed's pick, and the first of them, twice as
likely as each other, is Vallée's own where the style has one: the MOOG, OBERHEIM and JUNO presets (AETHER's Moog bass
and lead and Juno pad, ZETA's Moog funk bass and Oberheim stabs, ASTRAL's Juno pads, strings and arp ..) and its kits:
DEAD DRUMS (AETHER, LOTUS, HEX), TAPE KIT (LEMURIA, SIGIL), CRUSH KIT (WRAITH, ORACLE, ASTRAL).

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

**A whole song in one press.** Set PART to **SONG** and press OCT+. GEN writes all five parts (INTRO, VERSE, CHORUS,
BRIDGE, OUTRO), keeps each track's part as a pattern of its own (30 patterns: SAVE > PATTERNS needs that many free),
and makes every track's list the song: INTRO, VERSE, CHORUS, VERSE, CHORUS, BRIDGE, CHORUS, OUTRO, with SONG set to
ONCE. Press PLAY: it plays through once (32 bars) and stops at the top. OCT+ again writes the next seed's song in
place of it (its patterns given back first); OCT− puts the lists, the tracks and the patterns back as they were. A
track whose role is OFF keeps its own list and pattern. Edit any part afterwards on SEQ > SONG and NOTES.

The 64 patterns are shared by the whole unit, not per project: a new project doesn't free its old one's, and a saved
project keeps its own. Short of room, GEN first clears the patterns nothing uses any more (as SAVE > PATTERNS' CLEAR
UNUSED). If it still says PATTERNS FULL, your saved projects hold them, and the second line says which (SAVED SLOT 1:30 2:30:
slot 1 holds 30, slot 2 holds 30): delete one you don't need on SAVE > PROJECT (KNOB 2: DELETE), and GEN frees its
patterns. A generated song takes 30, so two saved songs fill most of the 64.

**Making a song part by part.** Keep the seed and the style, step through PART (INTRO, VERSE, CHORUS, BRIDGE, OUTRO), and
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

Vallée has two themes. **XENO** (the default): the esoteric one, the tape deck with its pentagram reels, a creature
for every sound, THE SPELL. **TEEN**: plain, in an OP-1's colours (black, white lines and text, TE blue for values, red
for the selected thing; the tracks red, blue, white, violet, green, orange). Its HOME is an OP-1-style tape machine: two
line-drawn reels with turning spokes, the tape round a row of rollers to the head, the counter, the track, PLAY and the
tempo between the reels; the tracks under it, their names big. No knob cards on top: the deck and the tracks take
their room; turn a knob and the cards come back while you turn it (the same knobs, the same settings, also on the
track's EDIT pages). No creatures; turning PRESETS pops up a box with the sound's name in big letters and its engine
under it. HOME on HOME has the same song map and
section keys, at the bottom, and above it, on black, a wire shape of a few thin lines that turns and bulges towards each track as it
plays, its lines flashing in that track's colour; it takes a new build at every PLAY and new directions at every part,
never the same twice. Pick one on MENU > DISPLAY > THEME.
Skiff's other six (SKIFF, HI-CON, PAPER, CRYPT, NEON, QUARK) and their widgets and pixel font are gone, which freed
about 36 KB of the FM-1's flash; a unit on one of them comes up in XENO. THEME CUSTOM (your own colours, from the web
editor) still works, over either.

**HOME is a tape machine**, after the OP-1's tape (like the ORBIT and KARMA home screens). Two reels with turning
hexagram hubs wind the tape from left to right as the pattern plays. The selected track's creature stands on the head and
sings (pink eyes, a halo) on its notes. There's a tape counter and one meter per track. Below the deck, each track is a
lane of tape running past the head.

**Every sound has its own creature** (original pixel art, 12 x 12), of its **track's family**, the same six
elements THE SPELL conjures with, each in its colour (XENO's track colours are these colours too):

| Track | Family | Bodies |
|---|---|---|
| 1 | green aliens | a grey's dome, a bulb in a suit, a cone head, an egg on tentacles |
| 2 | blue cryptids | bigfoot, mothman, a wendigo, a yeti |
| 3 | yellow psi orbs | a great orb, a ring, a small orb, one lit from above |
| 4 | pink crystals | a spire, a diamond, a cluster, a geode |
| 5 | purple sigils | the eye's triangle, a seal, an ankh, a chaos star |
| 6 | orange mythic herbs | a mandrake, a mushroom, a flower, a sprout |

The sound decides the rest: which body, its eyes, what's on or round it (antennae, ears, rays, sparks, facets..) and a
mark: 256 different creatures in each family, and sounds next to each other in a list never share one. A user preset
gets one from its name. They blink, sing on their track's notes, and sleep while muted.

**Turning PRESETS on HOME** opens a black box with a border in the track's colour: the new sound's
creature six times its size, the sound's name and its engine. It closes by itself about a second after the last
detent.

## THE SPELL

Tap **HOME** while on HOME and you're in THE SPELL. Tap HOME again for the tracks.

**At the top, the song's map** (where HOME's knob cards are): the song as SEQ > SONG has it, each part a block as long
as its bars, the same music in the same colour, named as GEN named them (INTRO, VERSE, CHORUS, BRIDGE, OUTRO) or A, B,
C... in the order they first come. The playhead runs across it, the part it's in lights up, and its name and the bar
(BAR 9 / 32) are above. With no lists it's one LOOP block.

**Play the song live from the keys.** On THE SPELL the keys don't play notes: each white key is one block of the song
on the map, in order, from the keyboard's first key on the left: F the first block, G the second, A the third, and on
up the keyboard (15 white keys, up to 15 blocks;
the last white key is the transition key, below). A GEN song's eight blocks are F INTRO, G VERSE, A CHORUS, B VERSE,
C CHORUS, D BRIDGE, E CHORUS, and the next F the OUTRO. Each block on the map shows its key's letter in its corner, and
the keys light up: the block playing lit, the others breathing. Press one and every track jumps to the start of that
block **on the next bar line**, all together, so it always lands on the 1 and stays in time: you can press it anywhere
in the bar before. A key is always its block: press the key of the block that is playing and that block starts again.
Until it comes in, the block blinks and the top line reads e.g. VERSE > CHORUS. Stopped, a key moves the song there
and PLAY starts from it. With SONG ON the song carries on from the block you jumped to; with SONG OFF each block loops
until you press another key.

**The transition key**: the keyboard's last white key (the top G, at the far right). Hold it and the effect set on
MENU › CONTROL › TRANSITION FX plays, just as a key of the FX layer does; let go and it stops. Any of the FX layer's
effects (REPEAT 1/8 1/16 1/32, REVERSE, LPF, HPF, TAPE STOP, FREEZE, OCT UP, OCT DN, GATE, DUB, FLANGE, VINYL);
REPEAT 1/16 to start with. Use it when you want it, e.g. held over the last beat before the chorus comes in. It
breathes while THE SPELL is up, lit while held, and the map says FX. Jumps themselves are always plain.

**The black keys** are the performance's controls, the same in every octave:

| Key | What it does |
|---|---|
| F# | drops a jump that hasn't come in yet |
| G# | the next part of the song (after the one coming, if one is) |
| A# | **a new take** of the part playing: GEN writes it again, live. The key, tempo, chords and sounds stay; the notes are new. Every bar but the one playing changes at once, that one once it has gone by, so it comes in on a bar line. The top line says e.g. CHORUS TAKE 2. Every time that part comes round it is the new take |
| C# | a new take of the selected track's part only (ALGORITHM picks the track): just the bass, just the drums .. |
| D# | **REC ARRANGE**: on (the map says REC and counts), play the parts in the order you want, D# again (or let the song stop): that order is the song now, up to 16 parts, SONG ONCE, from the top. SHIFT + D# puts the arrangement back as it was |

New takes are for a song GEN wrote in this session (after a restart, or once you edit its lists on SEQ > SONG, the
keys still play its parts; A# and C# say GEN SONGS ONLY). After REC ARRANGE the keys follow the new order: F its first
block, G its second ..

**Below, the Alchemist** (the head from the intro) conjures with your six tracks, each its own element:

| Track | Element |
|---|---|
| 1 | green aliens |
| 2 | blue cryptids |
| 3 | yellow psi orbs |
| 4 | pink crystals |
| 5 | purple sigils |
| 6 | orange mythic herbs |

When a track plays (a note on its step, not muted) one of its element leaves its jar on the shelf and arcs into the
summoning circle (ringed with candles), at most one a track a beat. The Alchemist's two hands float either side of
him like a puppet master's, fingers working the strings of what he summons; as each element lands, beams shoot from
their fingertips into the circle. The shelf counts what each track has given.

**The sky** behind him isn't empty: every so often something passes across it, one at a time and never the same
way twice (what it is, its height, direction, speed, colour and when the next one comes are all random): a UFO with
blinking lights (sometimes it shines its beam down), a comet trailing its tail, a shooting star, a pulsing plasma
being with waving tendrils, or a see-through ghost. Not with ANIM OFF.

**What the spell summons** rises in the circle as the song goes on, and is whole when the song ends. Its shape comes from
the six sounds you chose; each part of it takes the colour of an element, in the proportions the song used them; and
once a track has given enough of itself its trait appears: aliens give it eyes, cryptids horns, psi orbs a halo,
crystals a crown, sigils a rune ring, herbs vines. Other sounds, other parts, another mix: another figure. The same
song from the top: the same one. Every PLAY starts a new spell; stopped, the figure stays. With ANIM OFF nothing flies:
the elements go straight in.

## Room on the FM-1

The FM-1's firmware has to fit a fixed slot of 581,564 bytes. Skiff 2.6 already filled all but about 4.5 KB of it
(its released file: an image of about 577,000 bytes), and Vallée's additions need about 34 KB. So Vallée is built
without Skiff's SLICE engine and its two built-in drum loops (`FELUCCA_SLICE=0`, a build switch Skiff already had),
which frees about 50 KB: with everything (THE SPELL included) the image comes to roughly 565 KB, about 11-16 KB
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
(a USB disk named WL82 UBOOT1.00) where it can be written again. After a start that failed (a crash or hang in its first 30 seconds) the next start skips the intro.

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

**Identity.** A release answers **FM-1_56XY** (2.6: `FM-1_5626`), Vallée's own number, not Skiff's FM-1_7XY, so
The Firmware Hub can tell the two apart. Push a tag `v*` (e.g. `v2.6-vallee`) and the build also makes a GitHub
release with the package attached; `fmhub.json` in the root is the hub's card (its text, picture and links).

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

**Source.** Vallée is GPL-3.0. If you share the firmware, share this source with it (your GitHub repo does that).
The ABOUT page and its QR point at this repo (`ui_menu.c`); a fork of your own: change them to yours.

## Code

| File | What |
|---|---|
| `firmware/src/vgen.c` | GEN: the styles, harmony, voicing, the parts' writers, the sheet's knobs, cards and panel |
| `firmware/src/chime.c` | the opening chime |
| `firmware/src/ui_draw.c` `intro_frame` | the opening animation |
| `firmware/src/ui_xeno.c` | XENO's deck and lanes; the creature box |
| `firmware/src/ui_beast.c` | the creatures: their parts and how a sound picks them |
| `firmware/src/ui_world.c` | THE SPELL: the song's map, the Alchemist, the figure |
| `tests/ui_test.c` `test_gen`, `test_beasts`, `test_world` | GEN's tests; the creatures (all different) and the box; THE SPELL |
| `tests/vallee_stress.c` | the stress test: GEN fuzzed, a monkey on the whole UI, the creatures (ASan + UBSan) |
| `web/emu/emu_test.mjs` | the intro's test |

## Credits

Vallée is a fork of Skiff by illuminus5, built on Felucca by Leo Kuroshita (Hügelton Instruments), with the FM-1
community's work (README.md has the full credits). GEN's way of working (genre, seed, energy; OCT+ writes, OCT+ again
rolls) comes from JIANT FM's GENE by juanjiant-bit; Vallée's generator code and tables are its own. GPL-3.0. Not
affiliated with M-VAVE, or with any artist a style is named after.
