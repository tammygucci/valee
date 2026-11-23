# Changelog

## Next (in testing)

- **GEN: your own chords** (issue #1). GEN's new fifth page, CHORDS: pick a part and press keys, one chord a bar
  (white keys I–VII of the key, black keys the ♭ chords), up to four; GEN writes the parts over them, and the part is
  as many bars as its chords. Saved with the project; Skiff and 0.9 still open such projects.
- **GEN: parts add up into a song.** Write a verse, then a chorus: SEQ > SONG has both, in a song's order, each with
  patterns of its own. Writing a part again replaces it in place; OCT− undoes the whole build. A song you arranged
  yourself is left alone.
- Only the release build has to fit the FM-1 now; the developer build with the serial console just warns.

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
