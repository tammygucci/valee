/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) GEN: the song generator, after JIANT FM's GENE (juanjiant-bit/JIANT, GPL-3.0): a style, a seed, a vibe, an
 * energy and the rest, and OCT+ writes a part of a song in that style into the tracks, as ordinary steps and sounds the
 * sequencer then edits like any other. The code and every pattern table here are Vallée's own: the styles are drawn
 * from what their genres share (tempo, groove, harmony, the kind of sounds), never a riff, a melody or a progression
 * of any one record.
 *
 * Where: HOME, then SELECT (HOME has no pages of its own; SELECT turned on HOME opens GEN, turned back past GEN's first
 * window it goes HOME). A sheet (ui.c sheet_t; ui_input.c SHEETS): four windows of four knobs, SELECT moving them
 *     STYLE SEED VIBE ENERGY  /  PART KEY TEMPO SOUNDS  /  DRUMS BASS KEYS LEAD  /  COLOR COMPLEX SWING HUMAN
 * PRESETS turns STYLE. OCT+ writes; OCT+ again with nothing turned: the next seed; a knob turned, then OCT+: the same
 * piece with that changed (each part of the music has its own random stream, so DRUMS turned keeps the bass).
 * OCT- puts back what the tracks held before GEN wrote (the undo copies GEN took, while nothing else has been undone or
 * copied since), else goes HOME. Leaving the page keeps what was written; SAVE held undoes it track by track ("GEN").
 *
 * The tracks: T1 BASS, T2 KEYS (chords), T3 LEAD, T4 DRUMS, T5 COLOR (an arp, a pad, a texture), T6 PERC (a second kit).
 * A part set OFF leaves its track as it is. SOUNDS KEEP writes the notes only (the tracks' sounds and mix stay).
 * PART: the same seed makes the same key, tempo, sounds and hook; LOOP INTRO VERSE CHORUS BRIDGE OUTRO each its own
 * four bars (64 steps of 1/16; ENO's loops their own lengths), to chain on SEQ > SONG into a whole song. */

enum { GS_IMPALA, GS_DEMARCO, GS_DAFT, GS_HEAD, GS_ENO, GS_STEELY, GS_YACHT, GS_LCD, GS_STROKE, GS_SLAB, GS_WINWOOD, GS_GEE, GS_N };
/* (the names: our own; after, in order: Tame Impala, Mac DeMarco, Daft Punk, Radiohead's Kid A, Brian Eno, Steely
 * Dan's "Peg", the Doobie Brothers' "What a Fool Believes", LCD Soundsystem's "Oh Baby", the Strokes, motorik (Neu!,
 * Can, Stereolab), Steve Winwood's "Valerie", Mk.gee's "Rockman") */
static const char *const GEN_STYLE[GS_N] = {"AETHER", "LOTUS", "ZETA", "ORACLE", "AKASHA", "SIGIL", "LEMURIA",
                                            "ASTRAL", "HEX", "OBELISK", "SERAPH", "WRAITH"};
enum { GV_DARK, GV_HAZY, GV_WARM, GV_BRIGHT, GV_WEIRD, GV_N };
static const char *const GEN_VIBE[GV_N] = {"DARK", "HAZY", "WARM", "BRIGHT", "WEIRD"};
enum { GP_LOOP, GP_INTRO, GP_VERSE, GP_CHORUS, GP_BRIDGE, GP_OUTRO, GP_SONG, GP_N };   /* ((Vallée) SONG: gen_song */
static const char *const GEN_PART[GP_N] = {"LOOP", "INTRO", "VERSE", "CHORUS", "BRIDGE", "OUTRO", "SONG"};
static const char *const GEN_AMT[4] = {"OFF", "LITE", "ON", "BUSY"};
static const char *const GEN_NOTE[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
enum { GR_BASS, GR_KEYS, GR_LEAD, GR_DRUMS, GR_COLOR, GR_PERC, GR_NROLE };   /* the tracks, T1 .. T6 */

/* ---------------------------------------------------------------- the knobs --- */
enum { GK_STYLE, GK_SEED, GK_VIBE, GK_ENERGY, GK_PART, GK_KEY, GK_TEMPO, GK_SOUNDS, GK_DRUMS, GK_BASS, GK_KEYS,
       GK_LEAD, GK_COLOR, GK_COMPLEX, GK_SWING, GK_HUMAN, GK_CHORDS, GK_BAR, GK_TYPE, GK_N };
#define GEN_NWIN 5u                                     /* ((Vallée) the fifth: CHORDS, a part's own (gch) */
static struct {
    uint8_t v[GK_N];             /* the knobs' values (TEMPO 0: AUTO, else 40 + its BPM - 60; SWING 0: AUTO, else 1 + %) */
    uint16_t seed;               /* 1 .. 999 */
    uint8_t win;                 /* the window SELECT shows */
    uint8_t made;                /* a piece written since GEN opened (OCT+ again: the next seed, nothing turned) */
    uint8_t dirty;               /* a knob turned since */
    uint8_t nseq;                /* the undo copies GEN took (useq), the newest */
    uint16_t useq[NTRK];
    int16_t g_was[8];            /* the globals as they were (OCT-: back) */
    uint8_t song;                /* (Vallée) the last write a SONG (gen_song): its patterns gen_sid, the lists gen_lwas */
} gen = {{0, 0, GV_HAZY, 60, GP_LOOP, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0}, 1, 0, 0, 0, 0, {0}, {0}, 0};
#define GC_NT 12u                                       /* (CHORDS' TYPE: END, AUTO, then GC_Q's) */
static const uint8_t GEN_MAX[GK_N] = {GS_N - 1, 0, GV_N - 1, 100, GP_N - 1, 12, 121, 1, 3, 3, 3, 3, 3, 100, 101, 100,
                                      1, 3, GC_NT};
static const uint8_t GEN_DEF[GK_N] = {0, 0, GV_HAZY, 60, GP_LOOP, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0};
/* ((Vallée) MINE: a part's own chords, a bar each, the part as many bars as it has (gen_chords.h) */
#include "gen_chords.h"
_Static_assert(GC_NPART == GP_OUTRO + 1u, "gch: a part's own chords for each part but SONG");
static void gen_reset(void)                            /* the knobs as at power-on (TEMPO, KEY, SWING: AUTO; (Vallée)
                                                        * COMPLEX 0, HUMAN 0: turned up only by hand) */
{
    memcpy(gen.v, GEN_DEF, sizeof gen.v);
    gen.seed = 1;
    gen.made = gen.dirty = gen.nseq = 0;
}
static const uint8_t GEN_G[8] = {G_BPM, G_SWING, G_RTYPE, G_RSIZE, G_DTIME, G_DFDBK, G_DMIX, G_CMODE};

/* ------------------------------------------------------------ the randomness --- */
typedef struct { uint32_t s; } grng_t;
static uint32_t g_next(grng_t *r)
{
    uint32_t x = r->s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return r->s = x ? x : 0x6D2B79F5u;
}
static uint32_t g_n(grng_t *r, uint32_t n) { return n ? g_next(r) % n : 0u; }
static int g_pct(grng_t *r, int32_t p) { return (int32_t)g_n(r, 100) < p; }
/* a stream of its own for each part of the music: the seed, the style and what it is for (id); part: the PART's own;
 * ((Vallée) g_vary: a part's take, THE SPELL's re-roll (gen_reroll): 0 the first, as before; only the parts' writers
 * see it, not the plan (key, tempo, chords) nor the sounds) */
static uint8_t g_vary;
static grng_t g_stream(uint32_t id, uint32_t part)
{
    grng_t r;
    uint32_t i;
    r.s = (uint32_t)gen.seed * 2654435761u ^ (uint32_t)gen.v[GK_STYLE] * 0x9E3779B9u ^ id * 0x85EBCA6Bu ^
          part * 0xC2B2AE35u ^ (uint32_t)g_vary * 0x27D4EB2Fu;
    for (i = 0; i < 4u; i++)
        g_next(&r);
    return r;
}

/* ------------------------------------------------------------- the harmony --- */
enum { Q_MAJ, Q_MIN, Q_MAJ7, Q_MIN7, Q_DOM7, Q_MAJ9, Q_MIN9, Q_DOM9, Q_SUS2, Q_SUS4, Q_ADD9, Q_MADD9, Q_M7B5, Q_POW,
       Q_MIN11, Q_DOM13, Q_9SUS4, Q_MAJ7S11, Q_MU, Q_MIN6, Q_7S9, Q_AUG, Q_DIM, Q_N };
static const struct { uint8_t n, iv[5]; const char *sfx; } CQ[Q_N] = {
    {3, {0, 4, 7}, ""},           {3, {0, 3, 7}, "m"},           {4, {0, 4, 7, 11}, "maj7"},
    {4, {0, 3, 7, 10}, "m7"},     {4, {0, 4, 7, 10}, "7"},       {5, {0, 4, 7, 11, 14}, "maj9"},
    {5, {0, 3, 7, 10, 14}, "m9"}, {5, {0, 4, 7, 10, 14}, "9"},   {3, {0, 2, 7}, "sus2"},
    {3, {0, 5, 7}, "sus4"},       {4, {0, 4, 7, 14}, "add9"},    {4, {0, 3, 7, 14}, "madd9"},
    {4, {0, 3, 6, 10}, "m7b5"},   {3, {0, 7, 12}, "5"},          {5, {0, 3, 7, 10, 17}, "m11"},
    {5, {0, 4, 10, 14, 21}, "13"}, {5, {0, 5, 7, 10, 14}, "9sus"}, {5, {0, 4, 7, 11, 18}, "maj7#11"},
    {4, {0, 2, 4, 7}, "add2"},    {4, {0, 3, 7, 9}, "m6"},       {5, {0, 4, 7, 10, 15}, "7#9"},
    {3, {0, 4, 8}, "+"},          {3, {0, 3, 6}, "dim"},
};
/* a progression: n chords over the four bars (4: a bar each, 8: two), each {root above the key, quality, the bass:
 * semitones above the root it plays (0: the root; a slash chord, F/G: root 5, bass 2)} */
typedef struct { uint8_t n, c[8][3]; } gprog_t;
#define P4(a, b, c, d) {4, {a, b, c, d}}
#define P8(a, b, c, d, e, f, g, h) {8, {a, b, c, d, e, f, g, h}}
#define C_(r, q) {r, q, 0}
#define CS_(r, q, b) {r, q, b}                         /* (a slash chord: over the note b above its root) */

/* ------------------------------------------------- the styles' vocabulary --- */
/* Drums: a lane a string of 16, BD SD CP CH OH TM RS CB: x a hit, a accented, o sometimes (more with ENERGY), g a ghost
 * (with COMPLEX); 0: the lane silent. Bass: R root, 5 fifth, O octave, C a chord tone, A an approach to the next chord,
 * - held, . rest. Comping: X the chord, held on; s a short stab; - held; . rest. Lead: n a note, - held, . rest (two
 * bars). */
typedef struct { const char *ln[8]; } gdrum_t;
typedef struct { uint8_t eng; const char *pre; } gsnd_t;
typedef struct {
    uint8_t bpm_lo, bpm_hi, swing, ext, human_late;
    uint8_t keys[4];
    uint8_t mode_dark, mode_light;          /* N_SCALE: 2 MIN, 3 DOR, 1 MAJ, 4 MIX, 9 LYD, 8 PHRY */
    const gprog_t *dark, *light;
    uint8_t ndark, nlight;
    const gdrum_t *drum;                    /* [0] the verse's, [1] the chorus's */
    const gdrum_t *perc;                    /* T6's */
    const char *const *bass;                /* 3 bars of 16 */
    const char *const *comp;                /* 3 bars of 16 */
    const char *const *lead;                /* 3 two-bar rhythms of 32 */
    uint8_t color;                          /* XC_*: what T5 plays */
    uint8_t lead_lo, lead_hi;               /* the lead's range (MIDI) */
    gsnd_t snd[GR_NROLE][3];                /* the sounds a role picks from (a 0 preset name: fewer) */
    uint8_t rtype, rsize, dtime, dfdbk, dmix, cmode;   /* the buses */
    uint8_t rev[GR_NROLE], dly[GR_NROLE];   /* the sends */
    uint8_t ins_keys;                       /* T2's INSERT (N_ITYPE: 6 PHASR, 7 FLANG, 8 CHOR), 0 none */
    uint8_t wobble;                         /* tape wobble: the LFO on PITCH of KEYS and LEAD */
    uint8_t duck;                           /* BASS KEYS COLOR ducked under the drums */
} gstyle_t;
enum { XC_ARP16, XC_JANGLE, XC_PAD, XC_ROBOT, XC_TEXTURE, XC_FILLS, XC_BELLS };
/* ((Vallée, the SPAMPY sound) SPAMPY's numbers (core.h); GRAIN and PHYS are gone: MORPH and FM6 take their roles until
 * the styles name SPAMPY's sounds (a name not found: the engine's first sound, g_preset) */
enum { E_ANALOG = 0, E_DIGITAL = 1, E_PHASE = 2, E_LOFI = 3, E_SAMPLE = 4, E_VOICE = 5, E_TRIO = 6, E_WHEEL = ENGI_DRAWBAR,
       E_MORPH = ENGI_NOISE, E_DRUM = ENGI_DRUM, E_FM6 = ENGI_FM6, E_OBV = ENGI_OBERVAVE, E_DRONE = ENGI_DRONE };

/* IMPALA: psychedelic pop: big laid-back drums, melodic bass, phased synth pads, a hazy synth hook, a 16th arp */
static const gprog_t IMP_D[] = {
    P4(C_(0, Q_MIN7), C_(5, Q_DOM7), C_(3, Q_MAJ7), C_(10, Q_MAJ)),
    P4(C_(0, Q_MIN9), C_(8, Q_MAJ7), C_(3, Q_MAJ), C_(10, Q_ADD9)),
    P4(C_(0, Q_MIN), C_(10, Q_MAJ), C_(8, Q_MAJ), C_(7, Q_DOM7)),
    P8(C_(0, Q_MIN7), C_(0, Q_MIN7), C_(5, Q_MIN7), C_(5, Q_MIN7), C_(3, Q_MAJ7), C_(3, Q_MAJ7), C_(2, Q_M7B5),
       C_(7, Q_DOM7))};
static const gprog_t IMP_L[] = {
    P4(C_(0, Q_MAJ7), C_(4, Q_MIN7), C_(9, Q_MIN7), C_(5, Q_MAJ7)),
    P4(C_(0, Q_MAJ), C_(10, Q_MAJ), C_(5, Q_MAJ), C_(0, Q_MAJ)),
    P4(C_(5, Q_MAJ7), C_(4, Q_MIN7), C_(2, Q_MIN7), C_(7, Q_9SUS4)),
    P4(C_(0, Q_ADD9), C_(3, Q_MAJ), C_(5, Q_MAJ), C_(5, Q_MIN))};
static const gdrum_t IMP_DR[2] = {
    {{"x.....x...x.....", "....a.......a...", 0, "x.x.x.x.x.x.x.x.", "..............o.", 0, 0, 0}},
    {{"x.....x.x.x...o.", "....a.......a..g", 0, "x.xox.xox.xox.xo", "......o.......x.", 0, 0, 0}}};
static const gdrum_t IMP_PC = {{0, 0, "....o.......x...", "xoxoxoxoxoxoxoxo", 0, "......o.....o...", 0, 0}};
static const char *const IMP_B[3] = {"R--.R.5-O-.CR-A-", "R-.R--C.5--.O-A.", "R--R.-C.5.-OC-A."};
static const char *const IMP_C[3] = {"X---------------", "X-------X-------", "X-------.X------"};
static const char *const IMP_L_[3] = {"..n-n.n---..n.n-n---n.n-n-----..", "n--.n.n-n---....n--.n.n-n-n---..",
                                      "...n-n-n--n-....n-n-n--.n-n-----"};
/* DEMARCO: slacker jazz-pop: lazy rimshot beat behind the grid, major sevenths, a wobbly chorused guitar, jangling */
static const gprog_t DEM_D[] = {
    P4(C_(0, Q_MIN7), C_(5, Q_DOM9), C_(10, Q_MAJ7), C_(3, Q_MAJ7)),
    P4(C_(0, Q_MIN9), C_(8, Q_MAJ7), C_(2, Q_M7B5), C_(7, Q_DOM7)),
    P4(C_(2, Q_MIN7), C_(7, Q_DOM7), C_(0, Q_MAJ7), C_(9, Q_DOM7))};
static const gprog_t DEM_L[] = {
    P4(C_(0, Q_MAJ7), C_(9, Q_MIN7), C_(2, Q_MIN7), C_(7, Q_DOM7)),
    P4(C_(5, Q_MAJ7), C_(5, Q_MIN6), C_(0, Q_MAJ7), C_(9, Q_MIN7)),
    P4(C_(0, Q_MAJ7), C_(4, Q_MIN7), C_(5, Q_MAJ7), C_(7, Q_DOM9)),
    P8(C_(0, Q_MAJ7), C_(0, Q_MAJ7), C_(11, Q_M7B5), C_(4, Q_DOM7), C_(9, Q_MIN7), C_(8, Q_MAJ7), C_(2, Q_MIN7),
       C_(7, Q_DOM7))};
static const gdrum_t DEM_DR[2] = {
    {{"x.......x.o.....", 0, 0, "x.x.x.x.x.x.x.x.", 0, 0, "....x.......x...", 0}},
    {{"x.....o.x.o.....", "....x.......x...", 0, "x.x.x.x.x.x.x.xo", "..............o.", 0, 0, 0}}};
static const gdrum_t DEM_PC = {{0, 0, 0, "x.o.x.o.x.o.x.o.", 0, 0, 0, "..............o."}};
static const char *const DEM_B[3] = {"R---5---C--.A---", "R--5--R-C---5-A-", "R---R-5-O---C-A-"};
static const char *const DEM_C[3] = {"X-----X-.X------", "X---.X--X---.X--", "X-------X--.X---"};
static const char *const DEM_L_[3] = {"n---n-n-----....n-n---n-------..", "..n-n-n-n---....n---n-n-n-----..",
                                      "n-----n-n-n-....n-----n-n-------"};
/* DAFT: French house: four on the floor, filtered disco stabs ducked under the kick, octave bass, a robot voice */
static const gprog_t DAF_D[] = {
    P4(C_(0, Q_MIN7), C_(5, Q_MIN7), C_(10, Q_DOM7), C_(3, Q_MAJ7)),
    P4(C_(0, Q_MIN9), C_(0, Q_MIN9), C_(8, Q_MAJ7), C_(10, Q_DOM9)),
    P4(C_(0, Q_MIN7), C_(3, Q_MAJ7), C_(10, Q_MAJ), C_(8, Q_MAJ7))};
static const gprog_t DAF_L[] = {
    P4(C_(0, Q_MAJ7), C_(9, Q_MIN7), C_(4, Q_MIN7), C_(5, Q_MAJ7)),
    P4(C_(5, Q_MAJ7), C_(7, Q_DOM7), C_(4, Q_MIN7), C_(9, Q_MIN7)),
    P4(C_(0, Q_MAJ9), C_(0, Q_MAJ9), C_(5, Q_MAJ9), C_(5, Q_MAJ9))};
static const gdrum_t DAF_DR[2] = {
    {{"a...x...a...x...", 0, "....x.......x...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", 0, 0, 0}},
    {{"a...x...a...x...", "............o...", "....a.......a...", "xxxxxxxxxxxxxxxx", "..x...x...x...x.", 0, 0, 0}}};
static const gdrum_t DAF_PC = {{0, 0, 0, "x.xxx.xxx.xxx.xx", 0, 0, "...o.....o......", 0}};
static const char *const DAF_B[3] = {"R..R..R.O..R..A.", "R.O.R.O.R.O.R.OA", "R..RO..RR..O5.A."};
static const char *const DAF_C[3] = {"..s...s...s...s.", "s.s..s.s..s.s.s.", "..s..s....s..s.."};
static const char *const DAF_L_[3] = {"n.n.n...n.n.n...n.n.n...n.n.n-..", "..n-..n...n-..n...n-..n...n-..n.",
                                      "n..n..n.n..n..n.n..n..n.n..n----"};
/* HEAD: cold, frantic electronics in the Kid A vein: a stuttering drum machine carrying the track at 132-140, sub
 * hits under the kick, sustained ambiguous pad chords a bar each, a chant-like lead on one or two notes, glitches */
static const gprog_t HEA_D[] = {
    P4(C_(0, Q_MIN6), C_(8, Q_AUG), C_(5, Q_MADD9), C_(1, Q_MAJ7)),
    P4(C_(0, Q_MADD9), C_(3, Q_AUG), C_(10, Q_SUS2), C_(8, Q_MAJ7S11)),
    P4(C_(0, Q_MIN), C_(0, Q_MIN6), C_(8, Q_MAJ7), CS_(5, Q_MIN, 7)),
    P4(C_(0, Q_MIN11), C_(6, Q_MAJ), C_(8, Q_AUG), C_(3, Q_SUS2))};
static const gprog_t HEA_L[] = {
    P4(C_(0, Q_ADD9), C_(8, Q_AUG), C_(3, Q_SUS2), C_(10, Q_MAJ)),
    P4(C_(0, Q_MAJ7), C_(6, Q_MAJ7S11), C_(4, Q_AUG), C_(11, Q_SUS4)),
    P4(C_(0, Q_SUS2), C_(5, Q_SUS2), C_(3, Q_ADD9), C_(1, Q_MAJ7))};
static const gdrum_t HEA_DR[2] = {
    {{"x..x.x..x.xo..x.", "....x..o..x.o..x", "....o.......x...", "xxoxxoxxxxoxxoxx", 0, 0, "..o..o.o...o..o.", 0}},
    {{"x..x.xx.x.xx..xo", "..o.x..x..x.x.xx", "....x..o....x..o", "xxxxxxoxxxxxxoxx", "..........o.....", 0, "o.o..o.o.o.o..o.", 0}}};
static const gdrum_t HEA_PC = {{0, 0, 0, "o.o..o.oo..oo.o.", 0, "...o......o....o", "o....o..o...o...", 0}};
static const char *const HEA_B[3] = {"R..R.R..R.RR..R.", "R..R....R.R...R.", "R....R..R..R..R."};
static const char *const HEA_C[3] = {"X---------------", "X---------------", "X-------X-------"};
static const char *const HEA_L_[3] = {"n.n.n-..n.n.n-..n.n.n-..n-n-n---", "n-n-n---....n-n-n-n-n---........",
                                      "..n.n.n-..n.n.n-..n.n.n-n-------"};
/* ENO: ambient: no beat (an ENERGY above half: a soft pulse), drones, loops of their own odd lengths, bells */
static const gprog_t ENO_D[] = {                    /* ((Vallée) five more each: a drone that shifts between two */
    P4(C_(0, Q_MIN9), C_(0, Q_MIN9), C_(8, Q_MAJ7), C_(8, Q_MAJ7)),   /* chords, the long minor, the Phrygian fall) */
    P4(C_(0, Q_MADD9), C_(10, Q_SUS2), C_(8, Q_MAJ7), C_(3, Q_ADD9)),
    P4(C_(0, Q_MIN11), C_(0, Q_MIN11), C_(0, Q_MIN11), C_(5, Q_SUS2)),
    P4(C_(0, Q_MIN9), C_(3, Q_MAJ7), C_(0, Q_MIN9), C_(10, Q_SUS4)),
    P4(C_(0, Q_SUS2), C_(1, Q_MAJ7), C_(0, Q_SUS2), C_(1, Q_MAJ7)),
    P4(C_(0, Q_MADD9), C_(8, Q_MAJ7S11), C_(5, Q_MIN9), C_(0, Q_MADD9)),
    P4(C_(0, Q_MIN7), C_(10, Q_MAJ9), C_(8, Q_MAJ7), C_(5, Q_MIN11))};
static const gprog_t ENO_L[] = {
    P4(C_(0, Q_ADD9), C_(5, Q_ADD9), C_(0, Q_ADD9), C_(7, Q_SUS4)),
    P4(C_(0, Q_MAJ7), C_(0, Q_MAJ7), C_(9, Q_MIN7), C_(9, Q_MIN7)),
    P4(C_(0, Q_SUS2), C_(2, Q_MIN7), C_(5, Q_MAJ7), C_(0, Q_SUS2)),
    P4(C_(0, Q_MAJ9), C_(0, Q_MAJ9), C_(2, Q_ADD9), C_(2, Q_ADD9)),
    P4(C_(0, Q_MAJ7S11), C_(7, Q_SUS2), C_(9, Q_MIN9), C_(5, Q_MAJ9)),
    P4(C_(5, Q_MAJ9), C_(0, Q_ADD9), C_(5, Q_MAJ9), C_(7, Q_9SUS4)),
    P4(C_(0, Q_ADD9), C_(4, Q_MIN7), C_(9, Q_MIN9), C_(2, Q_SUS2)),
    P4(C_(0, Q_SUS4), C_(0, Q_SUS2), C_(10, Q_ADD9), C_(0, Q_SUS2))};
static const gdrum_t ENO_DR[2] = {
    {{"o...............", 0, 0, 0, 0, 0, "........o.......", 0}},
    {{"x.......o.......", 0, 0, 0, 0, "............o...", "....o.......o...", 0}}};
static const gdrum_t ENO_PC = {{0, 0, 0, 0, 0, 0, 0, 0}};
static const char *const ENO_B[3] = {"R---------------", "R-------5-------", "R-----------O---"};
static const char *const ENO_C[3] = {"X---------------", "X-------X-------", "X-----------X---"};
static const char *const ENO_L_[3] = {"n-------........................", "........n-------................",
                                      "n-----------................n---"};
/* STEELY: the polished jazz-funk of Aja's "Peg": a bright major key, a crisp sixteenth groove with a little lilt
 * and ghosted snares, a popped octave bass, an electric piano comping in syncopated sixteenths, sus and "mu" chords
 * over moving basses, a stack of background voices holding the changes, a bluesy guitar answering at the bars' ends */
static const gprog_t STE_D[] = {
    P8(C_(0, Q_MU), C_(0, Q_MU), CS_(5, Q_MAJ, 2), CS_(5, Q_MAJ, 2), C_(10, Q_MAJ9), CS_(3, Q_MAJ, 2), C_(7, Q_9SUS4),
       C_(7, Q_7S9)),
    P8(C_(0, Q_DOM9), C_(0, Q_DOM9), C_(5, Q_DOM13), C_(5, Q_DOM13), CS_(10, Q_MAJ, 2), C_(8, Q_MAJ7S11), C_(7, Q_9SUS4),
       C_(7, Q_7S9)),
    P4(C_(0, Q_MU), CS_(10, Q_MAJ, 2), C_(5, Q_MAJ9), C_(7, Q_9SUS4))};
static const gprog_t STE_L[] = {
    P8(C_(0, Q_MU), C_(0, Q_MU), CS_(5, Q_MAJ, 2), CS_(5, Q_MAJ, 2), C_(4, Q_MIN7), C_(9, Q_DOM13), C_(2, Q_MIN9),
       C_(7, Q_9SUS4)),
    P8(C_(5, Q_MAJ9), CS_(5, Q_MAJ, 2), C_(0, Q_MU), CS_(0, Q_MAJ, 4), C_(3, Q_MU), C_(8, Q_MAJ7S11), C_(2, Q_MIN11),
       C_(7, Q_7S9)),
    P4(C_(0, Q_MAJ9), CS_(10, Q_MAJ, 2), CS_(5, Q_MAJ, 2), C_(7, Q_9SUS4)),
    P8(C_(0, Q_MU), C_(0, Q_MU), C_(9, Q_MIN9), C_(9, Q_MIN9), C_(5, Q_MAJ7), CS_(5, Q_MIN, 2), C_(7, Q_9SUS4),
       C_(7, Q_DOM13))};
static const gdrum_t STE_DR[2] = {
    {{"x.....x..x....x.", "....a..g.g..a..g", 0, "axxxaxxxaxxxaxxx", 0, 0, "..o.......o.....", 0}},
    {{"x.....x..x.x..x.", "..g.a..g.g.ga.gg", 0, "axxxaxxxaxxxaxxo", "..............x.", 0, 0, 0}}};
static const gdrum_t STE_PC = {{0, 0, "....o.......o...", "x.ox.ox.x.ox.ox.", 0, 0, 0, 0}};
static const char *const STE_B[3] = {"R..O.RO..R.OR.CA", "R.O..RO.R..O.RCP", "R..RO.ORR.O..OCA"};
static const char *const STE_C[3] = {".s..s.s..s..s.s.", "X-.s..s.X-.s..sP", ".s.s..X-..s.s.s."};
static const char *const STE_L_[3] = {"......n-n-n-....n-..n-n---......", "..n-n-n-..n-....n-n-n---n-......",
                                      "......n-.n-n-...n-..n-n-n-------"};
/* YACHT: the classic soft-rock groove of a Michael McDonald-era intro ("What a Fool Believes"): a piano bouncing on
 * syncopated eighths, its chords landing a sixteenth ahead of the bar, upper triads over a walking bass, sixteenth
 * hats accented on the eighths, the kick dancing around the backbeat, then a soulful lead and strings */
static const gprog_t YAC_D[] = {
    P8(C_(0, Q_MIN9), C_(0, Q_MIN9), CS_(10, Q_MAJ, 2), CS_(10, Q_MAJ, 2), C_(8, Q_MAJ7), C_(8, Q_MAJ7),
       CS_(5, Q_MAJ, 2), C_(7, Q_9SUS4)),
    P8(C_(2, Q_MIN9), CS_(0, Q_MAJ, 4), C_(5, Q_MAJ7), CS_(5, Q_MAJ, 2), C_(4, Q_MIN7), C_(9, Q_MIN7), C_(2, Q_MIN9),
       C_(7, Q_9SUS4))};
static const gprog_t YAC_L[] = {
    P8(C_(0, Q_MAJ), CS_(0, Q_MAJ, 4), C_(5, Q_MAJ), CS_(5, Q_MAJ, 2), C_(9, Q_MIN7), CS_(9, Q_MIN, 10), C_(5, Q_MAJ7),
       CS_(5, Q_MAJ, 2)),
    P8(CS_(5, Q_MAJ, 2), CS_(5, Q_MAJ, 2), C_(4, Q_MIN7), C_(9, Q_MIN7), C_(2, Q_MIN9), CS_(0, Q_MAJ, 4), C_(5, Q_MAJ7),
       C_(7, Q_9SUS4)),
    P8(C_(0, Q_MAJ9), CS_(10, Q_MAJ, 2), C_(5, Q_MAJ), CS_(5, Q_MIN, 4), C_(2, Q_MIN7), CS_(0, Q_MAJ, 4),
       CS_(5, Q_MAJ, 2), C_(7, Q_9SUS4))};
static const gdrum_t YAC_DR[2] = {
    {{"x.....x.x.....x.", "....a.......a...", 0, "axxxaxxxaxxxaxxx", 0, 0, 0, 0}},
    {{"x.....x.x..x..x.", "....a..g....a..g", 0, "axxxaxxxaxxxaxxx", "..............o.", 0, 0, 0}}};
static const gdrum_t YAC_PC = {{0, 0, "....o.......x...", "x.x.x.x.x.x.x.x.", 0, 0, 0, 0}};
static const char *const YAC_B[3] = {"R..R..R.R.R..RP.", "R..R..O.R.R..RP.", "R..R.ROR..R.R.P."};
static const char *const YAC_C[3] = {"s.s.ss.ss.s.ss.P", "s.ss.ss.s.ss.s.P", "s.s.ss.ss.s.s.sP"};
static const char *const YAC_L_[3] = {"n--.n.n-n-------..n-n.n-n-----..", "..n-n-n-n--.n---....n-n-n---n---",
                                      "n-n-n---n.n-n---....n-n-n-n-----"};
/* LCD: the slow, lovesick synth-pop of American Dream's opener ("Oh Baby"): a plain drum machine ticking at 86-100,
 * a pulsing analog sequence in eighths, big held synth chords swelling a bar or two at a time, a crooned lead on
 * long notes, a shimmering sixteenth arp rising out of it; mostly major, sweet and sad */
static const gprog_t LCD_D[] = {
    P4(C_(0, Q_MIN), C_(8, Q_MAJ7), C_(3, Q_MAJ), C_(10, Q_SUS4)),
    P4(C_(0, Q_MADD9), C_(0, Q_MADD9), C_(8, Q_MAJ), C_(8, Q_MAJ)),
    P4(C_(0, Q_MIN), C_(5, Q_MIN7), C_(8, Q_MAJ), C_(10, Q_MAJ))};
static const gprog_t LCD_L[] = {
    P4(C_(0, Q_ADD9), C_(0, Q_ADD9), C_(9, Q_MIN7), C_(5, Q_MAJ7)),
    P4(C_(0, Q_MAJ), C_(5, Q_ADD9), C_(9, Q_MIN), C_(7, Q_SUS4)),
    P4(C_(5, Q_MAJ7), C_(5, Q_MAJ7), C_(0, Q_ADD9), C_(7, Q_MAJ)),
    P4(C_(0, Q_MAJ), CS_(7, Q_MAJ, 5), C_(9, Q_MIN), C_(5, Q_MAJ))};
static const gdrum_t LCD_DR[2] = {
    {{"x.......x.......", "....x.......x...", 0, "x.x.x.x.x.x.x.x.", 0, 0, 0, 0}},
    {{"x.......x.x.....", "....a.......a...", "....x.......x...", "xxxxxxxxxxxxxxxx", "..............o.", 0, 0,
      0}}};
static const gdrum_t LCD_PC = {{0, 0, 0, "xoxoxoxoxoxoxoxo", 0, 0, 0, 0}};
static const char *const LCD_B[3] = {"R.R.R.R.R.R.R.R.", "R.R.R.R.O.R.R.R.", "R.R.R.R.R.R.5.5."};
static const char *const LCD_C[3] = {"X---------------", "X-------X-------", "X---------------"};
static const char *const LCD_L_[3] = {"n---n-n-n-------........n-n-n---", "....n---n---n-------n-n-n-------",
                                      "n-n-n-----------....n-n-n-n-----"};
/* STROKE: garage rock: straight eighths on hats, the bass on the roots, the chords downstroked in eighths, a single-
 * note guitar riff weaving through */
static const gprog_t STR_D[] = {
    P4(C_(0, Q_MIN), C_(10, Q_MAJ), C_(8, Q_MAJ), C_(7, Q_MAJ)),
    P4(C_(0, Q_MIN), C_(8, Q_MAJ), C_(3, Q_MAJ), C_(10, Q_MAJ)),
    P4(C_(0, Q_MIN), C_(5, Q_MIN), C_(10, Q_MAJ), C_(3, Q_MAJ))};
static const gprog_t STR_L[] = {
    P4(C_(0, Q_MAJ), C_(9, Q_MIN), C_(5, Q_MAJ), C_(7, Q_MAJ)),
    P4(C_(0, Q_MAJ), C_(4, Q_MIN), C_(9, Q_MIN), C_(5, Q_MAJ)),
    P4(C_(0, Q_MAJ), C_(7, Q_MAJ), C_(9, Q_MIN), C_(5, Q_MAJ)),
    P8(C_(0, Q_MAJ), C_(0, Q_MAJ), C_(2, Q_MIN), C_(2, Q_MIN), C_(4, Q_MIN), C_(4, Q_MIN), C_(5, Q_MAJ), C_(7, Q_MAJ))};
static const gdrum_t STR_DR[2] = {
    {{"x.....x.x.....x.", "....a.......a...", 0, "x.x.x.x.x.x.x.x.", 0, 0, 0, 0}},
    {{"x.....x.x.x...x.", "....a.......a.o.", 0, "x.x.x.x.x.x.x.x.", "a...............", 0, 0, 0}}};
static const gdrum_t STR_PC = {{0, 0, 0, "x.x.x.x.x.x.x.x.", 0, 0, 0, 0}};
static const char *const STR_B[3] = {"R.R.R.R.R.R.R.R.", "R.R.R.R.R.R.5.A.", "R.R.R.R.O.O.R.A."};
static const char *const STR_C[3] = {"s.s.s.s.s.s.s.s.", "s.s.s.s.s.s.s.ss", "X--.s.s.s.s.X--."};
static const char *const STR_L_[3] = {"n.n.n.n.n.n.n.n.n.n.n.n.n.n.n.n.", "n.n.n.nnn.n.n.n.n.n.n.n.n-n.n...",
                                      "n-n.n-n.n-n.n.n.n-n.n-n.n-n.n-.."};

/* SLAB: motorik, after Neu!, Can and Stereolab: the steady beat (the kick on the one and the and of two, the snare on
 * two and four, eighths on the hats) at 120-138, a one- or two-chord drone, an eighth-note bass, an organ's pulse,
 * "ba-ba" voices, an analog arp; nothing swings, the hands a little loose */
static const gprog_t SLB_D[] = {
    P4(C_(0, Q_MIN7), C_(0, Q_MIN7), C_(0, Q_MIN7), C_(0, Q_MIN7)),
    P4(C_(0, Q_MIN7), C_(0, Q_MIN7), C_(10, Q_MAJ), C_(10, Q_MAJ)),
    P4(C_(0, Q_MIN9), C_(5, Q_DOM9), C_(0, Q_MIN9), C_(5, Q_DOM9))};
static const gprog_t SLB_L[] = {
    P4(C_(0, Q_MAJ7), C_(0, Q_MAJ7), C_(10, Q_MAJ7), C_(10, Q_MAJ7)),
    P4(C_(0, Q_MAJ7), C_(5, Q_MAJ7), C_(0, Q_MAJ7), C_(5, Q_MAJ7)),
    P4(C_(0, Q_ADD9), C_(0, Q_ADD9), C_(0, Q_ADD9), C_(2, Q_MIN7))};
static const gdrum_t SLB_DR[2] = {
    {{"x.....x.x.......", "....x.......x...", 0, "x.x.x.x.x.x.x.x.", 0, 0, 0, 0}},
    {{"x.....x.x.....x.", "....a.......a...", 0, "x.x.x.x.x.x.x.x.", "..............o.", 0, 0, 0}}};
static const gdrum_t SLB_PC = {{0, 0, 0, "xxxxxxxxxxxxxxxx", 0, 0, 0, 0}};
static const char *const SLB_B[3] = {"R.R.R.R.R.R.R.R.", "R.R.R.R.5.5.R.R.", "R.R.O.R.R.R.O.R."};
static const char *const SLB_C[3] = {"s.s.s.s.s.s.s.s.", "X-------X-------", "X---s.s.X---s.s."};
static const char *const SLB_L_[3] = {"n-..n-..n-n-n---n-..n-..n-n-n---", "n.n.n---....n.n.n---....n.n.n---",
                                      "n---n---n-n-n---n---n---n-n-n---"};

/* WINWOOD: bright eighties blue-eyed soul in the vein of Steve Winwood's "Valerie": a drum machine with a big gated
 * snare at 116-124, sixteenth hats, a synth bass pumping eighths, glossy polysynth chords punched and held, sus
 * chords resolving, a soulful lead, a sparkling sequenced arp; major and hopeful */
static const gprog_t WIN_D[] = {
    P4(C_(0, Q_MIN7), C_(10, Q_ADD9), C_(8, Q_MAJ7), C_(10, Q_SUS4)),
    P4(C_(0, Q_MIN9), C_(5, Q_DOM9), C_(0, Q_MIN9), CS_(10, Q_MAJ, 2))};
static const gprog_t WIN_L[] = {
    P4(C_(0, Q_ADD9), C_(5, Q_MAJ), C_(9, Q_MIN7), C_(7, Q_SUS4)),
    P4(C_(0, Q_MAJ), CS_(7, Q_MAJ, 5), C_(5, Q_MAJ9), C_(7, Q_MAJ)),
    P8(C_(5, Q_MAJ), C_(5, Q_MAJ), C_(0, Q_ADD9), CS_(0, Q_MAJ, 4), C_(2, Q_MIN7), C_(9, Q_MIN7), C_(7, Q_SUS4),
       C_(7, Q_MAJ)),
    P4(C_(0, Q_MAJ), C_(4, Q_MIN7), C_(5, Q_ADD9), C_(7, Q_9SUS4))};
static const gdrum_t WIN_DR[2] = {
    {{"x.....x.x.......", "....a.......a...", "....x.......x...", "xxxxxxxxxxxxxxxx", 0, 0, 0, 0}},
    {{"x.....x.x.x.....", "....a.......a...", "....x.......x...", "axxxaxxxaxxxaxxx", "..............x.",
      "............o.o.", 0, 0}}};
static const gdrum_t WIN_PC = {{0, 0, 0, "x.xxx.xxx.xxx.xx", 0, 0, 0, 0}};
static const char *const WIN_B[3] = {"R.R.R.R.R.R.R.R.", "R.R.R.RRR.R.O.RA", "R.R.O.R.R.R.O.R."};
static const char *const WIN_C[3] = {"X-..X-..X-..X-.P", "X-------..s.X---", "X-.sX-.sX---s.s."};
static const char *const WIN_L_[3] = {"..n-n-n-n---n-......n-n-n-n-----", "n-n-..n-n-n---..n-n-..n-n-n-----",
                                      "....n-n-n-n-n---....n-n-n-------"};
/* GEE: the warped, saturated eighties of Mk.gee's "Rockman": a crushed, pumping drum machine at 88-100, a deep
 * sparse bass, chorused and wobbling guitar-like chords, falsetto lines up high, everything a little out of tape,
 * the harmony open (add9, maj7, sus) between major and its relative minor */
static const gprog_t GEE_D[] = {
    P4(C_(0, Q_MADD9), C_(8, Q_MAJ7), C_(3, Q_ADD9), C_(10, Q_SUS2)),
    P4(C_(0, Q_MIN7), C_(0, Q_MIN7), C_(5, Q_ADD9), CS_(8, Q_MAJ, 2))};
static const gprog_t GEE_L[] = {
    P4(C_(0, Q_ADD9), C_(4, Q_MIN7), C_(5, Q_MAJ7), C_(5, Q_MAJ7)),
    P4(C_(0, Q_MAJ7), CS_(7, Q_MAJ, 5), C_(9, Q_MADD9), C_(5, Q_SUS2)),
    P4(C_(5, Q_ADD9), C_(0, Q_ADD9), C_(7, Q_SUS4), C_(9, Q_MIN7))};
static const gdrum_t GEE_DR[2] = {
    {{"x.......x.x.....", "....a.......a...", 0, "x.x.x.x.x.x.x.x.", 0, 0, 0, 0}},
    {{"x.....x.x.x.....", "....a.......a..g", "....x.......x...", "x.x.x.x.x.x.x.x.", "......o.......o.", 0, 0,
      0}}};
static const gdrum_t GEE_PC = {{0, 0, 0, 0, 0, "...........o.o.o", "o.......o.......", 0}};
static const char *const GEE_B[3] = {"R-------R.......", "R-----R.R-------", "R-------5---R..A"};
static const char *const GEE_C[3] = {"X--.s.X--.s.X-s.", "X-------X--.s.s.", "X--.X--.X---s..."};
static const char *const GEE_L_[3] = {"....n-n-n---....n-n-n-n-n-------", "n---n-n-n-----..........n-n-n---",
                                      "..n-n-n-n-n-----..n-n-n---------"};

#define GS(a) a, sizeof a / sizeof a[0]
static const gstyle_t GSTY[GS_N] = {
    /* IMPALA */
    {96, 118, 12, 1, 0, {9, 4, 2, 7}, 3, 4, IMP_D, IMP_L, 4, 4, IMP_DR, &IMP_PC, IMP_B, IMP_C, IMP_L_, XC_ARP16, 64, 86,
     {{{E_ANALOG, "MONO BASS"}, {E_DIGITAL, "FM BASS"}, {E_TRIO, "TURBO BASS"}},
      {{E_ANALOG, "JP8 PAD"}, {E_ANALOG, "VHS PAD"}, {E_PHASE, "CZ STRING"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_ANALOG, "JUNO LEAD"}, {E_TRIO, "SQR LEAD"}, {E_LOFI, "WAVE LEAD"}},
      {{E_DRUM, "VINTAGE"}, {E_DRUM, "ACOUSTIC"}, {E_DRUM, "LO-FI"}},
      {{E_TRIO, "MIDNIGHT ARP"}, {E_LOFI, "8BIT ARP"}, {E_VOICE, "CHOIR AAH"}},
      {{E_DRUM, "JAZZ"}, {E_DRUM, "LO-FI"}, {0, 0}}},
     2, 100, 10, 70, 70, 2, {6, 36, 30, 22, 30, 16}, {0, 10, 40, 0, 50, 0}, 6, 0, 0},
    /* DEMARCO */
    {72, 96, 18, 2, 1, {7, 2, 9, 4}, 3, 1, DEM_D, DEM_L, 3, 4, DEM_DR, &DEM_PC, DEM_B, DEM_C, DEM_L_, XC_JANGLE, 62, 81,
     {{{E_DIGITAL, "FM BASS"}, {E_ANALOG, "FUNK BASS"}, {E_FM6, "DX BASS 4"}},
      {{E_DIGITAL, "WURLI"}, {E_DIGITAL, "RHODES"}, {E_FM6, "TINE EP"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_LOFI, "WAVE LEAD"}, {E_ANALOG, "PAN FLUTE"}, {E_DIGITAL, "HARP"}},
      {{E_DRUM, "VINTAGE"}, {E_DRUM, "LO-FI"}, {E_DRUM, "DUST"}},
      {{E_FM6, "NYLON PICK"}, {E_DIGITAL, "KALIMBA"}, {E_DIGITAL, "MUSIC BOX"}},
      {{E_DRUM, "JAZZ"}, {0, 0}, {0, 0}}},
     0, 70, 11, 50, 50, 0, {10, 24, 26, 14, 30, 10}, {0, 0, 20, 0, 12, 0}, 8, 1, 0},
    /* DAFT */
    {118, 126, 8, 1, 0, {9, 5, 2, 0}, 2, 1, DAF_D, DAF_L, 3, 3, DAF_DR, &DAF_PC, DAF_B, DAF_C, DAF_L_, XC_ROBOT, 60, 76,
     {{{E_ANALOG, "FUNK BASS"}, {E_DIGITAL, "FM BASS"}, {E_TRIO, "TURBO BASS"}},
      {{E_ANALOG, "POLY COMP"}, {E_ANALOG, "JUNO CHORUS"}, {E_FM6, "TINE EP"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_VOICE, "TALKBOX"}, {E_TRIO, "SYNC LEAD"}, {E_ANALOG, "FAT LEAD"}},
      {{E_DRUM, "HOUSE"}, {E_DRUM, "909"}, {E_DRUM, "DISCO"}},
      {{E_VOICE, "TALKBOX"}, {E_TRIO, "RAVE STAB"}, {E_LOFI, "CHIP CHORD"}},
      {{E_DRUM, "GARAGE"}, {E_DRUM, "HOUSE"}, {0, 0}}},
     0, 60, 11, 40, 60, 2, {0, 16, 16, 10, 18, 8}, {0, 10, 30, 0, 24, 0}, 0, 0, 1},
    /* HEAD */
    {132, 140, 0, 1, 0, {9, 2, 7, 4}, 2, 9, HEA_D, HEA_L, 4, 3, HEA_DR, &HEA_PC, HEA_B, HEA_C, HEA_L_, XC_TEXTURE, 64,
     79,
     {{{E_ANALOG, "SUB BASS"}, {E_ANALOG, "REESE"}, {E_TRIO, "TURBO BASS"}},
      {{E_ANALOG, "DARK STR"}, {E_OBV, "DARK PAD"}, {E_ANALOG, "ATMOS PAD"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_VOICE, "SOUL OOH"}, {E_VOICE, "VOX PAD"}, {E_DIGITAL, "GLASS PAD"}},
      {{E_DRUM, "GLITCH"}, {E_DRUM, "ELECTRO"}, {E_DRUM, "INDUSTR"}},
      {{E_MORPH, "NEON SWEEP"}, {E_MORPH, "GLASS MOTION"}, {E_DRONE, "DARK TIDE"}},
      {{E_DRUM, "606"}, {E_DRUM, "GLITCH"}, {0, 0}}},
     0, 70, 11, 50, 50, 0, {6, 34, 30, 8, 30, 16}, {0, 10, 30, 0, 24, 18}, 0, 0, 0},
    /* ENO */
    {54, 80, 0, 1, 0, {3, 2, 4, 9}, 3, 9, ENO_D, ENO_L, 7, 8, ENO_DR, &ENO_PC, ENO_B, ENO_C, ENO_L_, XC_BELLS, 67, 91,
     {{{E_ANALOG, "SUB BASS"}, {E_DRONE, "DEEP SPACE"}, {E_FM6, "PAD"}},
      {{E_MORPH, "VECTOR PAD"}, {E_VOICE, "CHOIR AAH"}, {E_OBV, "AIR CHOIR"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_DIGITAL, "TUBE BELL"}, {E_DIGITAL, "KALIMBA"}, {E_DIGITAL, "MUSIC BOX"}},
      {{E_DRUM, "AMBIENT"}, {E_DRUM, "LO-FI"}, {0, 0}},
      {{E_DIGITAL, "GLASS PAD"}, {E_SAMPLE, "LOFI FLUTE"}, {E_DIGITAL, "CELESTA"}},
      {{E_DRUM, "AMBIENT"}, {0, 0}, {0, 0}}},
     2, 127, 12, 82, 60, 0, {30, 80, 70, 40, 70, 40}, {0, 20, 50, 0, 50, 0}, 0, 0, 0},
    /* STEELY */
    {124, 132, 10, 3, 0, {7, 2, 9, 4}, 4, 1, STE_D, STE_L, 3, 4, STE_DR, &STE_PC, STE_B, STE_C, STE_L_, XC_FILLS, 64,
     83,
     {{{E_ANALOG, "FUNK BASS"}, {E_DIGITAL, "FM BASS"}, {E_FM6, "DX BASS 5"}},
      {{E_DIGITAL, "RHODES"}, {E_DIGITAL, "DX7 EP"}, {E_PHASE, "SOFT KEYS"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_VOICE, "CHOIR AAH"}, {E_FM6, "BRASS 6"}, {E_ANALOG, "SYN BRASS"}},
      {{E_DRUM, "ACOUSTIC"}, {E_DRUM, "TIGHT"}, {E_DRUM, "JAZZ"}},
      {{E_FM6, "NYLON PICK"}, {E_ANALOG, "TRAP PLUCK"}, {E_DIGITAL, "HARP"}},
      {{E_DRUM, "JAZZ"}, {0, 0}, {0, 0}}},
     0, 50, 11, 34, 30, 0, {4, 14, 22, 8, 14, 6}, {0, 0, 12, 0, 10, 0}, 8, 0, 0},
    /* YACHT */
    {116, 124, 4, 2, 0, {1, 8, 3, 10}, 3, 1, YAC_D, YAC_L, 2, 3, YAC_DR, &YAC_PC, YAC_B, YAC_C, YAC_L_, XC_PAD, 64, 84,
     {{{E_DIGITAL, "FM BASS"}, {E_FM6, "DX BASS 2"}, {E_ANALOG, "FUNK BASS"}},
      {{E_DIGITAL, "M1 PIANO"}, {E_DIGITAL, "RHODES"}, {E_DIGITAL, "FM GRAND"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_VOICE, "SOUL OOH"}, {E_SAMPLE, "SAX"}, {E_ANALOG, "JP8 BRASS"}},
      {{E_DRUM, "LINNDRUM"}, {E_DRUM, "ACOUSTIC"}, {E_DRUM, "LM-1"}},
      {{E_ANALOG, "JUNO STRINGS"}, {E_VOICE, "M1 STRINGS"}, {E_ANALOG, "SYN BRASS"}},
      {{E_DRUM, "JAZZ"}, {0, 0}, {0, 0}}},
     2, 70, 11, 36, 34, 0, {6, 18, 24, 10, 32, 8}, {0, 0, 16, 0, 0, 0}, 8, 0, 0},
    /* LCD */
    {86, 100, 0, 1, 0, {0, 9, 4, 2}, 2, 1, LCD_D, LCD_L, 3, 4, LCD_DR, &LCD_PC, LCD_B, LCD_C, LCD_L_, XC_ARP16, 60, 79,
     {{{E_ANALOG, "MONO BASS"}, {E_ANALOG, "SAW BASS"}, {E_ANALOG, "REESE"}},
      {{E_ANALOG, "JP8 PAD"}, {E_ANALOG, "JUNO STRINGS"}, {E_ANALOG, "VHS PAD"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_VOICE, "VOX PAD"}, {E_ANALOG, "JUNO LEAD"}, {E_VOICE, "CHOIR AAH"}},
      {{E_DRUM, "DISCO"}, {E_DRUM, "909"}, {E_DRUM, "ELECTRO"}},
      {{E_TRIO, "MIDNIGHT ARP"}, {E_MORPH, "ORBIT ARP"}, {E_LOFI, "8BIT ARP"}},
      {{E_DRUM, "606"}, {E_DRUM, "HOUSE"}, {0, 0}}},
     2, 78, 11, 44, 30, 0, {4, 26, 30, 8, 26, 8}, {0, 6, 26, 0, 20, 0}, 0, 0, 0},
    /* STROKE */
    {124, 146, 0, 0, 0, {9, 4, 2, 7}, 2, 1, STR_D, STR_L, 3, 4, STR_DR, &STR_PC, STR_B, STR_C, STR_L_, XC_JANGLE, 60, 79,
     {{{E_DIGITAL, "FM BASS"}, {E_ANALOG, "SAW BASS"}, {E_FM6, "DX BASS 6"}},
      {{E_FM6, "NYLON PICK"}, {E_DIGITAL, "HARP"}, {E_ANALOG, "TRAP PLUCK"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_ANALOG, "TRAP PLUCK"}, {E_FM6, "NYLON PICK"}, {E_MORPH, "ICE PLUCK"}},
      {{E_DRUM, "VINTAGE"}, {E_DRUM, "TIGHT"}, {E_DRUM, "ACOUSTIC"}},
      {{E_WHEEL, "STREET ORGAN"}, {E_FM6, "NYLON PICK"}, {E_ANALOG, "TRAP PLUCK"}},
      {{E_DRUM, "BRIGHT"}, {0, 0}, {0, 0}}},
     0, 44, 11, 30, 30, 0, {4, 10, 10, 8, 10, 6}, {0, 0, 6, 0, 0, 0}, 0, 0, 0},
    /* SLAB */
    {120, 138, 0, 1, 0, {4, 9, 2, 7}, 3, 1, SLB_D, SLB_L, 3, 3, SLB_DR, &SLB_PC, SLB_B, SLB_C, SLB_L_, XC_ARP16, 62, 79,
     {{{E_ANALOG, "MONO BASS"}, {E_ANALOG, "SAW BASS"}, {E_TRIO, "TURBO BASS"}},
      {{E_WHEEL, "JAZZ ORGAN"}, {E_WHEEL, "STREET ORGAN"}, {E_WHEEL, "CHAPEL"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_VOICE, "CHOIR AAH"}, {E_VOICE, "SOUL OOH"}, {E_ANALOG, "SINE PAD"}},
      {{E_DRUM, "TIGHT"}, {E_DRUM, "ACOUSTIC"}, {E_DRUM, "LO-FI"}},
      {{E_LOFI, "8BIT ARP"}, {E_LOFI, "GAME LEAD"}, {E_TRIO, "SEQ PULSE"}},
      {{E_DRUM, "JAZZ"}, {E_DRUM, "DUST"}, {0, 0}}},
     0, 60, 10, 40, 40, 2, {4, 16, 24, 8, 20, 8}, {0, 0, 24, 0, 26, 0}, 8, 0, 0},
    /* WINWOOD */
    {116, 124, 0, 1, 0, {0, 7, 2, 9}, 3, 1, WIN_D, WIN_L, 2, 4, WIN_DR, &WIN_PC, WIN_B, WIN_C, WIN_L_, XC_ARP16, 64,
     83,
     {{{E_ANALOG, "SAW BASS"}, {E_ANALOG, "MONO BASS"}, {E_DIGITAL, "FM BASS"}},
      {{E_ANALOG, "JP8 BRASS"}, {E_ANALOG, "WARM PAD"}, {E_DIGITAL, "DX7 EP"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_ANALOG, "JUNO LEAD"}, {E_ANALOG, "FAT LEAD"}, {E_ANALOG, "PWM LEAD"}},
      {{E_DRUM, "LINNDRUM"}, {E_DRUM, "80S"}, {E_DRUM, "LM-1"}},
      {{E_TRIO, "MIDNIGHT ARP"}, {E_DIGITAL, "DX7 BELL"}, {E_ANALOG, "TRAP PLUCK"}},
      {{E_DRUM, "SYNWAVE"}, {0, 0}, {0, 0}}},
     0, 64, 11, 40, 30, 0, {4, 20, 24, 14, 20, 8}, {0, 0, 18, 0, 20, 0}, 8, 0, 0},
    /* GEE */
    {88, 100, 6, 1, 1, {4, 9, 2, 7}, 3, 1, GEE_D, GEE_L, 2, 3, GEE_DR, &GEE_PC, GEE_B, GEE_C, GEE_L_, XC_JANGLE, 67, 88,
     {{{E_DIGITAL, "FM BASS"}, {E_ANALOG, "SUB BASS"}, {E_TRIO, "TURBO BASS"}},
      {{E_FM6, "NYLON PICK"}, {E_DIGITAL, "MUSIC BOX"}, {E_DIGITAL, "M1 PIANO"}},   /* ((Vallée) SPAMPY's sounds */
      {{E_VOICE, "VOX PAD"}, {E_VOICE, "SOUL OOH"}, {E_VOICE, "CHOIR AAH"}},
      {{E_DRUM, "BOOMBAP"}, {E_DRUM, "LO-FI"}, {E_DRUM, "DUST"}},
      {{E_FM6, "NYLON PICK"}, {E_LOFI, "WAVE LEAD"}, {E_DIGITAL, "HARP"}},
      {{E_DRUM, "LO-FI"}, {E_DRUM, "DUST"}, {0, 0}}},
     2, 66, 11, 40, 34, 0, {6, 26, 34, 10, 24, 8}, {0, 10, 24, 0, 14, 0}, 8, 1, 1},
};

/* --------------------------------------------------------------- the plan --- */
/* what a piece is before its notes: the panel shows it, OCT+ writes from it */
typedef struct {
    uint8_t key, scale, bpm, swing, light, ext, n, bars;
    uint8_t ch[8][3];            /* the part's chords: root (0..11 above the key), quality, the bass above the root */
} gplan_t;
static const gstyle_t *g_sty(void) { return &GSTY[gen.v[GK_STYLE] % GS_N]; }
/* the quality at an extension level: 0 triads, 1 as written, 2 ninths, 3 ninths without the root (KEYS) */
static uint32_t q_ext(uint32_t q, int32_t lv)
{
    if (lv <= 0) {
        switch (q) {
        case Q_MAJ7: case Q_MAJ9: case Q_DOM7: case Q_DOM9: case Q_DOM13: case Q_MU: case Q_ADD9: case Q_MAJ7S11:
        case Q_7S9: return Q_MAJ;
        case Q_MIN7: case Q_MIN9: case Q_MIN11: case Q_MIN6: case Q_MADD9: return Q_MIN;
        case Q_9SUS4: return Q_SUS4;
        default: return q;
        }
    }
    if (lv >= 2) {
        switch (q) {
        case Q_MAJ7: return Q_MAJ9;
        case Q_MIN7: return Q_MIN9;
        case Q_DOM7: return Q_DOM9;
        case Q_MAJ: return Q_ADD9;
        case Q_MIN: return Q_MADD9;
        default: return q;
        }
    }
    return q;
}
/* ((Vallée) MINE) a part's own chord i into p->ch[i]: a white key's degree of the plan's mode (its own chord there), a
 * black key's root (the mode's chord if the root is in it, else major); TYPE's quality, else the mode's at the style's
 * extensions (q_ext) */
static uint32_t gc_n(uint32_t part)
{
    uint32_t n = 0;
    while (part < GC_NPART && n < 4u && gch[part].r[n])
        n++;
    return n;
}
static int gc_mine(uint32_t part) { return part < GC_NPART && gch[part].on && gch[part].r[0]; }
static void gc_chord_code(gplan_t *p, uint32_t i, uint32_t code, uint32_t q)
{
    uint32_t m = SCALE_MASK[p->scale < NELEM(SCALE_MASK) ? p->scale : 1u], k, n = 0, d = 7, r, a, b;
    uint8_t pc[7];
    if (__builtin_popcount(m) != 7)                     /* (the modes GEN plays are all of seven) */
        m = SCALE_MASK[1];
    for (k = 0; k < 12u; k++)
        if ((m >> k) & 1u)
            pc[n++] = (uint8_t)k;
    r = code <= 7u ? pc[(code + 6u) % 7u] : (code - 8u) % 12u;
    for (k = 0; k < 7u; k++)
        if (pc[k] == r)
            d = k;
    a = d < 7u ? (pc[(d + 2u) % 7u] + 12u - r) % 12u : 4u;   /* the third and the fifth the mode has there */
    b = d < 7u ? (pc[(d + 4u) % 7u] + 12u - r) % 12u : 7u;
    k = a == 3u ? (b == 6u ? Q_DIM : Q_MIN) : b == 8u ? Q_AUG : Q_MAJ;
    p->ch[i][0] = (uint8_t)r;
    p->ch[i][1] = (uint8_t)(q ? (q - 1u) % Q_N : q_ext(k, p->ext));
    p->ch[i][2] = 0;
}
static void gc_chord(gplan_t *p, uint32_t part, uint32_t i) { gc_chord_code(p, i, gch[part].r[i], gch[part].q[i]); }
static void gen_plan(gplan_t *p)
{
    const gstyle_t *s = g_sty();
    uint32_t part = gen.v[GK_PART] % GP_N, vibe = gen.v[GK_VIBE] % GV_N, i, nb, a, b, c;
    grng_t r = g_stream('K', 0), h = g_stream('H', 0);
    const gprog_t *bank, *pr;
    int32_t lv;
    p->key = gen.v[GK_KEY] ? (uint8_t)((gen.v[GK_KEY] - 1u) % 12u) : s->keys[g_n(&r, 4)];
    p->light = vibe == GV_WARM || vibe == GV_BRIGHT || (vibe == GV_WEIRD && g_n(&r, 2));
    p->scale = p->light ? s->mode_light : s->mode_dark;
    if (vibe == GV_BRIGHT && p->scale == 1 && g_pct(&r, 40))
        p->scale = 9;                                   /* (LYD: brighter still) */
    if (vibe == GV_DARK && p->scale == 3 && g_pct(&r, 50))
        p->scale = 2;                                   /* (MIN: darker than DOR) */
    if (gen.v[GK_ENERGY] < 40u)                         /* (low ENERGY: slower than the style's range, 0: 70 % of it) */
        a = ((uint32_t)s->bpm_lo * (70u + 30u * gen.v[GK_ENERGY] / 40u) + 50u) / 100u + g_n(&r, 3) - 1u;
    else                                                /* (40 .. 100 %: across the style's range) */
        a = (uint32_t)s->bpm_lo + ((uint32_t)(s->bpm_hi - s->bpm_lo) * (gen.v[GK_ENERGY] - 40u) + 30u) / 60u +
            g_n(&r, 5) - 2u;
    p->bpm = (uint8_t)clamp(gen.v[GK_TEMPO] ? 60 + (int32_t)gen.v[GK_TEMPO] - 1 : (int32_t)a, 40, 240);
    p->swing = (uint8_t)(gen.v[GK_SWING] ? gen.v[GK_SWING] - 1u : s->swing);
    lv = (int32_t)s->ext + (vibe == GV_HAZY) - (vibe == GV_BRIGHT) + (gen.v[GK_COMPLEX] > 70) - (gen.v[GK_COMPLEX] < 15);
    p->ext = (uint8_t)clamp(lv, 0, 3);
    bank = p->light ? s->light : s->dark;
    nb = p->light ? s->nlight : s->ndark;
    a = g_n(&h, nb);                                    /* the verse's, the chorus's, the bridge's: three apart */
    b = nb > 1u ? (a + 1u + g_n(&h, nb - 1u)) % nb : a;
    c = a;
    if (nb > 2u)                                        /* (the first that is neither) */
        for (c = 0; c == a || c == b; c++)
            ;
    pr = &bank[part == GP_CHORUS ? b : part == GP_BRIDGE ? c : a];
    p->n = pr->n;
    p->bars = 4;
    for (i = 0; i < pr->n; i++) {
        p->ch[i][0] = pr->c[i][0];
        p->ch[i][1] = (uint8_t)q_ext(pr->c[i][1], p->ext);
        p->ch[i][2] = pr->c[i][2];
    }
    if (vibe == GV_WEIRD) {                             /* a chord turned strange: a semitone or a tritone away */
        i = 1u + g_n(&h, (uint32_t)p->n - 1u);
        p->ch[i][0] = (uint8_t)((p->ch[i][0] + (g_n(&h, 2) ? 6u : 1u)) % 12u);
        p->ch[i][1] = g_n(&h, 2) ? Q_MAJ7S11 : Q_SUS2;
        p->ch[i][2] = 0;
    }
    if (part == GP_OUTRO) {                             /* home at the end */
        p->ch[p->n - 1u][0] = p->ch[0][0];
        p->ch[p->n - 1u][1] = p->ch[0][1];
        p->ch[p->n - 1u][2] = p->ch[0][2];
    }
    if (gc_mine(part)) {                                /* ((Vallée) MINE: the part's own, a bar each, as long as they */
        p->n = p->bars = (uint8_t)gc_n(part);
        for (i = 0; i < p->n; i++)
            gc_chord(p, part, i);
    }
}
/* the chord at step i of 64 */
static uint32_t g_chord_at(const gplan_t *p, uint32_t i)   /* ((Vallée) MINE: the part's bars, round again after */
{
    uint32_t l = p->bars * 16u;
    return (i % 64u % l) * p->n / l;
}
/* a chord's name, "Dm7" */
static void g_chord_name(const gplan_t *p, uint32_t c, char *b)
{
    str_cpy(b, GEN_NOTE[(p->key + p->ch[c][0]) % 12u], 4);
    str_cpy(b + str_len(b), CQ[p->ch[c][1] % Q_N].sfx, 9);
    if (p->ch[c][2]) {                                  /* (a slash chord: "F/G") */
        str_cpy(b + str_len(b), "/", 2);
        str_cpy(b + str_len(b), GEN_NOTE[(p->key + p->ch[c][0] + p->ch[c][2]) % 12u], 4);
    }
}

/* ------------------------------------------------------------- the notes --- */
static void w_clear(step_t *w, uint32_t n)
{
    uint32_t i;
    memset(w, 0, n * sizeof *w);                        /* (every byte: no note left from the last track's) */
    for (i = 0; i < n; i++)
        w[i].time = ST_REST;
}
static void w_note(step_t *s, const uint8_t *notes, uint32_t n, int32_t vel)
{
    uint32_t k;
    s->time = ST_NOTE;
    s->n = (uint8_t)(n > 4u ? 4u : n);
    for (k = 0; k < 4u; k++)
        s->note[k] = k < s->n ? notes[k] : 0;
    s->vel = (uint8_t)clamp(vel, 1, 127);
    s->hit = s->acc = 0;
}
static void w_tie(step_t *s) { memset(s, 0, sizeof *s); s->time = ST_TIE; }
/* HUMAN: a velocity moved, a step nudged (laid back: late more than early) */
static int32_t g_vel(grng_t *r, int32_t v)
{
    int32_t h = gen.v[GK_HUMAN] * 24 / 100;
    return h ? v + (int32_t)g_n(r, (uint32_t)(2 * h + 1)) - h : v;
}
static void g_nudge(grng_t *r, step_t *s, int late)
{
    int32_t h = gen.v[GK_HUMAN] * 6 / 100, n;
    if (!h)
        return;
    n = (int32_t)g_n(r, (uint32_t)(2 * h + 1)) - h + (late ? h : 0);
    s->nudge = (int8_t)clamp(n, NUDGE_MIN, NUDGE_MAX);
}
/* the scale's pitch classes, as a mask (seq.c SCALE_MASK) */
static uint32_t g_mask(const gplan_t *p) { return SCALE_MASK[p->scale % NELEM(SCALE_MASK)]; }
static int g_in(const gplan_t *p, uint32_t m) { return (g_mask(p) >> ((m + 12u - p->key) % 12u)) & 1u; }
/* the scale's note nearest m, inside lo .. hi */
static int32_t g_snap(const gplan_t *p, int32_t m, int32_t lo, int32_t hi)
{
    int32_t j;
    m = clamp(m, lo, hi);
    for (j = 0; j < 6; j++) {
        if (m - j >= lo && g_in(p, (uint32_t)(m - j)))
            return m - j;
        if (m + j <= hi && g_in(p, (uint32_t)(m + j)))
            return m + j;
    }
    return m;
}
/* chord c's pitch classes (absolute), up to 5 */
static uint32_t g_pcs(const gplan_t *p, uint32_t c, uint8_t *pc)
{
    uint32_t q = p->ch[c][1] % Q_N, k;
    for (k = 0; k < CQ[q].n; k++)
        pc[k] = (uint8_t)((p->key + p->ch[c][0] + CQ[q].iv[k]) % 12u);
    return CQ[q].n;
}
static int g_tone(const gplan_t *p, uint32_t c, uint32_t m)
{
    uint8_t pc[5];
    uint32_t n = g_pcs(p, c, pc), k;
    for (k = 0; k < n; k++)
        if (pc[k] == m % 12u)
            return 1;
    return 0;
}

/* the chord voiced for KEYS: at most four notes, rootless when jazzy (ext 3), between lo and hi, the inversion that
 * moves least from the last (prev, pn notes) */
static uint32_t g_voice(const gplan_t *p, uint32_t c, const uint8_t *prev, uint32_t pn, uint8_t *out, int32_t lo, int32_t hi)
{
    uint8_t pc[5], use[4];
    uint32_t n = g_pcs(p, c, pc), q = p->ch[c][1] % Q_N, k, m = 0, r, best = 0xFFFFFFFFu, bn = 0;
    if (q == Q_POW) {                                   /* (a power chord: the root low, its fifth, the octave) */
        int32_t b = lo + (int32_t)((pc[0] + 12u - (uint32_t)lo % 12u) % 12u);
        out[0] = (uint8_t)b;
        out[1] = (uint8_t)(b + 7);
        out[2] = (uint8_t)(b + 12);
        return 3;
    }
    for (k = 0; k < n && m < 4u; k++) {                 /* which tones: rootless at ext 3, the fifth first to go */
        if (p->ext >= 3u && k == 0u && n >= 4u)
            continue;
        if (n - (p->ext >= 3u && n >= 4u) > 4u && CQ[q].iv[k] == 7u)
            continue;
        use[m++] = pc[k];
    }
    for (r = 0; r < m; r++) {                           /* each inversion: up from lo */
        uint8_t v[4];
        uint32_t cost = 0, j;
        int32_t x = lo + (int32_t)((use[r] + 12u - (uint32_t)lo % 12u) % 12u);
        for (j = 0; j < m; j++) {
            if (j)
                for (x++; (uint32_t)x % 12u != use[(r + j) % m]; x++)
                    ;
            v[j] = (uint8_t)x;
        }
        if (v[m - 1u] > hi)
            cost += 1000u;
        if (pn)
            for (j = 0; j < m; j++) {
                int32_t d = (int32_t)v[j] - (int32_t)prev[j < pn ? j : pn - 1u];
                cost += (uint32_t)(d < 0 ? -d : d);
            }
        else
            cost += (uint32_t)((v[0] + v[m - 1u]) / 2u > 64u ? (v[0] + v[m - 1u]) / 2u - 64u : 64u - (v[0] + v[m - 1u]) / 2u);
        if (cost < best) {
            best = cost;
            bn = m;
            memcpy(out, v, m);
        }
    }
    return bn;
}

/* DRUMS (T4) and PERC (T6): the style's lanes; ENERGY the maybes, COMPLEX the ghosts and rolls; bar 4 a fill */
static void g_drums(step_t *w, const gdrum_t *d, grng_t *r, int32_t energy, int fill, uint32_t len, int intro)
{
    uint32_t i, l;
    for (i = 0; i < len; i++) {
        uint32_t hit = 0, acc = 0, ghost = 0, bar = i / 16u;
        for (l = 0; l < 8u; l++) {                       /* (its 8 lanes of before: mapped below, lanes_from8) */
            char c = d->ln[l] ? d->ln[l][i % 16u] : '.';
            if (!d->ln[l] || c == '\0')
                continue;
            if (intro && bar < 2u && l != 0u && l != 3u)       /* (INTRO: the kick and the hats first) */
                continue;
            if (c == 'x' || (c == 'o' && g_pct(r, energy)) || c == 'a')
                hit |= 1u << l;
            if (c == 'a')
                acc |= 1u << l;
            if (c == 'g' && g_pct(r, gen.v[GK_COMPLEX]))
                ghost |= 1u << l;
        }
        if (fill && bar == 3u && (i % 16u) >= 12u - (energy > 60 ? 4u : 0u)) {   /* the fill: the snare, the toms */
            uint32_t f = g_n(r, 3);
            hit &= (1u << 0) | (1u << 3);
            hit |= f == 0 ? 1u << 1 : f == 1 ? 1u << 5 : (i & 1u) ? 1u << 1 : 1u << 5;
            if ((i % 16u) == 15u)
                acc |= hit;
        }
        if (!hit && !ghost)
            continue;
        w[i].time = ST_NOTE;
        w[i].hit = (uint16_t)lanes_from8(hit | ghost);   /* ((Vallée) GEN writes the old 8, mapped to SPAMPY lanes */
        w[i].acc = (uint16_t)lanes_from8(acc);
        w[i].vel = (uint8_t)clamp(g_vel(r, hit ? 100 : 46), 1, 127);
        if (gen.v[GK_COMPLEX] > 60 && ((hit >> 3) & 1u) && g_pct(r, gen.v[GK_COMPLEX] - 55))
            w[i].flags |= (uint8_t)(1u << SF_RATCH_SH);   /* (a hat doubled) */
        if (gen.v[GK_STYLE] % GS_N == GS_HEAD && (hit & 6u) && g_pct(r, 20 + gen.v[GK_COMPLEX] / 3))
            w[i].flags = (uint8_t)((w[i].flags & ~SF_RATCH) | (1u + g_n(r, 3)) << SF_RATCH_SH);   /* (HEAD: the snare,
                                                       * the clap stuttering: x2 .. x4) */
        if (gen.v[GK_STYLE] % GS_N == GS_HEAD && g_pct(r, 30))
            w[i].probability = (uint8_t)(55 + g_n(r, 40));   /* (HEAD: some hits come and go) */
        g_nudge(r, &w[i], g_sty()->human_late && ((hit >> 1) & 1u || (hit >> 6) & 1u));
    }
}

/* BASS (T1): the style's bar shapes on the chords, low */
static uint32_t g_bass_pitch(const gplan_t *p, uint32_t c, char sym, uint32_t next, grng_t *r)
{
    uint32_t root = (p->key + p->ch[c][0] + p->ch[c][2]) % 12u, b = 33u + (root + 3u) % 12u, nr;   /* A1 .. G#2 */
    if (sym == 'P')                                     /* (a push: the next chord's bass, early) */
        return 33u + ((p->key + p->ch[next][0] + p->ch[next][2]) % 12u + 3u) % 12u;
    uint8_t pc[5];
    if (sym == '5')
        return b + 7u;
    if (sym == 'O')
        return b + 12u;
    if (sym == 'C') {
        uint32_t n = g_pcs(p, c, pc), m = pc[1u + g_n(r, n - 1u)];
        return b + (m + 12u - root) % 12u;
    }
    if (sym == 'A') {                                   /* to the next chord's root: a step or a semitone below/above */
        nr = 33u + ((p->key + p->ch[next][0] + p->ch[next][2]) % 12u + 3u) % 12u;
        if (gen.v[GK_COMPLEX] > 40 && g_pct(r, 50))
            return g_n(r, 2) ? nr + 1u : nr - 1u;      /* (chromatic, from above or below) */
        return g_in(p, nr - 2u) ? nr - 2u : nr - 1u;   /* (a scale step below) */
    }
    return b;
}
static void g_bass(step_t *w, const gplan_t *p, grng_t *r, uint32_t amt, uint32_t part)
{
    const gstyle_t *s = g_sty();
    uint32_t bar, i, pick = g_n(r, 3);
    for (bar = 0; bar < 4u; bar++) {
        const char *t = s->bass[(pick + (bar == 3u && g_pct(r, 60))) % 3u];
        if (part == GP_INTRO && bar < 2u)
            continue;
        for (i = 0; i < 16u; i++) {
            uint32_t st = bar * 16u + i, c = g_chord_at(p, st), nx = g_chord_at(p, (st + 16u - st % 16u) % 64u);
            char sym = t[i];
            uint8_t m;
            if (sym == '-' && st && (w[st - 1u].time == ST_NOTE || w[st - 1u].time == ST_TIE)) {
                w_tie(&w[st]);
                continue;
            }
            if (sym == '.' || sym == '-')
                continue;
            if (amt == 1u && i % 4u && sym != 'A')      /* LITE: on the beats */
                continue;
            if ((sym == 'A' || sym == 'P') && i < 12u)  /* (an approach, a push: only near the bar's end) */
                sym = 'C';
            if (c != g_chord_at(p, st - st % 16u) && sym != 'A' && i % 8u == 0u)
                sym = 'R';                              /* (two chords a bar: the second's root on its beat) */
            m = (uint8_t)g_bass_pitch(p, c, sym, (sym == 'A' || sym == 'P') ? nx : c, r);
            w_note(&w[st], &m, 1, g_vel(r, i % 4u ? 92 : 108));
            g_nudge(r, &w[st], s->human_late);
        }
        if (amt == 3u && bar == 3u && g_pct(r, 70)) {   /* BUSY: a run to the top */
            uint8_t m = (uint8_t)g_bass_pitch(p, g_chord_at(p, 62), 'O', 0, r);
            w_note(&w[62], &m, 1, g_vel(r, 96));
        }
    }
}

/* KEYS (T2): the chord voiced and voice-led, in the style's comping shape */
static void g_keys(step_t *w, const gplan_t *p, grng_t *r, uint32_t amt, uint32_t part)
{
    const gstyle_t *s = g_sty();
    uint8_t prev[4] = {0}, v[4];
    uint32_t pn = 0, bar, i, pick = g_n(r, 3), lastc = 0xFFu, vn = 0, pushed = 0xFFu;
    int32_t lo = gen.v[GK_STYLE] % GS_N == GS_STROKE ? 50 : 53, hi = lo + 21;
    for (bar = 0; bar < 4u; bar++) {
        const char *t = s->comp[(pick + (part == GP_CHORUS && bar >= 2u)) % 3u];
        if (amt == 1u)
            t = s->comp[0];
        for (i = 0; i < 16u; i++) {
            uint32_t st = bar * 16u + i, c = g_chord_at(p, st);
            char sym = t[i];
            if (c != lastc && c == pushed) {            /* (pushed already: it holds into its bar) */
                lastc = c;
                pushed = 0xFFu;
                if (sym == 'X' || sym == 's')
                    sym = '-';
            } else if (sym == 'P' && i >= 12u) {        /* a push: the next bar's chord, early (the anticipation) */
                uint32_t nx = g_chord_at(p, (st / 16u + 1u) * 16u % 64u);
                if (nx != c && st / 16u < 3u) {
                    vn = g_voice(p, nx, prev, pn, v, lo, hi);
                    memcpy(prev, v, vn);
                    pn = vn;
                    pushed = nx;
                }
                sym = 'X';
            } else if (sym == 'P')
                sym = 's';
            if (c != lastc) {                           /* a new chord: voiced now, struck on its first step */
                vn = g_voice(p, c, prev, pn, v, lo, hi);
                memcpy(prev, v, vn);
                pn = vn;
                if ((sym == '-' || sym == '.') && i + 1u < 16u && (t[i + 1u] == 'X' || t[i + 1u] == 's'))
                    sym = '.';                          /* (the shape strikes it a step later: there) */
                else if (sym == '-' || sym == '.')
                    sym = 'X';
                lastc = c;
            }
            if (sym == '-' && st && (w[st - 1u].time == ST_NOTE || w[st - 1u].time == ST_TIE)) {
                w_tie(&w[st]);
                continue;
            }
            if (sym != 'X' && sym != 's')
                continue;
            w_note(&w[st], v, vn, g_vel(r, sym == 's' && i % 4u ? 84 : 98));
            if (sym == 's' && amt == 3u && i + 1u < 16u && t[i + 1u] == '.' && g_chord_at(p, st + 1u) == c &&
                pushed == 0xFFu && g_pct(r, 35)) {   /* (not over a new chord, nor a push) */
                w_note(&w[st + 1u], v, vn, g_vel(r, 70));   /* BUSY: a ghost stab after */
            }
            g_nudge(r, &w[st], s->human_late);
        }
    }
}

/* LEAD (T3): a motif from the seed (the same in every PART: the hook), two bars of the style's rhythm, its pitches a
 * walk on the scale pulled to the chord on the beats; bars 3 and 4 answer it and come home */
static void g_lead(step_t *w, const gplan_t *p, uint32_t amt, uint32_t part)
{
    const gstyle_t *s = g_sty();
    grng_t r = g_stream('L', part == GP_BRIDGE ? 7u : 0u), rv = g_stream('l', part);
    const char *rh = s->lead[g_n(&r, 3)];
    int32_t lo = s->lead_lo, hi = s->lead_hi, cur = (lo + hi) / 2, deg, i, j;
    uint8_t motif[32];
    uint32_t sty = gen.v[GK_STYLE] % GS_N, harm = sty == GS_STEELY;
    int riff = sty == GS_DAFT || sty == GS_LCD || sty == GS_STROKE;
    if (part == GP_INTRO && amt < 3u)
        return;
    for (i = 0; i < 32; i++) {                          /* the motif's pitches: a walk, the beats on chord tones */
        uint32_t c = g_chord_at(p, (uint32_t)i), stepd = g_n(&r, 7);
        deg = stepd < 3u ? 1 : stepd < 5u ? -1 : stepd == 5u ? 2 : -2;
        if (sty == GS_HEAD)                             /* (HEAD: a chant, a note or two, now and then a step) */
            deg = stepd < 5u ? 0 : stepd == 5u ? 2 : -2;
        if (riff && i >= 16)                            /* (a riff: the bar again) */
            deg = 0;
        cur += deg;
        while (cur > hi) cur -= 5;
        while (cur < lo) cur += 5;
        for (j = 0; j < 3 && !g_in(p, (uint32_t)cur); j++)
            cur++;
        if (i % 4 == 0)                                 /* the beat: the nearest chord tone */
            for (j = 0; j < 4; j++) {
                if (g_tone(p, c, (uint32_t)(cur + j))) { cur += j; break; }
                if (g_tone(p, c, (uint32_t)(cur - j))) { cur -= j; break; }
            }
        motif[i] = (uint8_t)(g_tone(p, c, (uint32_t)cur) && cur >= lo && cur <= hi ? cur : g_snap(p, cur, lo, hi));
        cur = motif[i];
        if (riff && i >= 16)
            motif[i] = motif[i - 16];
    }
    for (i = 0; i < 64; i++) {
        uint32_t k = (uint32_t)i % 32u, c = g_chord_at(p, (uint32_t)i);
        char sym = rh[k];
        uint8_t m[2];
        if (sym == '-' && i && (w[i - 1].time == ST_NOTE || w[i - 1].time == ST_TIE)) {
            w_tie(&w[i]);
            continue;
        }
        if (sym != 'n')
            continue;
        if ((part == GP_VERSE || part == GP_OUTRO || amt == 1u) && g_pct(&rv, 35) && k % 4u)
            continue;                                   /* (lighter in the verse) */
        if (part == GP_OUTRO && i >= 48)
            break;
        m[0] = motif[k];
        if (i >= 32 && !riff) {                         /* the answer: the same shape, its pitches moved to fit */
            if (!g_tone(p, c, m[0]) && (k % 4u == 0u || !g_in(p, m[0])))
                for (j = 1; j < 4; j++) {
                    if (g_tone(p, c, (uint32_t)m[0] + (uint32_t)j)) { m[0] = (uint8_t)(m[0] + j); break; }
                    if (g_tone(p, c, (uint32_t)m[0] - (uint32_t)j)) { m[0] = (uint8_t)(m[0] - j); break; }
                }
            if (i >= 56) {                              /* the end: home, the key's root or third */
                int32_t t = m[0];
                for (j = 0; j < 7; j++)
                    if ((uint32_t)(t - j) % 12u == p->key) { t -= j; break; }
                m[0] = (uint8_t)g_snap(p, t, lo, hi);
            }
        }
        if (harm) {                                     /* STEELY: horns in two parts, a chord tone under */
            int32_t u = m[0] - 3;
            while (u > m[0] - 6 && !g_tone(p, c, (uint32_t)u))
                u--;
            m[1] = (uint8_t)u;
        }
        w_note(&w[i], m, harm ? 2u : 1u, g_vel(&rv, k % 4u ? 88 : 104));
        g_nudge(&rv, &w[i], s->human_late);
    }
}

/* COLOR (T5): an arp of the chord, a jangle, a pad, the robot's syllables, a texture, guitar fills, bells */
static void g_color(step_t *w, const gplan_t *p, grng_t *r, uint32_t amt, uint32_t part)
{
    const gstyle_t *s = g_sty();
    uint32_t i, kind = s->color, pick = g_n(r, 4);
    uint8_t pc[5], v[4];
    if (part == GP_INTRO && amt < 2u)
        return;
    for (i = 0; i < 64u; i++) {
        uint32_t c = g_chord_at(p, i), n = g_pcs(p, c, pc), k;
        uint8_t m;
        switch (kind) {
        case XC_ARP16:                                  /* up and down the chord, sixteenths (LITE: eighths) */
            if (amt == 1u && (i & 1u))
                break;
            k = (i + pick) % (2u * n - 2u > 0u ? 2u * n - 2u : 1u);
            k = k < n ? k : 2u * n - 2u - k;
            m = (uint8_t)(64u + (pc[k] + 12u - 4u) % 12u + (k >= 2u && pick & 1u ? 12u : 0u));
            w_note(&w[i], &m, 1, g_vel(r, i % 4u ? 78 : 96));
            break;
        case XC_JANGLE:                                 /* root, fifth, third, fifth: eighths, mid-high */
            if (i & 1u || (amt == 1u && i % 4u))
                break;
            k = ((i / 2u) % 4u == 0u) ? 0u : ((i / 2u) % 4u == 2u) ? 1u % n : n > 2u ? 2u : 0u;
            m = (uint8_t)(60u + (pc[k] + 12u - 0u) % 12u + ((i / 2u) % 4u == 3u ? 12u : 0u));
            w_note(&w[i], &m, 1, g_vel(r, 84));
            break;
        case XC_PAD:                                    /* the chord held, a bar at a time */
            if (i % 16u == 0u || c != g_chord_at(p, i - 1u)) {
                uint32_t vn = g_voice(p, c, 0, 0, v, 60, 84);
                w_note(&w[i], v, vn, g_vel(r, 80));
            } else
                w_tie(&w[i]);
            break;
        case XC_ROBOT:                                  /* short syllables on the root and fifth, offbeat */
            if (!((0x2492u >> (i % 16u)) & 1u) || (amt == 1u && g_pct(r, 50)))
                break;
            m = (uint8_t)(60u + (pc[g_n(r, n > 2u ? 3u : n)] + 12u - 0u) % 12u);
            w_note(&w[i], &m, 1, g_vel(r, 96));
            break;
        case XC_TEXTURE:                                /* long notes now and then, low */
            if (i % 16u == 0u && g_pct(r, amt == 3u ? 90 : 60)) {
                m = (uint8_t)(48u + pc[g_n(r, n)]);
                w_note(&w[i], &m, 1, g_vel(r, 70));
                w[i].probability = (uint8_t)(60 + g_n(r, 40));
            } else if (i && (w[i - 1u].time == ST_NOTE || w[i - 1u].time == ST_TIE) && i % 16u < 12u)
                w_tie(&w[i]);
            break;
        case XC_FILLS:                                  /* a lick at the bar's end: four sixteenths down the chord */
            if (i % 16u >= 12u && (i / 16u) % 2u == 1u && (amt > 1u || i >= 48u)) {
                k = 3u - (i % 16u - 12u);
                m = (uint8_t)(64u + pc[k % n]);
                w_note(&w[i], &m, 1, g_vel(r, 86));
            }
            break;
        default:                                        /* XC_BELLS: (ENO) its own loop: below */
            break;
        }
    }
}
/* ENO: the bells and the second voice play loops of their own odd lengths over the chords: a note or two each */
static const uint8_t ENO_LL[4] = {47, 53, 59, 61}, ENO_CL[4] = {29, 37, 41, 43};   /* ((Vallée) the seed picks */
static uint32_t g_eno_loop(step_t *w, const gplan_t *p, grng_t *r, uint32_t len, uint32_t notes, int32_t lo)
{
    uint32_t i, k;
    for (k = 0; k < notes; k++) {
        uint32_t at = g_n(r, len - 6u), c = g_chord_at(p, at * 64u / len);
        uint8_t pc[5], m;
        uint32_t n = g_pcs(p, c, pc);
        m = (uint8_t)(lo + (int32_t)((pc[g_n(r, n)] + 12u - (uint32_t)lo % 12u) % 12u));
        if (w[at].time == ST_NOTE)
            continue;
        w_note(&w[at], &m, 1, g_vel(r, 76));
        for (i = 1; i < 4u + g_n(r, 4) && at + i < len && w[at + i].time != ST_NOTE; i++)
            w_tie(&w[at + i]);
    }
    return len;
}

/* --------------------------------------------------------- sounds and mix --- */
static uint32_t g_preset(uint32_t e, const char *name)
{
    const engine_t *en = ENGINES[e % NENGINES];
    uint32_t i;
    for (i = 0; i < en->npresets; i++)
        if (str_eq(en->presets[i].name, name))
            return i;
    return 0;
}
/* the role's sound: one of the style's (the seed's pick), a light track's only from what it may play */
static void g_sound(track_t *t, uint32_t role, grng_t *r)
{
    const gstyle_t *s = g_sty();
    uint32_t k = trk_index(t), n = 0, i, pick;
    const gsnd_t *c[3];
    for (i = 0; i < 3u; i++)
        if (s->snd[role][i].pre && eng_allowed(k, s->snd[role][i].eng))
            c[n++] = &s->snd[role][i];
    if (!n)
        return;
    {   /* ((Vallée) the first, the style's own sound (its vintage synths, its kits, listed first): twice as likely as
         * each other. The stream's value mixed first (a finalizer): the roles' streams start close together, and
         * their low bits ran alike: the bass and the lead always took the same place, never MOOG BASS with JUNO PAD) */
        uint32_t h = g_next(r) ^ (role + 1u) * 0x9E3779B9u;
        h ^= h >> 16; h *= 0x85EBCA6Bu; h ^= h >> 13; h *= 0xC2B2AE35u; h ^= h >> 16;
        pick = h % (n + 1u);
        if (pick >= n)
            pick = 0;
    }
    if (t->eng_req % NENGINES != c[pick]->eng)
        set_engine_of(t, c[pick]->eng);
    apply_preset_to(t, g_preset(c[pick]->eng, c[pick]->pre));
}
static void g_mix(track_t *t, uint32_t role)
{
    const gstyle_t *s = g_sty();
    static const uint8_t LEVEL[GR_NROLE] = {104, 92, 96, 108, 84, 88};
    static const int8_t PAN[GR_NROLE] = {0, -14, 10, 0, 22, -24};
    t->p[P_LEVEL] = LEVEL[role];
    t->p[P_PAN] = PAN[role];
    t->p[P_REV] = s->rev[role];
    t->p[P_DLY] = s->dly[role];
    t->p[P_CHOR] = (int16_t)(role == GR_KEYS && s->wobble ? 50 : role == GR_KEYS || role == GR_COLOR ? 20 : 0);
    t->p[P_ITYPE] = (int16_t)(role == GR_KEYS ? s->ins_keys : 0);
    if (role == GR_KEYS && s->ins_keys) {
        t->p[P_IA] = 40;
        t->p[P_IB] = 70;
        t->p[P_IC] = 60;
    }
    t->p[P_LD_PIT] = (int16_t)((role == GR_KEYS || role == GR_LEAD) && s->wobble ? 2 : 0);
    if ((role == GR_KEYS || role == GR_LEAD) && s->wobble) {
        t->p[P_LRATE] = 34;
        t->p[P_LWAVE] = 0;
    }
    t->p[P_DKSRC] = (int16_t)(s->duck && (role == GR_BASS || role == GR_KEYS || role == GR_COLOR) ? 1 + GR_DRUMS : 0);
    if (t->p[P_DKSRC]) {
        t->p[P_DKAMT] = 80;
        t->p[P_DKREL] = 60;
    }
    if (role == GR_BASS)
        t->p[P_VOICE] = V_LEGATO;
    if (role == GR_LEAD && gen.v[GK_STYLE] % GS_N == GS_HEAD)
        t->p[P_GLIDE] = 40;
}

/* ----------------------------------------------------------------- writing --- */
/* the undo copy of track k as it is (load_begin's, always a new one: SAVE held puts it back, "GEN") */
static void gen_copy(track_t *t)
{
    uint32_t i = trk_index(t);
    undo_label = "GEN";
    undo_push(UNDO_SOUND | UNDO_PAT);
    undo.trk = (uint8_t)(i + 1u);
    undo.what = UNDO_SOUND | UNDO_PAT;
    undo.eng = t->eng_req;
    undo.preset = t->preset;
    undo.user = t->user;
    memcpy(undo.p, t->p, sizeof undo.p);
    memcpy(undo.step, t->step, sizeof undo.step);
    memcpy(undo.fm6, fm6_patch[i], FP_SIZE);
    undo.fm6_slot = fm6_slot[i];
    undo.pid = trk_pid[i];
    motion_restore(t);
    motion_snapshot_track(t, &undo.motion_backup);
    undo.keep = 0;
    undo.after = 0;
    undo.t_ms = fm1_ms;
    if (gen.nseq < NTRK)
        gen.useq[gen.nseq++] = undo.seq;
}
/* GEN's copies are the newest, none undone: OCT- can put them all back */
static int gen_copies_top(void)
{
    uint32_t a, j, ok;
    if (!gen.nseq || undo_back || gen.nseq > undo_n)
        return 0;
    for (a = 0; a < gen.nseq; a++) {
        uint16_t sq = undo_ring[(undo_top + UNDO_N - a) % UNDO_N].seq;
        for (ok = 0, j = 0; j < gen.nseq; j++)
            ok |= gen.useq[j] == sq;
        if (!ok)
            return 0;
    }
    return 1;
}
static int gen_role_on(uint32_t role)
{
    static const uint8_t K[GR_NROLE] = {GK_BASS, GK_KEYS, GK_LEAD, GK_DRUMS, GK_COLOR, GK_DRUMS};
    return gen.v[K[role]] != 0;
}
static uint32_t gen_amt(uint32_t role)
{
    static const uint8_t K[GR_NROLE] = {GK_BASS, GK_KEYS, GK_LEAD, GK_DRUMS, GK_COLOR, GK_DRUMS};
    return gen.v[K[role]] & 3u;
}

/* (Vallée) role's steps for part (GEN's knobs as they are, plan p) into w: their length (gen_write; THE SPELL's
 * re-roll, gen_part_steps) */
static step_t gen_w[NSTEP] __attribute__((section(".pool")));   /* ((Vallée) the pool: .bss is tight with 16 lanes) */
static uint32_t gen_role_steps(uint32_t role, uint32_t part, const gplan_t *p, step_t *w)
{
    const gstyle_t *s = g_sty();
    uint32_t sty = gen.v[GK_STYLE] % GS_N, amt = gen_amt(role), len = 64;
    int32_t energy = gen.v[GK_ENERGY] * (part == GP_CHORUS ? 120 : part == GP_VERSE || part == GP_OUTRO ? 85 :
                                         part == GP_INTRO ? 60 : 100) / 100;
    grng_t r = g_stream('D' + role * 7u, role == GR_LEAD ? 0u : part);
    w_clear(w, NSTEP);
    switch (role) {
    case GR_BASS: g_bass(w, p, &r, amt, part); break;
    case GR_KEYS: g_keys(w, p, &r, amt, part); break;
    case GR_LEAD:
        if (sty == GS_ENO)                              /* ((Vallée) the loops' odd lengths the seed's) */
            len = g_eno_loop(w, p, &r, ENO_LL[g_n(&r, 4)], 2u + amt, s->lead_lo);
        else
            g_lead(w, p, amt, part);
        break;
    case GR_DRUMS:
        if (sty != GS_ENO || energy > 50)
            g_drums(w, &s->drum[part == GP_CHORUS || (part == GP_LOOP && energy > 70) || amt == 3u],
                    &r, energy, part != GP_INTRO && part != GP_OUTRO && amt > 1u, 64, part == GP_INTRO);
        break;
    case GR_COLOR:
        if (sty == GS_ENO)
            len = g_eno_loop(w, p, &r, ENO_CL[g_n(&r, 4)], 2u + amt, 72);
        else
            g_color(w, p, &r, amt, part);
        break;
    default:                                            /* PERC: a second kit; HEAD's an odd length */
        if (sty == GS_HEAD)
            len = 12u + 2u * g_n(&r, 2);
        if (amt > 1u || part == GP_CHORUS)
            g_drums(w, s->perc, &r, energy, 0, len, part == GP_INTRO);
        break;
    }
    return len == 64u ? p->bars * 16u : len;            /* ((Vallée) MINE: as many bars as the part's chords) */
}

/* write the piece into the tracks */
static void gen_write(void)
{
    {                                                   /* (the knobs inside their ends, whatever came before) */
        uint32_t k;
        for (k = 0; k < GK_N; k++)
            if (gen.v[k] > GEN_MAX[k])
                gen.v[k] = GEN_MAX[k];
    }
    step_t *w = gen_w;
    const gstyle_t *s = g_sty();
    gplan_t p;
    uint32_t role, part = gen.v[GK_PART] % GP_N, sty = gen.v[GK_STYLE] % GS_N, keep = !gen_copies_top(), i;
    gen_plan(&p);
    if (keep) {                                         /* a fresh GEN: the copies, the globals as they were */
        gen.nseq = 0;
        for (i = 0; i < NELEM(GEN_G); i++)
            gen.g_was[i] = song.g[GEN_G[i]];
    }
    undo_depth++;                                       /* (the sound loads below take no copies of their own) */
    for (role = 0; role < GR_NROLE; role++) {
        track_t *t = &trk[role];
        grng_t rs = g_stream('S' + role, 0);
        uint32_t len, j, have;
        if (!gen_role_on(role))
            continue;
        for (have = 0, j = 0; j < gen.nseq; j++)       /* (a copy of this track already: OCT+ again) */
            have |= undo_find(gen.useq[j]) >= 0 && undo_ring[undo_find(gen.useq[j])].trk == role + 1u;
        if (!have) {
            undo_depth--;
            gen_copy(t);
            undo_depth++;
        }
        if (!gen.v[GK_SOUNDS]) {                        /* SOUNDS NEW */
            g_sound(t, role, &rs);
            g_mix(t, role);
        }
        len = gen_role_steps(role, part, &p, w);
        fm1_irq_off();                                  /* the audio ISR sees the old pattern or the new one */
        memcpy(t->step, w, sizeof t->step);
        t->p[P_SLEN] = (int16_t)len;
        t->p[P_SDIV] = (int16_t)(sty == GS_ENO ? 1 : 2);   /* (ENO: eighths, slow) */
        t->p[P_SSWING] = 0;
        t->p[P_MUTE] = 0;
        if (role != GR_DRUMS && role != GR_PERC) {
            t->p[P_ROOT] = p.key;
            t->p[P_SCALE] = p.scale;
        }
        fm1_irq_on();
        motion_reset(t);
        panic_req |= (uint8_t)(1u << role);
        if (t == TSEL)
            sync_reload = 1;
    }
    undo_depth--;
    song.g[G_BPM] = p.bpm;
    song.g[G_SWING] = p.swing;
    if (!gen.v[GK_SOUNDS]) {                            /* the buses */
        song.g[G_RTYPE] = s->rtype;
        song.g[G_RSIZE] = s->rsize;
        song.g[G_DTIME] = s->dtime;
        song.g[G_DFDBK] = s->dfdbk;
        song.g[G_DMIX] = s->dmix;
        song.g[G_CMODE] = s->cmode;
        if (sty == GS_ENO) {                            /* ((Vallée) AKASHA: the seed's room: the hall as before, a close
                                                         * dry one, or the delay leading) */
            grng_t rr = g_stream('R', 0);
            uint32_t sp = g_n(&rr, 3);
            if (sp == 1u) {
                song.g[G_RSIZE] = 64; song.g[G_DMIX] = 30;
            } else if (sp == 2u) {
                song.g[G_RSIZE] = 96; song.g[G_DTIME] = 12; song.g[G_DFDBK] = 72; song.g[G_DMIX] = 80;
                /* (TIME 12: 1/4D, params.c N_DTIME's last; it was 100, past its end: the delay played 8T while
                 * the card said 1/4D, and a project saved and loaded again came back as 1/4D) */
            }
        }
    }
    gen.made = 1;
    gen.dirty = 0;
    ui.force = 1;
}

/* OCT+: write (again: the next seed, when nothing was turned) */
/* ---- (Vallée) PART SONG: the five parts written one after another, each track's kept as a pattern of its own
 * (bank.c), and each track's list made the song: INTRO VERSE CHORUS VERSE CHORUS BRIDGE CHORUS OUTRO, SONG ONCE. A
 * track whose role is OFF keeps its list and its pattern. OCT- puts the lists, the tracks and the pool back ---- */
static uint32_t gen_pool_free(void);                    /* (bank.c) */
static uint32_t gen_slot_held(uint32_t sl);             /* (bank.c) */
static int gen_keep_trk(uint32_t k, uint8_t *id);
static int gen_flush(void);
static int gen_load(uint32_t k, uint32_t id);
static void gen_unkeep(uint32_t id);
static int gen_bind(uint32_t k, uint8_t *id);
static uint32_t gen_pend_free(void);
static int trk_keep(uint32_t k);
#define GEN_NPART 5u                                    /* INTRO .. OUTRO */
#define GEN_NOID 0xFFu                                  /* (no pattern) */
static const uint8_t GEN_ARR[8] = {GP_INTRO, GP_VERSE, GP_CHORUS, GP_VERSE, GP_CHORUS, GP_BRIDGE, GP_CHORUS, GP_OUTRO};
static uint8_t gen_sid[GR_NROLE][GEN_NPART];            /* each part's pattern a track (.bss: the pool is full) */
static uint8_t gen_lwas[sizeof lists];                  /* the lists as they were */
/* ((Vallée) the song's arrangement, each entry a part (0 INTRO .. 4 OUTRO): SONG's GEN_ARR, or the parts written one by
 * one, each once, in a song's order; the tracks it holds (their roles ON when it began) */
static uint8_t gen_arr[NENT], gen_narr, gen_smask;
/* ((Vallée) THE SPELL's re-roll: the knobs and seed the song was written with, each part's take (0 the first), the
 * lists' fingerprint then (another song since, or the lists edited: not GEN's song any more), each track's bar still
 * to write (+ 1, gen_rr_poll) and its part) */
static uint8_t gen_sv[GK_N], gen_pvar[GEN_NPART], gen_rr[NTRK], gen_rr_pi[NTRK];
static uint16_t gen_sseed;
static uint32_t gen_lsig;
static uint32_t gen_lists_sig(void)
{
    uint32_t i, h = 0x811C9DC5u;
    const uint8_t *b = &lists.ent[0][0][0];
    for (i = 0; i < sizeof lists.ent; i++)
        h = (h ^ b[i]) * 16777619u;
    return h;
}
static void gen_lists_signed(void)                      /* (THE SPELL's REC ARRANGE: the same patterns, a new order) */
{
    if (gen.song)
        gen_lsig = gen_lists_sig();
}
static void gen_pid_back(void)                          /* each track's pattern id as GEN's copy of it has it */
{
    uint32_t j;
    for (j = 0; j < gen.nseq; j++) {
        int32_t i = undo_find(gen.useq[j]);
        if (i >= 0 && (undo_ring[i].what & UNDO_PAT))
            trk_pid[(undo_ring[i].trk - 1u) % NTRK] = undo_ring[i].pid;
    }
}
static void gen_song_drop(void)                         /* the song's lists and patterns gone (its copies stay) */
{
    uint32_t k, p;
    memset(gen_rr, 0, sizeof gen_rr);
    fm1_irq_off();
    memcpy(&lists, gen_lwas, sizeof lists);
    fm1_irq_on();
    gen_pid_back();
    for (k = 0; k < GR_NROLE; k++)
        for (p = 0; p < GEN_NPART; p++)
            if (gen_sid[k][p] != GEN_NOID)
                gen_unkeep(gen_sid[k][p]);
    memset(gen_sid, GEN_NOID, sizeof gen_sid);
    (void)gen_flush();
    gen.song = 0;
    gen_narr = 0;
}
static uint32_t gen_on_mask(void)
{
    uint32_t k, m = 0;
    for (k = 0; k < GR_NROLE; k++)
        if (gen_role_on(k))
            m |= 1u << k;
    return m;
}
/* each track of the song: its list the arrangement (cur: the entry of part pi, the first), its patterns gen_sid */
static void gen_lists_make(uint32_t pi)
{
    uint32_t k, e, at = 0;
    for (e = 0; e < gen_narr; e++)
        if (gen_arr[e] == pi) {
            at = e;
            break;
        }
    fm1_irq_off();
    for (k = 0; k < GR_NROLE; k++) {
        if (!((gen_smask >> k) & 1u))
            continue;
        for (e = 0; e < NENT; e++) {
            uint32_t id = e < gen_narr ? gen_sid[k][gen_arr[e] % GEN_NPART] : GEN_NOID;
            lists.ent[k][e][0] = (uint8_t)(id != GEN_NOID ? id + 1u : 0u);
            lists.ent[k][e][1] = 0;
            lists.snd[k][e][0] = lists.snd[k][e][1] = 0;
        }
        lists.cur[k] = (uint8_t)at;
    }
    fm1_irq_on();
    gen_lsig = gen_lists_sig();
}
static int gen_is_song(void);
/* may a part written on its own go into a song: the lists GEN's song (the same tracks), or none yet (every track's
 * list no more than its first entry) */
static int gen_part_song_ok(void)
{
    uint32_t k, e;
    if (!gen_on_mask())
        return 0;
    if (gen.song && gen_is_song())
        return gen_smask == gen_on_mask();
    for (k = 0; k < NTRK; k++)
        for (e = 1; e < NENT; e++)
            if (lists.ent[k][e][0])
                return 0;
    return 1;
}
/* ((Vallée) a part written on its own (OCT+ on INTRO .. OUTRO): written, each track's part kept as a pattern of its
 * own and the song's lists made of the parts so far, each once in a song's order (a SONG's: its arrangement as it
 * was). Playing: on, unless SONG is (a new part, or the song playing: stopped first). 0 written; 1 not (said) */
static int gen_part_add(uint32_t pi)
{
    uint32_t k, e, fresh = !(gen.song && gen_is_song()), in = 0, need = 0, n = 0;
    for (e = 0; e < gen_narr && !fresh; e++)
        in |= gen_arr[e] == pi;
    for (k = 0; k < GR_NROLE; k++)
        if (gen_role_on(k)) {
            n++;
            need += fresh || gen_sid[k][pi] == GEN_NOID;
        }
    if (transport_busy() && (lists.song != SONG_OFF || gen_pend_free() < n))
        transport_stop_now();
    bank_poll();
    if (gen_pool_free() < need && pool_unused() && !transport_busy())
        pool_clear_unused();
    if (gen_pool_free() < need) {
        ui_way("PATTERNS FULL", WAY_PATTERNS);
        return 1;
    }
    if (fresh) {                                        /* (a new song: the lists as they were, for OCT-) */
        memcpy(gen_lwas, &lists, sizeof lists);
        memset(gen_sid, GEN_NOID, sizeof gen_sid);
        memset(gen_pvar, 0, sizeof gen_pvar);
        memset(gen_rr, 0, sizeof gen_rr);
        gen_narr = 0;
        gen_smask = (uint8_t)gen_on_mask();
    }
    for (k = 0; k < GR_NROLE; k++)                      /* (a track's own edits kept where they were first) */
        if (gen_role_on(k) && trk_pid[k])
            (void)trk_keep(k);
    gen_write();
    gen.song = 1;
    memcpy(gen_sv, gen.v, sizeof gen_sv);
    gen_sseed = gen.seed;
    for (k = 0; k < GR_NROLE; k++)
        if (gen_role_on(k) && gen_bind(k, &gen_sid[k][pi])) {
            ui_way("PATTERNS FULL", WAY_PATTERNS);
            return 1;
        }
    if (!transport_busy())
        (void)gen_flush();
    if (!in) {                                          /* (a new part: in a song's order) */
        for (e = gen_narr; e > 0u && gen_arr[e - 1u] > pi; e--)
            gen_arr[e] = gen_arr[e - 1u];
        gen_arr[e] = (uint8_t)pi;
        gen_narr++;
    }
    gen_lists_make(pi);
    ui.force = 1;
    return 0;
}
static int gen_song(void)                               /* 0 written; 1 not (said) */
{
    uint32_t k, p, e, n = 0;
    char b[36];
    for (k = 0; k < GR_NROLE; k++)
        n += gen_role_on(k) != 0;
    if (!n) {
        ui_say("NOTHING ", "TO WRITE");
        return 1;
    }
    if (transport_busy())
        transport_stop_now();
    bank_poll();
    if (gen.song && gen.made && gen_copies_top())
        gen_song_drop();                                /* (OCT+ again: the last song's patterns given back first) */
    else {
        gen.song = 0;
        memcpy(gen_lwas, &lists, sizeof lists);
        memset(gen_sid, GEN_NOID, sizeof gen_sid);
    }
    if (gen_pool_free() < GEN_NPART * n && pool_unused())
        pool_clear_unused();                            /* (short: first the patterns nothing uses, as SAVE > PATTERNS'
                                                         * CLEAR UNUSED: a project left, an old song; saved ones stay) */
    if (gen_pool_free() < GEN_NPART * n) {               /* (what holds them: the saved projects, by slot: SAVE >
                                                         * PROJECT's DELETE frees them; else how many it needs) */
        uint32_t sl, h, at = 0;
        b[0] = 0;
        for (sl = 0; sl < 4u; sl++)
            if ((h = gen_slot_held(sl)) != 0u && at < sizeof b - 7u) {
                if (!at)
                    str_cpy(b, "SAVED SLOT", sizeof b);
                at = str_len(b);
                b[at++] = ' ';
                b[at++] = (char)('1' + sl);
                b[at++] = ':';
                fmt_int(b + at, (int32_t)h);
                at = str_len(b);
            }
        if (!at) {
            str_cpy(b, "NEED ", sizeof b);
            fmt_int(b + 5, (int32_t)(GEN_NPART * n));
            str_cpy(b + str_len(b), " FREE", sizeof b - str_len(b));
        }
        ui_way("PATTERNS FULL", b);
        return 1;
    }
    gen.song = 1;                                       /* (from here OCT- gives back what was made) */
    memcpy(gen_sv, gen.v, sizeof gen_sv);               /* (THE SPELL's re-roll: what it was written with) */
    gen_sseed = gen.seed;
    memset(gen_pvar, 0, sizeof gen_pvar);
    memset(gen_rr, 0, sizeof gen_rr);
    for (p = 0; p < GEN_NPART; p++) {
        gen.v[GK_PART] = (uint8_t)(GP_INTRO + p);
        gen_write();
        for (k = 0; k < GR_NROLE; k++)
            if (gen_role_on(k) && gen_keep_trk(k, &gen_sid[k][p])) {
                gen.v[GK_PART] = GP_SONG;
                ui_way("PATTERNS FULL", WAY_PATTERNS);
                return 1;
            }
    }
    gen.v[GK_PART] = GP_SONG;
    if (gen_flush()) {
        ui_way("STORAGE ERROR", "");
        return 1;
    }
    for (e = 0; e < NELEM(GEN_ARR); e++)               /* ((Vallée) the arrangement: gen_lists_make) */
        gen_arr[e] = (uint8_t)(GEN_ARR[e] - GP_INTRO);
    gen_narr = (uint8_t)NELEM(GEN_ARR);
    gen_smask = (uint8_t)gen_on_mask();
    gen_lists_make(0);
    lists.song = SONG_ONCE;
    for (k = 0; k < GR_NROLE; k++)
        if (gen_role_on(k))
            (void)gen_load(k, gen_sid[k][0]);           /* (the tracks on the INTRO) */
    ui.force = 1;
    return 0;
}

/* ---- (Vallée) THE SPELL's re-roll: a part of GEN's song written anew while it plays (ui_world.c wl_keys). Only GEN's
 * song as it was written (its patterns in the lists, the same lists); the plan stays (key, tempo, chords), so do the
 * sounds; the parts' writers take the part's next take (g_vary). The tracks on that part now get it: every bar but
 * the one playing at once, that one once it has gone by (gen_rr_poll), so it always comes in on a bar line ---- */
static int gen_is_song(void) { return gen.song && gen_lists_sig() == gen_lsig; }
/* the part (0 INTRO .. 4 OUTRO) GEN's song is on: the one most tracks' patterns are; -1 none */
static int32_t gen_part_now(void)
{
    uint32_t pi, k, n, best = 0;
    int32_t at = -1;
    if (!gen_is_song())
        return -1;
    for (pi = 0; pi < GEN_NPART; pi++) {
        for (n = 0, k = 0; k < GR_NROLE; k++)
            n += gen_sid[k][pi] != GEN_NOID && trk_pid[k] == gen_sid[k][pi] + 1u;
        if (n > best) {
            best = n;
            at = (int32_t)pi;
        }
    }
    return at;
}
/* role's steps for part pi of the song, its take now, as written: into gen_w, their length */
static uint32_t gen_part_steps(uint32_t pi, uint32_t role)
{
    uint8_t v0[GK_N];
    uint16_t s0 = gen.seed;
    gplan_t pl;
    uint32_t len;
    memcpy(v0, gen.v, sizeof v0);
    memcpy(gen.v, gen_sv, sizeof gen.v);
    gen.seed = gen_sseed;
    gen.v[GK_PART] = (uint8_t)(GP_INTRO + pi % GEN_NPART);
    gen_plan(&pl);
    g_vary = gen_pvar[pi % GEN_NPART];
    len = gen_role_steps(role, GP_INTRO + pi % GEN_NPART, &pl, gen_w);
    g_vary = 0;
    memcpy(gen.v, v0, sizeof gen.v);
    gen.seed = s0;
    return len;
}
/* gen_w into track role: all but its bar playing (16 steps; a pattern of another length, or stopped: all) */
static void gen_live_put(uint32_t role, uint32_t len, uint32_t pi)
{
    track_t *t = &trk[role % NTRK];
    uint32_t b = NSTEP, i;
    if (song.playing && len == NSTEP && t->p[P_SLEN] == NSTEP)
        b = (t->seq_idx % NSTEP) / 16u * 16u;
    fm1_irq_off();                                      /* the audio ISR sees the old steps or the new ones */
    for (i = 0; i < NSTEP; i++)
        if (i < b || i >= b + 16u)
            t->step[i] = gen_w[i];
    t->p[P_SLEN] = (int16_t)len;
    fm1_irq_on();
    gen_rr[role % NTRK] = (uint8_t)(b < NSTEP ? b / 16u + 1u : 0u);
    gen_rr_pi[role % NTRK] = (uint8_t)pi;
    if (t == TSEL)
        sync_reload = 1;
}
/* part pi written anew for the tracks in mask on it now: 0 done, 1 not GEN's song, 2 none of them on it */
static int gen_reroll(uint32_t pi, uint32_t mask)
{
    uint32_t role, n = 0, len;
    if (!gen_is_song() || pi >= GEN_NPART)
        return 1;
    gen_pvar[pi]++;
    for (role = 0; role < GR_NROLE; role++) {
        if (!((mask >> role) & 1u) || gen_sid[role][pi] == GEN_NOID || trk_pid[role] != gen_sid[role][pi] + 1u)
            continue;
        len = gen_part_steps(pi, role);
        gen_live_put(role, len, pi);
        n++;
    }
    if (!n) {
        gen_pvar[pi]--;
        return 2;
    }
    return 0;
}
static uint32_t gen_take(uint32_t pi) { return gen_pvar[pi % GEN_NPART] + 1u; }
static const char *gen_part_name(uint32_t pi) { return GEN_PART[GP_INTRO + pi % GEN_NPART]; }
/* entry e of the lists: the part of GEN's song its patterns are (every track's that has one there): 0 .. 4; -1 not one */
static int32_t gen_part_of(uint32_t e)
{
    uint32_t k, pi, id;
    int32_t at = -1;
    if (!gen_is_song())
        return -1;
    for (k = 0; k < GR_NROLE; k++) {
        if (!(id = lists.ent[k][e % NENT][0]))
            continue;
        for (pi = 0; pi < GEN_NPART && gen_sid[k][pi] + 1u != id; pi++)
            ;
        if (pi < GEN_NPART) {
            if (at >= 0 && at != (int32_t)pi)
                return -1;
            at = (int32_t)pi;
        }
    }
    return at;
}
/* each frame (ui_input): a re-rolled track's bar that was playing, written once it has gone by (the track on that
 * part again: the song moved on before, it comes back to it) */
static void gen_rr_poll(void)
{
    uint32_t role, b, pi;
    for (role = 0; role < NTRK; role++) {
        if (!gen_rr[role])
            continue;
        b = gen_rr[role] - 1u;
        pi = gen_rr_pi[role];
        if (!gen_is_song()) {
            gen_rr[role] = 0;
            continue;
        }
        if (trk_pid[role] != gen_sid[role][pi] + 1u || (song.playing && (trk[role].seq_idx % NSTEP) / 16u == b))
            continue;
        (void)gen_part_steps(pi, role);
        fm1_irq_off();
        memcpy(&trk[role].step[b * 16u], &gen_w[b * 16u], 16u * sizeof(step_t));
        fm1_irq_on();
        gen_rr[role] = 0;
        if (&trk[role] == TSEL)
            sync_reload = 1;
    }
}

static void gen_oct(void)
{
    char b[24];
    if (gen.made && !gen.dirty)
        gen.seed = (uint16_t)(gen.seed % 999u + 1u);
    if (gen.v[GK_PART] % GP_N == GP_SONG) {
        if (!gen_song()) {
            str_cpy(b, GEN_STYLE[gen.v[GK_STYLE] % GS_N], sizeof b);
            str_cpy(b + str_len(b), " SONG #", sizeof b - str_len(b));
            fmt_int(b + str_len(b), gen.seed);
            ui_say(b, "  PLAY");
        }
        return;
    }
    {                                                   /* ((Vallée) INTRO .. OUTRO: into the song, part by part */
        uint32_t part = gen.v[GK_PART] % GP_N;
        if (part >= GP_INTRO && part <= GP_OUTRO && gen_part_song_ok()) {
            if (!gen_part_add(part - GP_INTRO)) {
                str_cpy(b, GEN_PART[part], sizeof b);
                str_cpy(b + str_len(b), " #", sizeof b - str_len(b));
                fmt_int(b + str_len(b), gen.seed);
                ui_say(b, " IN SONG");
            }
            return;
        }
    }
    if (gen.song && gen_is_song()) {                    /* (LOOP, or other tracks, over GEN's song: its parts kept, */
        uint32_t k;                                     /* the tracks off them, so nothing is written over a part) */
        for (k = 0; k < GR_NROLE; k++)
            if (gen_role_on(k) && trk_pid[k])
                (void)trk_keep(k);
        for (k = 0; k < GR_NROLE; k++)
            if (gen_role_on(k))
                trk_pid[k] = 0;
    }                                                   /* (its lists edited since: the user's now, kept as they are) */
    gen.song = 0;
    gen_write();
    str_cpy(b, GEN_STYLE[gen.v[GK_STYLE] % GS_N], sizeof b);
    str_cpy(b + str_len(b), " #", sizeof b - str_len(b));
    fmt_int(b + str_len(b), gen.seed);
    ui_say("WRITTEN ", b);
}
/* OCT-: what GEN wrote over, back (its copies the newest), else HOME */
static int gen_back(void)
{
    uint32_t i;
    if (!gen.made || !gen_copies_top())
        return 0;
    if (gen.song) {                                     /* ((Vallée) a SONG: its lists, its patterns back first) */
        if (transport_busy())
            transport_stop_now();
        fm1_irq_off();
        memcpy(&lists, gen_lwas, sizeof lists);
        fm1_irq_on();
        gen_pid_back();
    }
    for (i = 0; i < gen.nseq; i++) {
        undo_t *c = &undo_ring[(undo_top + UNDO_N - undo_back) % UNDO_N];
        if (!undo_swap(c))
            break;
        undo_back++;
    }
    for (i = 0; i < NELEM(GEN_G); i++)
        song.g[GEN_G[i]] = gen.g_was[i];
    if (gen.song) {
        uint32_t k, p;
        for (k = 0; k < GR_NROLE; k++)
            for (p = 0; p < GEN_NPART; p++)
                if (gen_sid[k][p] != GEN_NOID)
                    gen_unkeep(gen_sid[k][p]);
        (void)gen_flush();
        gen.song = 0;
        gen_narr = 0;
        memset(gen_rr, 0, sizeof gen_rr);
    }
    gen.made = 0;
    gen.nseq = 0;
    ui_say("PUT BACK ", "AS BEFORE GEN");
    return 1;
}

/* -------------------------------------------------------- the sheet's hooks --- */
static const uint8_t GEN_WIN[GEN_NWIN][4] = {{GK_STYLE, GK_SEED, GK_VIBE, GK_ENERGY}, {GK_PART, GK_KEY, GK_TEMPO, GK_SOUNDS},
                                             {GK_DRUMS, GK_BASS, GK_KEYS, GK_LEAD}, {GK_COLOR, GK_COMPLEX, GK_SWING, GK_HUMAN},
                                             {GK_CHORDS, GK_PART, GK_BAR, GK_TYPE}};
static const char *const GEN_LABEL[GK_N] = {"STYLE", "SEED", "VIBE", "ENERGY", "PART", "KEY", "TEMPO", "SOUNDS", "DRUMS",
                                            "BASS", "KEYS", "LEAD", "COLOR", "COMPLEX", "SWING", "HUMAN", "CHORDS", "BAR",
                                            "TYPE"};
/* ((Vallée) CHORDS' TYPE: 0 END (the part ends before this bar), 1 AUTO (the mode's), then these */
static const uint8_t GC_Q[GC_NT - 1u] = {Q_MAJ, Q_MIN, Q_DOM7, Q_MAJ7, Q_MIN7, Q_SUS2, Q_SUS4, Q_DIM, Q_M7B5, Q_ADD9,
                                         Q_DOM9};
static const char *const GC_QN[GC_NT - 1u] = {"MAJ", "MIN", "7", "MAJ7", "M7", "SUS2", "SUS4", "DIM", "M7B5", "ADD9",
                                              "9"};
#define GC_WIN 4u                                       /* (the fifth window: CHORDS) */
static uint32_t gc_part(void) { return gen.v[GK_PART] % GP_N; }
static uint32_t gc_type(void)                           /* TYPE's place at BAR: END, AUTO, GC_Q's (not one: AUTO's) */
{
    uint32_t part = gc_part(), b = gen.v[GK_BAR] & 3u, k;
    if (part >= GC_NPART || !gch[part].r[b])
        return 0;
    for (k = 0; k + 1u < GC_NT; k++)
        if (gch[part].q[b] == GC_Q[k] + 1u)
            return k + 2u;
    return 1;
}
/* CHORDS turned MINE the first time: GEN's own chords there to start from, as they sound */
static void gc_from_plan(uint32_t part)
{
    gplan_t p;
    uint32_t i;
    gch[part].on = 0;
    gen_plan(&p);
    for (i = 0; i < 4u; i++) {
        uint32_t c = p.n == 8u ? i * 2u : i;
        gch[part].r[i] = (uint8_t)(i < p.n || p.n == 8u ? 8u + p.ch[c][0] % 12u : 0u);
        gch[part].q[i] = (uint8_t)(p.ch[c][1] % Q_N + 1u);
    }
}
static uint32_t gc_song_mine(void)                      /* (the SONG's parts, INTRO .. OUTRO, that play their own) */
{
    uint32_t pt, n = 0;
    for (pt = GP_INTRO; pt <= GP_OUTRO; pt++)
        n += (uint32_t)gc_mine(pt);
    return n;
}
static int gc_editable(void)
{
    if (gc_part() < GC_NPART)
        return 1;
    ui_message("PICK A PART, NOT SONG");
    return 0;
}
/* a key on CHORDS: the chord for BAR (a white key the mode's degree, C I .. B VII; a black key its root), BAR on to the
 * next. At bar 1 a new progression: the bars after it go */
static void gen_chord_key(uint32_t k)
{
    uint32_t part = gc_part(), b = gen.v[GK_BAR] & 3u, pc = (k + 5u) % 12u, i;   /* (key 0: F) */
    static const uint8_t WD[12] = {1, 0, 2, 0, 3, 4, 0, 5, 0, 6, 0, 7};
    if (!gc_editable())
        return;
    if (b == 0u)
        for (i = 1; i < 4u; i++)
            gch[part].r[i] = 0;
    gch[part].r[b] = (uint8_t)(WD[pc] ? WD[pc] : 8u + pc);
    gch[part].q[b] = 0;
    gch[part].on = 1;
    gen.v[GK_BAR] = (uint8_t)((b + 1u) & 3u);
    gen.dirty = 1;
}
static const char *gen_title(void) { return "GEN"; }
static const char *gen_act_name(void) { return gen.made && !gen.dirty ? "NEW SEED" : "WRITE"; }
static int gen_can_back(void) { return gen.made && gen_copies_top(); }
static void gen_pick(int32_t s)                       /* PRESETS: the style */
{
    gen.v[GK_STYLE] = (uint8_t)clamp((int32_t)gen.v[GK_STYLE] + (s > 0 ? 1 : -1), 0, GS_N - 1);
    gen.dirty = 1;
}
static void gen_knob(uint32_t k, int32_t s)
{
    uint32_t id = GEN_WIN[gen.win % GEN_NWIN][k & 3u], part = gc_part(), b = gen.v[GK_BAR] & 3u, t;
    if (id == GK_CHORDS) {                              /* ((Vallée) AUTO / MINE */
        if (!gc_editable())
            return;
        if (s > 0 && !gch[part].r[0])
            gc_from_plan(part);
        gch[part].on = s > 0;
    } else if (id == GK_BAR) {                          /* (up to the bar after the last: one more) */
        t = gc_n(part) < 4u ? gc_n(part) : 3u;
        gen.v[GK_BAR] = (uint8_t)clamp((int32_t)b + (s > 0 ? 1 : -1), 0, part < GC_NPART ? (int32_t)t : 3);
        return;
    } else if (id == GK_TYPE) {
        if (!gc_editable())
            return;
        t = (uint32_t)clamp((int32_t)gc_type() + (s > 0 ? 1 : -1), 0, GC_NT);
        if (t && !gch[part].r[b])                       /* (END turned on: the chord before it, or the root's) */
            gch[part].r[b] = b && gch[part].r[b - 1u] ? gch[part].r[b - 1u] : 1u;
        if (!t)
            gch[part].r[b] = 0;
        gch[part].q[b] = (uint8_t)(t >= 2u ? GC_Q[t - 2u] + 1u : 0u);
        gch[part].on = gch[part].r[0] != 0;
    } else if (id == GK_SEED)
        gen.seed = (uint16_t)clamp((int32_t)gen.seed + accel(EN_K1 + k, s, 999), 1, 999);
    else if (GEN_MAX[id] >= 100u)
        gen.v[id] = (uint8_t)clamp((int32_t)gen.v[id] + accel(EN_K1 + k, s, GEN_MAX[id]), 0, GEN_MAX[id]);
    else
        gen.v[id] = (uint8_t)clamp((int32_t)gen.v[id] + (s > 0 ? 1 : -1), 0, GEN_MAX[id]);
    gen.dirty = 1;
}
static void gen_value(uint32_t id, char *val, const char **unit)
{
    *unit = "";
    switch (id) {
    case GK_STYLE: str_cpy(val, GEN_STYLE[gen.v[id] % GS_N], 12); break;
    case GK_SEED: fmt_int(val, gen.seed); break;
    case GK_VIBE: str_cpy(val, GEN_VIBE[gen.v[id] % GV_N], 12); break;
    case GK_PART: str_cpy(val, GEN_PART[gen.v[id] % GP_N], 12); break;
    case GK_KEY: str_cpy(val, gen.v[id] ? GEN_NOTE[(gen.v[id] - 1u) % 12u] : "AUTO", 12); break;
    case GK_TEMPO:
        if (gen.v[id])
            fmt_int(val, 60 + gen.v[id] - 1);
        else
            str_cpy(val, "AUTO", 12);
        *unit = gen.v[id] ? "BPM" : "";
        break;
    case GK_SOUNDS: str_cpy(val, gen.v[id] ? "KEEP" : "NEW", 12); break;
    case GK_SWING:
        if (gen.v[id])
            fmt_int(val, gen.v[id] - 1);
        else
            str_cpy(val, "AUTO", 12);
        *unit = gen.v[id] ? "%" : "";
        break;
    case GK_ENERGY: case GK_COMPLEX: case GK_HUMAN: fmt_int(val, gen.v[id]); *unit = "%"; break;
    case GK_CHORDS:                                     /* (SONG: how many of its parts play their own) */
        if (gc_part() == GP_SONG && gc_song_mine()) {
            fmt_int(val, (int32_t)gc_song_mine());
            str_cpy(val + str_len(val), " MINE", 8);
        } else
            str_cpy(val, gc_mine(gc_part()) ? "MINE" : "AUTO", 12);
        break;
    case GK_BAR: case GK_TYPE:                          /* (AUTO: neither means anything) */
        if (!gc_mine(gc_part())) {
            str_cpy(val, "-", 12);
            break;
        }
        if (id == GK_BAR) {
            fmt_int(val, (gen.v[id] & 3u) + 1u);
            break;
        }
        {
            uint32_t t = gc_type(), q = gch[gc_part()].q[gen.v[GK_BAR] & 3u];
            str_cpy(val, !t ? "END" : t >= 2u ? GC_QN[t - 2u] : q ? (CQ[(q - 1u) % Q_N].sfx[0] ? CQ[(q - 1u) % Q_N].sfx
                    : "MAJ") : "AUTO", 12);
        }
        break;
    default: str_cpy(val, GEN_AMT[gen.v[id] & 3u], 12); break;
    }
}
static void gen_cards(void)
{
    static const uint8_t ICON[GK_N] = {ICON_X_EYE, ICON_X_STAR, ICON_TONE, ICON_LEVEL, ICON_X_SONG, ICON_SCALE, ICON_TEMPO,
                                       ICON_LOAD, ICON_DRUM, ICON_SUB, ICON_DRAWBAR, ICON_VOICE, ICON_ARP, ICON_ALGORITHM,
                                       ICON_SWING, ICON_GLIDE, ICON_TRIO, ICON_STEPS, ICON_SHAPE};
    uint32_t k;
    for (k = 0; k < 4u; k++) {
        uint32_t id = GEN_WIN[gen.win % GEN_NWIN][k], mx = GEN_MAX[id];
        char val[12];
        const char *unit;
        int32_t v = id == GK_SEED ? gen.seed : id == GK_CHORDS ? (gc_mine(gc_part()) || (gc_part() == GP_SONG &&
                    gc_song_mine())) : id == GK_TYPE ? (int32_t)gc_type()
                    : gen.v[id];
        gen_value(id, val, &unit);
        draw_column(k, GEN_LABEL[id], val, unit, (id >= GK_DRUMS && id <= GK_COLOR && !gen.v[id]) ||
                    ((id == GK_BAR || id == GK_TYPE) && !gc_mine(gc_part())) ? VR_DIM : VR_LIVE,
                    id == GK_SEED ? v * 1000 / 999 : mx ? v * 1000 / (int32_t)mx : -1, ICON[id]);
    }
}
static uint32_t gen_sig(void)
{
    uint32_t h = 2166136261u ^ gen.seed * 131u ^ gen.win * 977u ^ gen.made * 7u, k;
    const uint8_t *c = (const uint8_t *)gch;
    for (k = 0; k < GK_N; k++)
        h = (h ^ gen.v[k]) * 16777619u;
    for (k = 0; k < sizeof gch; k++)                    /* ((Vallée) CHORDS' too */
        h = (h ^ c[k]) * 16777619u;
    return h;
}

/* the panel: the seed's sigil (points on a circle joined as a star, its own for every seed), and the piece in words:
 * the style and vibe, the key and tempo, the part's chords */
static void gen_sigil(int32_t cx, int32_t cy, int32_t rad, uint16_t c, uint16_t dim)
{
    grng_t r = g_stream('Z', 0);
    uint32_t n = 5u + g_n(&r, 5), step = 2u + g_n(&r, n / 2u > 2u ? n / 2u - 1u : 1u), i, ph = g_next(&r);
    int32_t px[10], py[10];
    xe_ring(cx, cy, rad, dim);
    xe_ring(cx, cy, rad - 3, dim);
    for (i = 0; i < n; i++) {
        uint32_t a = ph + i * (0xFFFFFFFFu / n);
        px[i] = cx + (rad - 5) * QK_COS(a) / 32767;
        py[i] = cy + (rad - 5) * QK_SIN(a) / 32767;
    }
    for (i = 0; i < n; i++)
        xe_line(px[i], py[i], px[(i + step) % n], py[(i + step) % n], c);
    for (i = 0; i < n; i++)
        cv_rect(px[i] - 1, py[i] - 1, 3, 3, T_ACCENT);
    xe_ring(cx, cy, rad / 4, c);
    cv_rect(cx - 1, cy - 1, 3, 3, T_TEXT);
}
/* ((Vallée) TEEN: no star; the seed as flat shapes on a 3 x 3 grid, as its tape machine draws (a disc, a square, a
   ring, a pill, a small dot, or none), each in one of its tracks' four colours: its own for every seed */
static void gen_teen_mark(int32_t cx, int32_t cy, int32_t rad)
{
    grng_t r = g_stream('Z', 0);
    const int32_t cell = (2 * rad) / 3, q = cell / 2 - 3;
    uint32_t i, empty = g_n(&r, 9);                     /* (one place left empty: room in it) */
    for (i = 0; i < 9u; i++) {
        int32_t x = cx - cell + (int32_t)(i % 3u) * cell, y = cy - cell + (int32_t)(i / 3u) * cell;
        uint32_t k = g_n(&r, 5);
        uint16_t col = T_TRK(g_n(&r, 4));
        if (i == empty)
            continue;
        switch (k) {
        case 0: xe_disc(x, y, q, col); break;
        case 1: cv_rect(x - q, y - q, 2 * q + 1, 2 * q + 1, col); break;
        case 2: xe_ring(x, y, q, col); xe_ring(x, y, q - 1, col); xe_ring(x, y, q - 2, col); break;
        case 3: cv_rect(x - q / 2, y - q / 2, q + 1, q + 1, col); xe_disc(x, y - q / 2, q / 2, col);
                xe_disc(x, y + q / 2, q / 2, col); break;
        default: xe_disc(x, y, q / 3 + 1, col); break;
        }
    }
}
/* ((Vallée) the song's code: the knobs set away from where GEN starts, short (the style, vibe, part and seed are the
 * lines above it). The same code on any Vallée of this version writes the same song: ENERGY E, KEY IN, TEMPO BPM,
 * SOUNDS KEEP, the parts' amounts DR BA KY LD CO (0 OFF .. 3 BUSY), COMPLEX X, SWING S, HUMAN H; "" all as GEN starts */
static void gen_code(char *b, uint32_t n)
{
    static const uint8_t K[] = {GK_ENERGY, GK_KEY, GK_TEMPO, GK_SOUNDS, GK_DRUMS, GK_BASS, GK_KEYS, GK_LEAD, GK_COLOR,
                                GK_COMPLEX, GK_SWING, GK_HUMAN};
    static const char *const T[] = {"E", "IN ", "", "KEEP", "DR", "BA", "KY", "LD", "CO", "X", "S", "H"};
    uint32_t i, k, v;
    b[0] = 0;
    for (i = 0; i < NELEM(K); i++) {
        k = K[i];
        v = gen.v[k];
        if (v == GEN_DEF[k] || str_len(b) + 9u > n)
            continue;
        if (b[0])
            str_cpy(b + str_len(b), " ", n - str_len(b));
        str_cpy(b + str_len(b), T[i], n - str_len(b));
        if (k == GK_KEY)
            str_cpy(b + str_len(b), GEN_NOTE[(v - 1u) % 12u], n - str_len(b));
        else if (k == GK_TEMPO) {
            fmt_int(b + str_len(b), 60 + (int32_t)v - 1);
            str_cpy(b + str_len(b), "BPM", n - str_len(b));
        } else if (k == GK_SWING)
            fmt_int(b + str_len(b), (int32_t)v - 1);
        else if (k != GK_SOUNDS)
            fmt_int(b + str_len(b), (int32_t)v);
    }
}
static void graph_gen(const track_t *t, uint16_t c)
{
    gplan_t p;
    char b[40], nm[12];
    uint32_t i, w = gen.win % GEN_NWIN;
    int32_t h = (int32_t)cv_h, rad = (h - 18) / 2 > 44 ? 44 : (h - 18) / 2, x0 = 108, y,
            top = (h - (2 * rad + 9)) / 2 > 6 ? (h - (2 * rad + 9)) / 2 : 6, cy = top + rad;   /* ((Vallée) in the middle */
    (void)t;                                            /* of the box, up and down) */
    (void)c;
    gen_plan(&p);
    if (UX_TEEN)
        gen_teen_mark(10 + rad + 4, cy, rad);           /* ((Vallée) TEEN: shapes, no star */
    else
        gen_sigil(10 + rad + 4, cy, rad, T_THEME, T_LINE);
    for (i = 0; i < GEN_NWIN; i++)                      /* the windows: SELECT's place */
        cv_rect(10 + rad + 4 - ((int32_t)GEN_NWIN * 9 - 3) / 2 + (int32_t)i * 9, cy + rad + 6, 6, 3,
                i == w ? T_ACCENT : T_RAISE);                /* ((Vallée) five now: centred under the sigil) */
    y = top;
    str_cpy(b, GEN_STYLE[gen.v[GK_STYLE] % GS_N], sizeof b);
    cv_free_text(x0, y - AF_M_CAP_Y, &AF_M, b, T_TEXT, cv_bg, 232 - x0);
    y += 20;
    str_cpy(b, GEN_VIBE[gen.v[GK_VIBE] % GV_N], sizeof b);
    str_cpy(b + str_len(b), "  ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), GEN_PART[gen.v[GK_PART] % GP_N], sizeof b - str_len(b));
    str_cpy(b + str_len(b), "  #", sizeof b - str_len(b));
    fmt_int(b + str_len(b), gen.seed);
    cv_free_text(x0, y - AF_S_CAP_Y, &AF_S, b, T_MID, cv_bg, 232 - x0);
    y += 16;
    gen_code(b, 22);                                    /* ((Vallée) the knobs set otherwise: the song's code */
    if (b[0] && w != GC_WIN) {                          /* ((CHORDS: not there, the room the chords') */
        cv_free_text(x0, y - AF_S_CAP_Y, &AF_S, b, T_ACCENT, cv_bg, 232 - x0);
        y += 14;
    }
    str_cpy(b, GEN_NOTE[p.key % 12u], sizeof b);
    str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), N_SCALE[p.scale % NELEM(N_SCALE)], sizeof b - str_len(b));
    str_cpy(b + str_len(b), "  ", sizeof b - str_len(b));
    if (w == GC_WIN) {                                  /* ((Vallée) CHORDS: what the C key plays, the I there */
        gplan_t t = p;
        gc_chord_code(&t, 0, 1, 0);
        g_chord_name(&t, 0, nm);
        str_cpy(b + str_len(b), "C KEY ", sizeof b - str_len(b));
        str_cpy(b + str_len(b), nm, sizeof b - str_len(b));
    } else {
        fmt_int(b + str_len(b), p.bpm);
        str_cpy(b + str_len(b), " BPM", sizeof b - str_len(b));
    }
    cv_free_text(x0, y - AF_S_CAP_Y, &AF_S, b, T_THEME, cv_bg, 232 - x0);
    y += 18;
    if (w == GC_WIN && gc_part() == GP_SONG) {          /* ((Vallée) CHORDS on SONG: each part's own, a line each */
        uint32_t pt, was = gen.v[GK_PART], j;
        for (pt = GP_INTRO; pt <= GP_OUTRO && y + 12 <= h; pt++) {
            gplan_t q;
            if (!gc_mine(pt))
                continue;
            gen.v[GK_PART] = (uint8_t)pt;
            gen_plan(&q);
            gen.v[GK_PART] = (uint8_t)was;
            str_cpy(b, GEN_PART[pt], sizeof b);
            for (j = 0; j < q.n; j++) {
                g_chord_name(&q, j, nm);
                str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
                str_cpy(b + str_len(b), nm, sizeof b - str_len(b));
            }
            cv_free_text(x0, y - AF_S_CAP_Y, &AF_S, b, T_TEXT, cv_bg, 232 - x0);
            y += 14;
        }
        if (!gc_song_mine())
            cv_free_text(x0, y - AF_S_CAP_Y, &AF_S, "ALL GEN'S OWN", T_MID, cv_bg, 232 - x0);
        return;
    }
    if (w == GC_WIN && gc_mine(gc_part())) {            /* ((Vallée) CHORDS, MINE: a bar each, BAR lit (two a line; */
        uint32_t bar = gen.v[GK_BAR] & 3u;              /* the bar after the last: "+") */
        for (i = 0; i < 4u && i <= p.n && y + 12 <= h; i++) {
            b[0] = (char)('1' + i);
            b[1] = ' ';
            b[2] = 0;
            if (i < p.n)
                g_chord_name(&p, i, b + 2);
            else
                str_cpy(b + 2, "+", sizeof b - 2u);
            cv_free_text(x0 + (int32_t)(i & 1u) * 62, y - AF_S_CAP_Y, &AF_S, b, i == bar ? T_ACCENT : T_TEXT, cv_bg, 60);
            y += (int32_t)(i & 1u) * 14;
        }
        return;
    }
    for (i = 0; i < p.n && y + 12 <= h; i += 2u) {     /* the chords, two a line */
        g_chord_name(&p, i, nm);
        str_cpy(b, nm, sizeof b);
        if (i + 1u < p.n) {
            str_cpy(b + str_len(b), "   ", sizeof b - str_len(b));
            g_chord_name(&p, i + 1u, nm);
            str_cpy(b + str_len(b), nm, sizeof b - str_len(b));
        }
        cv_free_text(x0, y - AF_S_CAP_Y, &AF_S, b, T_TEXT, cv_bg, 232 - x0);
        y += 14;
        if (p.n == 4u && i == 2u)
            break;
    }
}
