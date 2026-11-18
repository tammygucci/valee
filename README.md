<p align="center">
  <img src="web/brand/logo.png" alt="Vallée" width="300">
</p>

<h1 align="center">Vallée</h1>

<p align="center"><b>Firmware for the M-VAVE FM-1 that writes you a song, then lets you perform it.</b><br>
<i>receiving signal from the other side</i></p>

<p align="center">
  <a href="https://tammygucci.github.io/valee/webapp/installer/"><b>Install</b></a> ·
  <a href="https://tammygucci.github.io/valee/try/"><b>Try it in your browser</b></a> ·
  <a href="https://tammygucci.github.io/valee/webapp/editor/">Editor</a> ·
  <a href="https://tammygucci.github.io/valee/webapp/installer/brand/vallee_cheatsheet.pdf">Cheat sheet</a> ·
  <a href="VALLEE.md">Full manual</a>
</p>

<p align="center">
  <a href="https://tammygucci.github.io/valee/webapp/installer/brand/vallee_demo.mp4"><img src="web/brand/poster.jpg" alt="Watch the demo" width="420"></a>
  <a href="https://tammygucci.github.io/valee/webapp/installer/brand/vallee_perform.mp4"><img src="web/brand/poster_perform.jpg" alt="Watch a performance" width="420"></a>
  <br><a href="https://tammygucci.github.io/valee/webapp/installer/brand/vallee_summon.mp4"><img src="web/brand/poster_summon.jpg" alt="Watch SUMMON" width="420"></a>
  <br><sub>The demo, a live performance and SUMMON (click to play)</sub>
</p>

> **This is an alpha (0.9).** Everything here works on my own FM-1 and passes the full test suite, but it hasn't been
> on many units yet. Back up anything you care about first (the installer does this for you), expect a few rough
> edges, and please [report what you find](https://github.com/tammygucci/valee/issues/new/choose). You can always
> go back: see [Going back](#going-back) below.

Vallée is built on [Skiff](https://github.com/illuminus5/skiff) 2.6 by illuminus5, a six-track groovebox firmware for
the FM-1. Skiff's engines, sequencer, editor and saves all work the same way here. On top of that, Vallée adds a song
generator, a way to play a whole song live from the keys, an effects chain across the whole mix, new synth sounds and
drum kits, and its own look.

## Going back

You can leave Vallée at any time, and your music comes with you.

- **To the official M-VAVE firmware:** open the [Vallée installer](https://tammygucci.github.io/valee/webapp/installer/)
  in Chrome or Edge, connect the FM-1 by USB and choose the official firmware. It saves a full backup of your music
  first. Your FM-1 goes back to the version it had before (V15 or V16), and the page tells you which one it needs.
  You can also use M-VAVE's own updater. Afterwards, pair any Bluetooth devices again.
- **To Skiff:** install Skiff with the [Skiff installer](https://illuminus5.github.io/skiff/webapp/installer/), the
  same way. Your projects, sounds and samples carry over, and Skiff simply ignores ALEMBIC. **One exception:** Skiff
  has no SUMMON, so it won't open a project with a SUMMON track (or a sound saved from SUMMON). Before you switch,
  change those tracks to another sound and save, or first use **Save backup** (Complete backup) in the [Vallée editor](https://tammygucci.github.io/valee/webapp/editor/). Nothing is erased: the
  projects open again if you come back to Vallée.

If the FM-1 won't start at all, see [If the FM-1 won't start](#install) below.

## What Vallée adds

### GEN: a song in one press

Pick a style, a vibe and a seed, and press OCT+. GEN writes all six tracks (bass, chords, lead, drums, colour and
percussion) in one of **twelve styles**: AETHER, LOTUS, ZETA, ORACLE, AKASHA, SIGIL, LEMURIA, ASTRAL, HEX, OBELISK,
SERAPH and WRAITH. Set PART to SONG and it writes a whole arrangement: intro, verses, choruses, a bridge and an outro.
Everything it writes is ordinary steps and sounds, so you can edit all of it afterwards. The same settings always
give the same song, so you can share a song as a short code.

### THE SPELL: play the song live

<img src="web/brand/xeno_spell.png" alt="THE SPELL" width="240" align="right">

Tap HOME on HOME. The song's map runs across the top, and the keyboard becomes the song. Each white key is a section
of the song, and pressing one jumps every track to that section on the next bar line, so you're always in time. The
spare white keys are effects you hold: repeats, tape stop, filter sweeps, freeze and more. The black keys give you
the next part, **a new take** of the part that's playing (GEN rewrites it live), a new take of one track, and
**REC ARRANGE**, which records the order you play the parts in as the new song.

Below the map, the Alchemist pulls an element from each of your six tracks into a summoning circle as they play.
By the end of the song he has built a figure that comes from your sounds and your mix, different for every song.

<br clear="right">

### ALEMBIC: the whole mix through four vessels

A chain on the master mix: **DRIVE** (soft, hard, fold, fuzz), **MOTION** (phaser, flanger, chorus, wobble),
**SPACE** (echo, reverb or both) and **GRAIN** (tape wow, dust, crush, filter). Each stage has a BLEND knob, and
BLEND 0 switches that stage off completely. On screen they're four flasks, filled as far as each BLEND and moving
with the sound.

### New sounds

- **MOOG, OBERHEIM and JUNO filters** for the ANALOG engine, with 21 presets voiced after those instruments.
- **Three drum kits**: DEAD (a seventies studio kit damped with towels), TAPE (warm soul session) and CRUSH (a drum
  machine run too hot).
- **SUMMON**, an ambient engine. Press any key on its EDIT page and it plays its own part in the song's chords, never against them, growing
  fuller over the song while candles light and a being forms, a different one for every key and every time (SUMMONER
  on XENO, AURA on TEEN). One in fifty is a demon; one in fifty an angel. Another key fades it out.

### Its own look

<p>
  <img src="web/brand/xeno_home.png" alt="XENO's HOME" width="240">
  <img src="web/brand/teen_home.png" alt="TEEN's HOME" width="240">
  <img src="web/brand/teen_scope.png" alt="TEEN's SPELL" width="240">
</p>

- **XENO**, the default: a tape-machine HOME with pentagram reels, and an original pixel creature for every sound
  (aliens, cryptids, psi orbs, crystals, sigils and mythic herbs, one family per track) that sings on its notes.
- **TEEN**: the same screens kept plain and minimal, in a classic portable synth's colours.
- **An opening**: a 6.8-second intro drawn after the "consciousness energy grid" of the declassified Gateway
  Process report, with its own binaural chime.

## Install

At your own risk, as with any FM-1 firmware. You can always go back.

1. Open the **[Vallée installer](https://tammygucci.github.io/valee/webapp/installer/)** in Chrome or Edge on a
   computer.
2. Connect the FM-1 by USB (straight into the computer, not through a hub) and follow the page.

The same page can take the FM-1 back to the official M-VAVE firmware. Your Skiff projects, presets and samples carry
over. The only exception is that a SLICE track or preset becomes an FM6 sound, because Vallée leaves SLICE out to fit
in the FM-1's memory ([why](VALLEE.md#room-on-the-fm-1)).

**If the FM-1 won't start:** Vallée leaves Skiff's update loader, recovery and save formats untouched, so
[Skiff's recovery steps](SKIFF.md#if-the-fm-1-does-not-start) all apply (use Vallée's installer where they say
Skiff's). Vallée also has its own fallback: if it crashes twice in a row within 30 seconds of switching on, the next
start goes into the chip's boot mode, where it can be written again.

## Known issues

- **Skiff can't open a project with a SUMMON track** (see [Going back](#going-back)).
- **The screen runs faster than Skiff's** (20 MHz instead of 12) so pages draw without tearing. Most units are fine
  with it; if yours shows flickering, stray lines or garbled pictures, please open an issue.
- **A SLICE preset loaded from your user slots becomes FM6's first sound**, and holding SAVE does not undo that load.
  Don't save over that slot if you want to keep the SLICE sound for Skiff later.
- **GEN SONG, then a single PART written while the song plays:** the song's patterns stay in the pool, unused. GEN
  clears them by itself the next time it needs room (or SAVE > PATTERNS > CLEAR UNUSED).

## Learn it

- **[VALLEE.md](VALLEE.md)**: everything Vallée adds, in full.
- **[SKIFF.md](SKIFF.md)**: Skiff's manual, for everything Vallée shares with Skiff (the sequencer, engines, MIDI and
  menus).
- **[The cheat sheet](https://tammygucci.github.io/valee/webapp/installer/brand/vallee_cheatsheet.pdf)**: one page
  to keep next to the unit.

## Build it

GitHub Actions builds the firmware, runs every test and publishes the installer, editor and emulator. To build
locally, see [BUILDING.md](BUILDING.md) and
[VALLEE.md](VALLEE.md#building-the-web-version-the-web-installer).

## Credits

Vallée is a fork of **[Skiff](https://github.com/illuminus5/skiff)** by illuminus5, which is built on
**[Felucca](https://github.com/hugelton/Felucca)** by Leo Kuroshita (Hügelton Instruments), with the work of the
FM-1 community ([full credits](SKIFF.md#credits)). GEN's way of working (genre, seed, energy) comes from
**GENE** in [JIANT FM](https://github.com/juanjiant-bit/JIANT) by juanjiant-bit, while Vallée's generator code is
its own. SUMMON was inspired by the idea of **SWARM** in JIANT; none of its code is used. If Vallée is useful to you, please [support Felucca's author](https://github.com/sponsors/hugelton).

## Licence

Free software under [GPL-3.0-only](LICENSE), as Skiff and Felucca are. If you share the firmware, share this source
with it. See [LICENSING.md](LICENSING.md) for the bundled fonts and DSP.

M-VAVE and FM-1 are trademarks of their owners. Vallée is not affiliated with or endorsed by M-VAVE, Skiff's or
Felucca's authors, Moog, Oberheim or Roland, or any artist a style leans toward.
