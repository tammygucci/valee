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
       GK_LEAD, GK_COLOR, GK_COMPLEX, GK_SWING, GK_HUMAN, GK_N };
#define GEN_NWIN 4u
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
} gen = {{0, 0, GV_HAZY, 60, GP_LOOP, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0}, 1, 0, 0, 0, 0, {0}, {0}, 0};
static const uint8_t GEN_MAX[GK_N] = {GS_N - 1, 0, GV_N - 1, 100, GP_N - 1, 12, 121, 1, 3, 3, 3, 3, 3, 100, 101, 100};
static const uint8_t GEN_DEF[GK_N] = {0, 0, GV_HAZY, 60, GP_LOOP, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0};
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
/* a stream of its own for each part of the music: the seed, the style and what it is for (id); part: the PART's own */
static grng_t g_stream(uint32_t id, uint32_t part)
{
    grng_t r;
    uint32_t i;
    r.s = (uint32_t)gen.seed * 2654435761u ^ (uint32_t)gen.v[GK_STYLE] * 0x9E3779B9u ^ id * 0x85EBCA6Bu ^
          part * 0xC2B2AE35u;
    for (i = 0; i < 4u; i++)
        g_next(&r);
    return r;
}

/* ------------------------------------------------------------- the harmony --- */
enum { Q_MAJ, Q_MIN, Q_MAJ7, Q_MIN7, Q_DOM7, Q_MAJ9, Q_MIN9, Q_DOM9, Q_SUS2, Q_SUS4, Q_ADD9, Q_MADD9, Q_M7B5, Q_POW,
       Q_MIN11, Q_DOM13, Q_9SUS4, Q_MAJ7S11, Q_MU, Q_MIN6, Q_7S9, Q_AUG, Q_N };
static const struct { uint8_t n, iv[5]; const char *sfx; } CQ[Q_N] = {
    {3, {0, 4, 7}, ""},           {3, {0, 3, 7}, "m"},           {4, {0, 4, 7, 11}, "maj7"},
    {4, {0, 3, 7, 10}, "m7"},     {4, {0, 4, 7, 10}, "7"},       {5, {0, 4, 7, 11, 14}, "maj9"},
    {5, {0, 3, 7, 10, 14}, "m9"}, {5, {0, 4, 7, 10, 14}, "9"},   {3, {0, 2, 7}, "sus2"},
    {3, {0, 5, 7}, "sus4"},       {4, {0, 4, 7, 14}, "add9"},    {4, {0, 3, 7, 14}, "madd9"},
    {4, {0, 3, 6, 10}, "m7b5"},   {3, {0, 7, 12}, "5"},          {5, {0, 3, 7, 10, 17}, "m11"},
    {5, {0, 4, 10, 14, 21}, "13"}, {5, {0, 5, 7, 10, 14}, "9sus"}, {5, {0, 4, 7, 11, 18}, "maj7#11"},
    {4, {0, 2, 4, 7}, "add2"},    {4, {0, 3, 7, 9}, "m6"},       {5, {0, 4, 7, 10, 15}, "7#9"},
    {3, {0, 4, 8}, "+"},
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
enum { E_ANALOG = 0, E_PHASE = 2, E_LOFI = 3, E_SAMPLE = 4, E_VOICE = 5, E_TRIO = 6, E_WHEEL = 7, E_GRAIN = 8,
       E_PHYS = 9, E_DRUM = 10, E_NOISE = 11, E_FM6 = 12 };

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
static const gprog_t ENO_D[] = {
    P4(C_(0, Q_MIN9), C_(0, Q_MIN9), C_(8, Q_MAJ7), C_(8, Q_MAJ7)),
    P4(C_(0, Q_MADD9), C_(10, Q_SUS2), C_(8, Q_MAJ7), C_(3, Q_ADD9))};
static const gprog_t ENO_L[] = {
    P4(C_(0, Q_ADD9), C_(5, Q_ADD9), C_(0, Q_ADD9), C_(7, Q_SUS4)),
    P4(C_(0, Q_MAJ7), C_(0, Q_MAJ7), C_(9, Q_MIN7), C_(9, Q_MIN7)),
    P4(C_(0, Q_SUS2), C_(2, Q_MIN7), C_(5, Q_MAJ7), C_(0, Q_SUS2))};
static const gdrum_t ENO_DR[2] = {
    {{"o...............", 0, 0, 0, 0, 0, "........o.......", 0}},
    {{"x.......o.......", 0, 0, 0, 0, "............o...", "....o.......o...", 0}}};
static const gdrum_t ENO_PC = {{0, 0, 0, 0, 0, 0, 0, 0}};
static const char *const ENO_B[3] = {"R---------------", "R---------------", "R-------5-------"};
static const char *const ENO_C[3] = {"X---------------", "X---------------", "X---------------"};
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
     {{{E_FM6, "PICK BASS"}, {E_ANALOG, "SQR BASS"}, {E_TRIO, "FAT BASS"}},
      {{E_ANALOG, "SUNSET PAD"}, {E_ANALOG, "SOFT PAD"}, {E_PHASE, "PD STRINGS"}},
      {{E_TRIO, "ARP LEAD"}, {E_ANALOG, "NEON LEAD"}, {E_LOFI, "WAVE LEAD"}},
      {{E_DRUM, "DEEP DRUMS"}, {E_DRUM, "STUDIO KIT"}, {E_DRUM, "BOOM KIT"}},
      {{E_TRIO, "NEON ARP"}, {E_LOFI, "ARP 8BIT"}, {E_VOICE, "DREAM CHOIR"}},
      {{E_DRUM, "PERC KIT"}, {E_DRUM, "LOFI KIT"}, {0, 0}}},
     2, 100, 10, 70, 70, 2, {6, 36, 30, 22, 30, 16}, {0, 10, 40, 0, 50, 0}, 6, 0, 0},
    /* DEMARCO */
    {72, 96, 18, 2, 1, {7, 2, 9, 4}, 3, 1, DEM_D, DEM_L, 3, 4, DEM_DR, &DEM_PC, DEM_B, DEM_C, DEM_L_, XC_JANGLE, 62, 81,
     {{{E_FM6, "PICK BASS"}, {E_LOFI, "TRI BASS"}, {E_FM6, "FM BASS"}},
      {{E_SAMPLE, "LOFI PIANO"}, {E_FM6, "TINE EP"}, {E_PHYS, "PLUCK"}},
      {{E_LOFI, "WAVE LEAD"}, {E_ANALOG, "SINE KEY"}, {E_PHYS, "HARP"}},
      {{E_DRUM, "TIGHT KIT"}, {E_DRUM, "LOFI KIT"}, {E_DRUM, "STUDIO KIT"}},
      {{E_FM6, "NYLON PICK"}, {E_SAMPLE, "KOTO"}, {E_LOFI, "CHIP BELL"}},
      {{E_DRUM, "PERC KIT"}, {0, 0}, {0, 0}}},
     0, 70, 11, 50, 50, 0, {10, 24, 26, 14, 30, 10}, {0, 0, 20, 0, 12, 0}, 8, 1, 0},
    /* DAFT */
    {118, 126, 8, 1, 0, {9, 5, 2, 0}, 2, 1, DAF_D, DAF_L, 3, 3, DAF_DR, &DAF_PC, DAF_B, DAF_C, DAF_L_, XC_ROBOT, 60, 76,
     {{{E_FM6, "FM BASS"}, {E_TRIO, "DIRTY BASS"}, {E_ANALOG, "DRIVE BASS"}},
      {{E_ANALOG, "PWM STR"}, {E_FM6, "TINE EP"}, {E_WHEEL, "GOSPEL"}},
      {{E_VOICE, "ROBOT TALK"}, {E_TRIO, "SYNC LEAD"}, {E_ANALOG, "SAW LEAD"}},
      {{E_DRUM, "ELECTRO KIT"}, {E_DRUM, "RHYTHM BOX"}, {E_DRUM, "CRISP KIT"}},
      {{E_VOICE, "ROBOT TALK"}, {E_VOICE, "VOX LEAD"}, {E_TRIO, "CHIP CHOIR"}},
      {{E_DRUM, "CRISP KIT"}, {E_DRUM, "PERC KIT"}, {0, 0}}},
     0, 60, 11, 40, 60, 2, {0, 16, 16, 10, 18, 8}, {0, 10, 30, 0, 24, 0}, 0, 0, 1},
    /* HEAD */
    {132, 140, 0, 1, 0, {9, 2, 7, 4}, 2, 9, HEA_D, HEA_L, 4, 3, HEA_DR, &HEA_PC, HEA_B, HEA_C, HEA_L_, XC_TEXTURE, 64,
     79,
     {{{E_ANALOG, "SUB BASS"}, {E_FM6, "DARK MATTER"}, {E_TRIO, "FAT BASS"}},
      {{E_ANALOG, "DARK STRINGS"}, {E_ANALOG, "SOFT PAD"}, {E_ANALOG, "PWM STR"}},
      {{E_VOICE, "VOX LEAD"}, {E_VOICE, "WHISPER"}, {E_ANALOG, "SINE KEY"}},
      {{E_DRUM, "ELECTRO KIT"}, {E_DRUM, "RHYTHM BOX"}, {E_DRUM, "CRISP KIT"}},
      {{E_NOISE, "GEIGER"}, {E_NOISE, "ARCADE"}, {E_NOISE, "PLASMA"}},
      {{E_DRUM, "SNAP KIT"}, {E_DRUM, "RHYTHM BOX"}, {0, 0}}},
     0, 70, 11, 50, 50, 0, {6, 34, 30, 8, 30, 16}, {0, 10, 30, 0, 24, 18}, 0, 0, 0},
    /* ENO */
    {60, 78, 0, 1, 0, {3, 2, 4, 9}, 3, 9, ENO_D, ENO_L, 2, 3, ENO_DR, &ENO_PC, ENO_B, ENO_C, ENO_L_, XC_BELLS, 67, 91,
     {{{E_ANALOG, "SUB BASS"}, {E_WHEEL, "PEDAL BASS"}, {E_FM6, "PAD"}},
      {{E_GRAIN, "CLOUD PAD"}, {E_GRAIN, "SHIMMER"}, {E_GRAIN, "FROZEN"}},
      {{E_FM6, "GLASS BELL"}, {E_PHYS, "KALIMBA"}, {E_SAMPLE, "MUSIC BOX"}},
      {{E_DRUM, "DEEP DRUMS"}, {E_DRUM, "LOFI KIT"}, {0, 0}},
      {{E_FM6, "GLASS KEYS"}, {E_SAMPLE, "FLUTE PAD"}, {E_LOFI, "CHIP BELL"}},
      {{E_DRUM, "PERC KIT"}, {0, 0}, {0, 0}}},
     2, 127, 12, 82, 60, 0, {30, 80, 70, 40, 70, 40}, {0, 20, 50, 0, 50, 0}, 0, 0, 0},
    /* STEELY */
    {124, 132, 10, 3, 0, {7, 2, 9, 4}, 4, 1, STE_D, STE_L, 3, 4, STE_DR, &STE_PC, STE_B, STE_C, STE_L_, XC_FILLS, 64,
     83,
     {{{E_FM6, "PICK BASS"}, {E_FM6, "FM BASS"}, {E_TRIO, "FAT BASS"}},
      {{E_FM6, "TINE EP"}, {E_FM6, "GLASS KEYS"}, {E_PHASE, "SOFT KEYS"}},
      {{E_VOICE, "CHOIR AAH"}, {E_FM6, "FM BRASS"}, {E_ANALOG, "SYNTH BRASS"}},
      {{E_DRUM, "STUDIO KIT"}, {E_DRUM, "TIGHT KIT"}, {E_DRUM, "CRISP KIT"}},
      {{E_FM6, "NYLON PICK"}, {E_ANALOG, "SYNTH PLUCK"}, {E_PHYS, "PLUCK"}},
      {{E_DRUM, "PERC KIT"}, {0, 0}, {0, 0}}},
     0, 50, 11, 34, 30, 0, {4, 14, 22, 8, 14, 6}, {0, 0, 12, 0, 10, 0}, 8, 0, 0},
    /* YACHT */
    {116, 124, 4, 2, 0, {1, 8, 3, 10}, 3, 1, YAC_D, YAC_L, 2, 3, YAC_DR, &YAC_PC, YAC_B, YAC_C, YAC_L_, XC_PAD, 64, 84,
     {{{E_FM6, "PICK BASS"}, {E_FM6, "FM BASS"}, {E_TRIO, "FAT BASS"}},
      {{E_SAMPLE, "PIANO"}, {E_FM6, "TINE EP"}, {E_SAMPLE, "LOFI PIANO"}},
      {{E_VOICE, "VOX LEAD"}, {E_SAMPLE, "SAX"}, {E_ANALOG, "HERO BRASS"}},
      {{E_DRUM, "STUDIO KIT"}, {E_DRUM, "TIGHT KIT"}, {E_DRUM, "DRUM KIT"}},
      {{E_ANALOG, "STRINGS"}, {E_ANALOG, "SYNTH BRASS"}, {E_PHASE, "PD STRINGS"}},
      {{E_DRUM, "PERC KIT"}, {0, 0}, {0, 0}}},
     2, 70, 11, 36, 34, 0, {6, 18, 24, 10, 32, 8}, {0, 0, 16, 0, 0, 0}, 8, 0, 0},
    /* LCD */
    {86, 100, 0, 1, 0, {0, 9, 4, 2}, 2, 1, LCD_D, LCD_L, 3, 4, LCD_DR, &LCD_PC, LCD_B, LCD_C, LCD_L_, XC_ARP16, 60, 79,
     {{{E_ANALOG, "SQR BASS"}, {E_ANALOG, "NIGHT BASS"}, {E_TRIO, "FAT BASS"}},
      {{E_ANALOG, "SUNSET PAD"}, {E_ANALOG, "SOFT PAD"}, {E_ANALOG, "STRINGS"}},
      {{E_VOICE, "VOX LEAD"}, {E_ANALOG, "NEON LEAD"}, {E_VOICE, "DREAM CHOIR"}},
      {{E_DRUM, "RHYTHM BOX"}, {E_DRUM, "ELECTRO KIT"}, {E_DRUM, "LOFI KIT"}},
      {{E_TRIO, "NEON ARP"}, {E_ANALOG, "SYNTH PLUCK"}, {E_LOFI, "ARP 8BIT"}},
      {{E_DRUM, "PERC KIT"}, {E_DRUM, "LOFI KIT"}, {0, 0}}},
     2, 78, 11, 44, 30, 0, {4, 26, 30, 8, 26, 8}, {0, 6, 26, 0, 20, 0}, 0, 0, 0},
    /* STROKE */
    {124, 146, 0, 0, 0, {9, 4, 2, 7}, 2, 1, STR_D, STR_L, 3, 4, STR_DR, &STR_PC, STR_B, STR_C, STR_L_, XC_JANGLE, 60, 79,
     {{{E_FM6, "PICK BASS"}, {E_ANALOG, "DRIVE BASS"}, {E_FM6, "FM BASS"}},
      {{E_PHYS, "PLUCK"}, {E_FM6, "NYLON PICK"}, {E_ANALOG, "SYNTH PLUCK"}},
      {{E_PHYS, "PLUCK"}, {E_FM6, "NYLON PICK"}, {E_ANALOG, "SYNTH PLUCK"}},
      {{E_DRUM, "TIGHT KIT"}, {E_DRUM, "DRUM KIT"}, {E_DRUM, "STUDIO KIT"}},
      {{E_WHEEL, "ROCK DRIVE"}, {E_FM6, "NYLON PICK"}, {E_ANALOG, "SYNTH PLUCK"}},
      {{E_DRUM, "CRISP KIT"}, {0, 0}, {0, 0}}},
     0, 44, 11, 30, 30, 0, {4, 10, 10, 8, 10, 6}, {0, 0, 6, 0, 0, 0}, 0, 0, 0},
    /* SLAB */
    {120, 138, 0, 1, 0, {4, 9, 2, 7}, 3, 1, SLB_D, SLB_L, 3, 3, SLB_DR, &SLB_PC, SLB_B, SLB_C, SLB_L_, XC_ARP16, 62, 79,
     {{{E_ANALOG, "SQR BASS"}, {E_ANALOG, "DRIVE BASS"}, {E_TRIO, "FAT BASS"}},
      {{E_WHEEL, "FULL ORGAN"}, {E_WHEEL, "ROCK DRIVE"}, {E_PHASE, "PD ORGAN"}},
      {{E_VOICE, "CHOIR AAH"}, {E_VOICE, "DREAM CHOIR"}, {E_ANALOG, "SINE KEY"}},
      {{E_DRUM, "TIGHT KIT"}, {E_DRUM, "DRUM KIT"}, {E_DRUM, "LOFI KIT"}},
      {{E_ANALOG, "SYNTH PLUCK"}, {E_LOFI, "PULSE LD"}, {E_TRIO, "ARP LEAD"}},
      {{E_DRUM, "PERC KIT"}, {E_DRUM, "LOFI KIT"}, {0, 0}}},
     0, 60, 10, 40, 40, 2, {4, 16, 24, 8, 20, 8}, {0, 0, 24, 0, 26, 0}, 8, 0, 0},
    /* WINWOOD */
    {116, 124, 0, 1, 0, {0, 7, 2, 9}, 3, 1, WIN_D, WIN_L, 2, 4, WIN_DR, &WIN_PC, WIN_B, WIN_C, WIN_L_, XC_ARP16, 64,
     83,
     {{{E_ANALOG, "SQR BASS"}, {E_ANALOG, "SUB SAW"}, {E_FM6, "FM BASS"}},
      {{E_ANALOG, "SUNSET PAD"}, {E_PHASE, "PD BRASS"}, {E_FM6, "GLASS KEYS"}},
      {{E_ANALOG, "NEON LEAD"}, {E_VOICE, "VOX LEAD"}, {E_FM6, "FM BRASS"}},
      {{E_DRUM, "NEON KIT"}, {E_DRUM, "ELECTRO KIT"}, {E_DRUM, "BOOM KIT"}},
      {{E_TRIO, "NEON ARP"}, {E_FM6, "GLASS BELL"}, {E_ANALOG, "SYNTH PLUCK"}},
      {{E_DRUM, "PERC KIT"}, {0, 0}, {0, 0}}},
     0, 64, 11, 40, 30, 0, {4, 20, 24, 14, 20, 8}, {0, 0, 18, 0, 20, 0}, 8, 0, 0},
    /* GEE */
    {88, 100, 6, 1, 1, {4, 9, 2, 7}, 3, 1, GEE_D, GEE_L, 2, 3, GEE_DR, &GEE_PC, GEE_B, GEE_C, GEE_L_, XC_JANGLE, 67, 88,
     {{{E_FM6, "PICK BASS"}, {E_ANALOG, "SUB BASS"}, {E_TRIO, "FAT BASS"}},
      {{E_PHYS, "PLUCK"}, {E_FM6, "NYLON PICK"}, {E_GRAIN, "PIANO HAZE"}},
      {{E_VOICE, "VOX LEAD"}, {E_VOICE, "WHISPER"}, {E_VOICE, "DREAM CHOIR"}},
      {{E_DRUM, "LOFI KIT"}, {E_DRUM, "BOOM KIT"}, {E_DRUM, "DEEP DRUMS"}},
      {{E_FM6, "NYLON PICK"}, {E_LOFI, "WAVE LEAD"}, {E_PHYS, "HARP"}},
      {{E_DRUM, "PERC KIT"}, {E_DRUM, "LOFI KIT"}, {0, 0}}},
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
}
/* the chord at step i of 64 */
static uint32_t g_chord_at(const gplan_t *p, uint32_t i) { return (i % 64u) * p->n / 64u; }
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
        for (l = 0; l < NLANE; l++) {
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
        w[i].hit = (uint8_t)(hit | ghost);
        w[i].acc = (uint8_t)acc;
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
    pick = g_n(r, n);
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

/* write the piece into the tracks */
static void gen_write(void)
{
    {                                                   /* (the knobs inside their ends, whatever came before) */
        uint32_t k;
        for (k = 0; k < GK_N; k++)
            if (gen.v[k] > GEN_MAX[k])
                gen.v[k] = GEN_MAX[k];
    }
    static step_t w[NSTEP];                             /* (.bss: the pool is full) */
    const gstyle_t *s = g_sty();
    gplan_t p;
    uint32_t role, part = gen.v[GK_PART] % GP_N, sty = gen.v[GK_STYLE] % GS_N, keep = !gen_copies_top(), i;
    int32_t energy = gen.v[GK_ENERGY] * (part == GP_CHORUS ? 120 : part == GP_VERSE || part == GP_OUTRO ? 85 :
                                         part == GP_INTRO ? 60 : 100) / 100;
    gen_plan(&p);
    if (keep) {                                         /* a fresh GEN: the copies, the globals as they were */
        gen.nseq = 0;
        for (i = 0; i < NELEM(GEN_G); i++)
            gen.g_was[i] = song.g[GEN_G[i]];
    }
    undo_depth++;                                       /* (the sound loads below take no copies of their own) */
    for (role = 0; role < GR_NROLE; role++) {
        track_t *t = &trk[role];
        grng_t r = g_stream('D' + role * 7u, role == GR_LEAD ? 0u : part), rs = g_stream('S' + role, 0);
        uint32_t amt = gen_amt(role), len = 64, j, have;
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
        w_clear(w, NSTEP);
        switch (role) {
        case GR_BASS: g_bass(w, &p, &r, amt, part); break;
        case GR_KEYS: g_keys(w, &p, &r, amt, part); break;
        case GR_LEAD:
            if (sty == GS_ENO)
                len = g_eno_loop(w, &p, &r, 53, 2u + amt, s->lead_lo);
            else
                g_lead(w, &p, amt, part);
            break;
        case GR_DRUMS:
            if (sty != GS_ENO || energy > 50)
                g_drums(w, &s->drum[part == GP_CHORUS || (part == GP_LOOP && energy > 70) || amt == 3u],
                        &r, energy, part != GP_INTRO && part != GP_OUTRO && amt > 1u, 64, part == GP_INTRO);
            break;
        case GR_COLOR:
            if (sty == GS_ENO)
                len = g_eno_loop(w, &p, &r, 37, 2u + amt, 72);
            else
                g_color(w, &p, &r, amt, part);
            break;
        default:                                        /* PERC: a second kit; HEAD's an odd length */
            if (sty == GS_HEAD)
                len = 12u + 2u * g_n(&r, 2);
            if (amt > 1u || part == GP_CHORUS)
                g_drums(w, s->perc, &r, energy, 0, len, part == GP_INTRO);
            break;
        }
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
#define GEN_NPART 5u                                    /* INTRO .. OUTRO */
#define GEN_NOID 0xFFu                                  /* (no pattern) */
static const uint8_t GEN_ARR[8] = {GP_INTRO, GP_VERSE, GP_CHORUS, GP_VERSE, GP_CHORUS, GP_BRIDGE, GP_CHORUS, GP_OUTRO};
static uint8_t gen_sid[GR_NROLE][GEN_NPART];            /* each part's pattern a track (.bss: the pool is full) */
static uint8_t gen_lwas[sizeof lists];                  /* the lists as they were */
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
    fm1_irq_off();
    for (k = 0; k < GR_NROLE; k++) {
        if (!gen_role_on(k))
            continue;
        for (e = 0; e < NENT; e++) {
            lists.ent[k][e][0] = (uint8_t)(e < NELEM(GEN_ARR) ? gen_sid[k][GEN_ARR[e] - GP_INTRO] + 1u : 0u);
            lists.ent[k][e][1] = 0;
            lists.snd[k][e][0] = lists.snd[k][e][1] = 0;
        }
        lists.cur[k] = 0;
    }
    lists.song = SONG_ONCE;
    fm1_irq_on();
    for (k = 0; k < GR_NROLE; k++)
        if (gen_role_on(k))
            (void)gen_load(k, gen_sid[k][0]);           /* (the tracks on the INTRO) */
    ui.force = 1;
    return 0;
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
    if (gen.song && gen.made && gen_copies_top())
        gen_song_drop();                                /* (a part again after a SONG: the song's patterns back) */
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
    }
    gen.made = 0;
    gen.nseq = 0;
    ui_say("PUT BACK ", "AS BEFORE GEN");
    return 1;
}

/* -------------------------------------------------------- the sheet's hooks --- */
static const uint8_t GEN_WIN[GEN_NWIN][4] = {{GK_STYLE, GK_SEED, GK_VIBE, GK_ENERGY}, {GK_PART, GK_KEY, GK_TEMPO, GK_SOUNDS},
                                             {GK_DRUMS, GK_BASS, GK_KEYS, GK_LEAD}, {GK_COLOR, GK_COMPLEX, GK_SWING, GK_HUMAN}};
static const char *const GEN_LABEL[GK_N] = {"STYLE", "SEED", "VIBE", "ENERGY", "PART", "KEY", "TEMPO", "SOUNDS", "DRUMS",
                                            "BASS", "KEYS", "LEAD", "COLOR", "COMPLEX", "SWING", "HUMAN"};
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
    uint32_t id = GEN_WIN[gen.win % GEN_NWIN][k & 3u];
    if (id == GK_SEED)
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
    default: str_cpy(val, GEN_AMT[gen.v[id] & 3u], 12); break;
    }
}
static void gen_cards(void)
{
    static const uint8_t ICON[GK_N] = {ICON_X_EYE, ICON_X_STAR, ICON_TONE, ICON_LEVEL, ICON_X_SONG, ICON_SCALE, ICON_TEMPO,
                                       ICON_LOAD, ICON_DRUM, ICON_SUB, ICON_DRAWBAR, ICON_VOICE, ICON_ARP, ICON_ALGORITHM,
                                       ICON_SWING, ICON_GLIDE};
    uint32_t k;
    for (k = 0; k < 4u; k++) {
        uint32_t id = GEN_WIN[gen.win % GEN_NWIN][k], mx = GEN_MAX[id];
        char val[12];
        const char *unit;
        int32_t v = id == GK_SEED ? gen.seed : gen.v[id];
        gen_value(id, val, &unit);
        draw_column(k, GEN_LABEL[id], val, unit, (id >= GK_DRUMS && id <= GK_COLOR && !gen.v[id]) ? VR_DIM : VR_LIVE,
                    id == GK_SEED ? v * 1000 / 999 : mx ? v * 1000 / (int32_t)mx : -1, ICON[id]);
    }
}
static uint32_t gen_sig(void)
{
    uint32_t h = 2166136261u ^ gen.seed * 131u ^ gen.win * 977u ^ gen.made * 7u, k;
    for (k = 0; k < GK_N; k++)
        h = (h ^ gen.v[k]) * 16777619u;
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
    gen_sigil(10 + rad + 4, cy, rad, T_THEME, T_LINE);
    for (i = 0; i < GEN_NWIN; i++)                      /* the windows: SELECT's place */
        cv_rect(10 + rad + 4 - 15 + (int32_t)i * 9, cy + rad + 6, 6, 3, i == w ? T_ACCENT : T_RAISE);
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
    str_cpy(b, GEN_NOTE[p.key % 12u], sizeof b);
    str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), N_SCALE[p.scale % NELEM(N_SCALE)], sizeof b - str_len(b));
    str_cpy(b + str_len(b), "  ", sizeof b - str_len(b));
    fmt_int(b + str_len(b), p.bpm);
    str_cpy(b + str_len(b), " BPM", sizeof b - str_len(b));
    cv_free_text(x0, y - AF_S_CAP_Y, &AF_S, b, T_THEME, cv_bg, 232 - x0);
    y += 18;
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
