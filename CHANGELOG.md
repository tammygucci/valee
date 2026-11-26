# Changelog

## 1.0

A new sound. Every synth engine, factory sound and drum kit in Vallée is now
[SPAMPY](https://github.com/Spampy-Droid/SPAMPY-0.0-r949)'s, built on [SLOOP](https://github.com/isod89/sloop-fm1)'s
engines. Everything else (GEN, THE SPELL, ALEMBIC, SUMMON, the looks) stays.

**Install:** the [Vallée installer](https://tammygucci.github.io/valee/webapp/installer/) in Chrome or Edge. After the
firmware it writes the built-in samples (about 30 seconds more). It backs up your music first.

### New
- **Fifteen synth engines**: ANALOG, DIGITAL, PHASE, LOFI, SAMPLE, VOICE, TRIO, WHEEL, FM6, MORPH, PROPHET, CZ-1, SID,
  OBERVAVE and DRONE, with about two hundred factory sounds, all level-matched.
- **The DRUM engine has 16 sounds a kit and 35 kits**: five sampled (ACOUSTIC, DEEP, TIGHT, BRIGHT, DUST) and thirty
  synthesised (808, 909, LINNDRUM, TRAP, AMAPIANO, JAZZ and more). GEN's drums use the new crash, toms and percussion.
- **The FX layer's 27 effects** (SPAMPY's PUNCH-IN FX): loops, stutter, reverse, stop, rewind, freeze, filters, kills,
  echo, flanger and more, in time with the song. THE SPELL's effect keys use them too.
- **ALEMBIC 3: MEDIUM**: the whole mix played off a cassette, a tape machine, a VHS tape or a record, with WEAR and WOW.
- **The mix**: SPAMPY's look-ahead limiter and speaker EQ; LEVEL and PAN glide instead of stepping.

### Changed
- **Songs from 0.9 open**, but each track's sound moves to the nearest new engine (steps, levels, pans and sends
  kept). The old engines and sounds (DRUM-X, the MOOG/OBERHEIM/JUNO filters, the DEAD/TAPE/CRUSH kits) are gone.
- **Going to Skiff**: songs made on 1.0 won't sound right there. Save a complete backup first.


## 0.9.1 alpha

- **GEN: your own chords** (issue #1). GEN's new fifth page, CHORDS: pick a part and press keys, one chord a bar
  (white keys I–VII of the key, black keys the ♭ chords), up to four; GEN writes the parts over them, and the part is
  as many bars as its chords. Saved with the project; Skiff and 0.9 still open such projects.
- **GEN: parts add up into a song.** Write a verse, then a chorus: SEQ > SONG has both, in a song's order, each with
  patterns of its own. Writing a part again replaces it in place; OCT− undoes the whole build. A song you arranged
  yourself is left alone.

## 0.9 alpha

The first public test version. Firmware for the M-VAVE FM-1 that writes you a song, then lets you perform it. Built on [Skiff](https://github.com/illuminus5/skiff) 2.6 by illuminus5.

This is an alpha: it works on my FM-1 and passes the full test suite, but it hasn't been on many units yet. The installer backs up your music first, and you can always go back ([how](https://github.com/tammygucci/valee#going-back)). Please [report anything odd](https://github.com/tammygucci/valee/issues/new/choose).

**Install:** the [Vallée installer](https://tammygucci.github.io/valee/webapp/installer/) in Chrome or Edge, or the `.fwsc` file below.

### What's in it
- **GEN**: a song in one press, in twelve styles; PART SONG writes a whole arrangement.
- **THE SPELL**: the keyboard becomes the song, with live re-takes and REC ARRANGE.
- **ALEMBIC**: drive, motion, space and grain across the whole mix.
- **New sounds**: MOOG, OBERHEIM and JUNO filters for ANALOG, three drum kits, and SUMMON, an ambient engine that plays along in your song's chords.
- **Its own look**: the XENO and TEEN themes, and an opening intro.

### Fixed before release
- An AKASHA song from GEN could change its echo after you saved and reloaded it. The delay time now stays the same.

### Known issues
- Skiff can't open a project with a SUMMON track. Change those tracks or make a backup before going back to Skiff.
- The screen runs faster than Skiff's (20 MHz). If yours flickers or shows garbled pictures, please report it.
- A SLICE sound from your user slots loads as an FM6 sound (SLICE isn't in Vallée), and holding SAVE doesn't undo that.
- SLICE tracks in Skiff projects become FM6 sounds.

Credits: Skiff by illuminus5, Felucca by Leo Kuroshita (Hügelton Instruments) and the FM-1 community; GEN's way of working after GENE in JIANT FM by juanjiant-bit; SUMMON inspired by JIANT's SWARM (none of its code used). GPL-3.0.
