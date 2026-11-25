/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* DRUM ((Vallée, the SPAMPY sound): SPAMPY's drum player, its drums.c and drum_synth.c (github.com/Spampy-Droid/
 * SPAMPY-0.0-r949, on SLOOP by isod89 and Felucca, GPL-3.0), as an engine any track plays.
 *
 * KIT (E0): SPAMPY's 35. ACOUSTIC, the General MIDI sample kit (the factory block's PERC set, eng_sample.c), and its four
 * treatments DEEP (pitched down, darker), TIGHT (a short fade), BRIGHT (up 2 semitones), DUST (darker, 8 bits); then 30
 * synthesised kits (drum_synth.c, tools/spampy/gen_drumkits.py: 808 .. LINNDRUM). Not taken: SPAMPY's user kits USR1..
 * (the FM-1's sample area holds the factory samples) and SYN1..SYN4 (their own flash bank and editor page).
 *
 * The 16 lanes (core.h NLANE, SPAMPY's order: KICK .. COWBELL), each with its TUNE (-8 .. +7 semitones), DECAY (-4 ..
 * +3) and BODY (-4 .. +4, a low reinforcement; 0: none, bit for bit), as SPAMPY's DRUMS > SOUND, and Vallée's LEVEL and
 * PAN (EDIT > LANES, LANE MIX). Notes: a GM note plays its sound (SPAMPY: the sample kit its zone, a synthesised kit
 * DS_MAP's lane and pitch); the lane it counts as (TUNE DECAY BODY LEVEL PAN, the mutes) is LANE_OF_GM's. The keys:
 * SPAMPY's, a white key a lane from F3 (KICK) to G5 (COWBELL), a black key its left neighbour's.
 *
 * Voices: SPAMPY's 6 (DRUM_NV), from the shared budget: a hit takes a free one, else the oldest (its sound fades out,
 * no click); a closed or pedal hat chokes the open one. Each voice its own envelope (engine_t.ownenv): the ADSR, GLIDE,
 * the pitch LFO and the matrix's PITCH do nothing; LEVEL, PAN, the sends, DIST, the SLICER, LFO AMP and velocity (the
 * hit's: SPAMPY's own velocity response) do.
 *
 * State: the part's shared engine area (engines.c eng_mem): the voices' synth state and their sample treatments. */
#include "drum_synth.c"
/* a kit engine (the grid's lanes, its voices by lane): ((Vallée) DRUM alone, DRUM-X retired) */
static inline int drum_engine(uint32_t e) { return e == ENGI_DRUM; }

/* ----------------------------------------------------- the grid's lanes --- */
/* A step's lane hits (step_t.hit / acc, the DRUM grid: SEQ > NOTES on a DRUM track) play these GM notes, on any
 * engine: DRUM strikes its lanes, a synth plays the pitches. Each is the designed pitch of
 * its lane (st 0 in DRUM_GM) */
/* ((Vallée, the SPAMPY sound) SPAMPY's 16 sounds (its drums.c LANE_NOTE), one per white key F3 (kick) .. G5 (cowbell) */
static const uint8_t DRUM_LANE_NOTE[NLANE] = {36, 35, 38, 39, 42, 46, 44, 37, 40, 43, 48, 49, 51, 70, 63, 56};
/* a GM note -> its lane: the nearest sound of the 16 (35..81; below: kick, above: shaker): SPAMPY's LANE_OF_GM */
static const uint8_t LANE_OF_GM[81 - 35 + 1] = {
    /* 35 */ 1, 0, 7, 2, 3, 8, 9, 4, 9, 6,
    /* 45 */ 9, 5, 10, 10, 11, 10, 12, 11, 12, 13,
    /* 55 */ 11, 15, 11, 13, 12, 14, 14, 14, 14, 14,
    /* 65 */ 14, 14, 15, 15, 13, 13, 15, 15, 13, 13,
    /* 75 */ 7, 7, 7, 14, 14, 15, 15};
/* the lane a note strikes (the grid, the keys, MIDI in: every note a lane's) */
static uint32_t drum_lane(uint32_t note)
{
    return note < 35u ? 0u : note > 81u ? 13u : LANE_OF_GM[note - 35u];
}
/* the lane's name as the track's KIT plays it (5 characters at most) */
/* (Skiff 1.3, from Felucca 1.4.1) the lanes' mutes, a bit each a track (KICK .. BELL), DRUM's and DRUM-X's: the EDIT
 * layer's black keys 1..8 (ui_layer.c). Performance state: not saved; a sound load clears the track's, a project load
 * every track's. A muted lane's voice rings on unheard (its hits still take a voice: un-muting is seamless) */
static uint16_t drum_mute[NTRK];   /* ((Vallée) bit l: grid lane l muted (16) */
static const char *drum_lane_name(const track_t *t, uint32_t l)   /* ((Vallée) SPAMPY's names, 8 characters at most */
{
    static const char *const N[NLANE] = {"KICK", "KICK 2", "SNARE", "CLAP", "HAT", "OPEN HAT", "PEDAL", "RIM",
                                         "SNARE 2", "LOW TOM", "HI TOM", "CRASH", "RIDE", "SHAKER", "CONGA", "COWBELL"};
    (void)t;
    return N[l & (NLANE - 1u)];
}
/* .. in 5 letters at most (SPAMPY's LANE_SHORT): a lane's parameter in the AUTOMATION list (KICK TUNE, OPEN PAN) */
static const char *drum_lane_short(const track_t *t, uint32_t l)
{
    static const char *const N[NLANE] = {"KICK", "KICK2", "SNARE", "CLAP", "HAT", "OPEN", "PEDAL", "RIM",
                                         "SNR 2", "TOM L", "TOM H", "CRASH", "RIDE", "SHAKE", "CONGA", "BELL"};
    (void)t;
    return N[l & (NLANE - 1u)];
}
/* .. in two letters, the drum machine way (the grid's lane column) */
static const char *drum_lane_abbr(const track_t *t, uint32_t l)
{
    static const char *const N[NLANE] = {"BD", "B2", "SD", "CP", "CH", "OH", "PH", "RS",
                                         "S2", "LT", "HT", "CR", "RD", "SH", "CG", "CB"};
    (void)t;
    return N[l & (NLANE - 1u)];
}

/* the lanes step s strikes: its hits, and its notes on their lanes (a NOTE step only) */
static uint32_t step_lanes(const step_t *s)
{
    uint32_t m = 0, i;
    if (s->time != ST_NOTE)
        return 0;
    for (i = 0; i < s->n && i < 4u; i++)
        m |= 1u << drum_lane(s->note[i]);
    return m | s->hit;
}

/* .. and which of them are accented */
static uint32_t step_accents(const step_t *s) { return s->flags & SF_ACCENT ? step_lanes(s) : s->acc & step_lanes(s); }

/* a step's notes become their lanes' hits (drum_lane: every GM note has its nearest of the 16: (Vallée) SPAMPY's
 * way, so a DRUM track's step holds hits only, never notes). A step accent of a step left with hits only becomes
 * the hits' accents. Pattern loads into a DRUM track, projects of before the grid, the grid's edits */
static void step_to_grid(step_t *s)
{
    uint32_t i, k = 0;
    if (s->time != ST_NOTE)
        return;
    for (i = 0; i < s->n && i < 4u; i++)
        s->hit |= (uint16_t)(1u << drum_lane(s->note[i]));
    for (i = k; i < 4u; i++)
        s->note[i] = 0;
    s->n = (uint8_t)k;
    if (!k && (s->flags & SF_ACCENT)) {
        s->acc |= s->hit;
        s->flags &= (uint8_t)~SF_ACCENT;
    }
}

/* ---------------------------------------------------------- the kits --- */
#define DRUM_SAMPLED 5u                         /* KIT 0..4: the GM sample kit and its treatments; 5..: drum_synth.c */
#define DRUM_KITS (DRUM_SAMPLED + DS_NKITS)
#define DRUM_NV 6u                              /* SPAMPY's NDRUM: the voices a kit sounds at once */
static const char *const N_DRUM_KIT[] = {"ACOUSTIC", "DEEP", "TIGHT", "BRIGHT", "DUST", DS_KIT_NAME_LIST};
_Static_assert(sizeof(N_DRUM_KIT) / sizeof(N_DRUM_KIT[0]) == DRUM_KITS, "a name per kit");
static const uint8_t DK_SMP[4] = {47, 60, 74, 87};   /* a sample's DECAY -4 .. -1: -60 dB in 60, 120, 250, 500 ms */
static uint32_t drum_kit(const track_t *t) { return (uint32_t)clamp(t->p[P_E0], 0, DRUM_KITS - 1); }

/* a part's voices' state (the part's shared area): voice i of the track (its DRUM_NV) uses slot i */
typedef struct {
    dsv_t ds[DRUM_NV];                          /* a synthesised kit's voice (drum_synth.c) */
    int32_t filter[DRUM_NV], env[DRUM_NV], denv[DRUM_NV];   /* the sample kit's treatments, its DECAY's fade (Q15) */
    int32_t bodylp[DRUM_NV];                    /* BODY's low band */
    int32_t tail[DRUM_NV];                      /* a hit on a sounding voice: where it was, falling away (no click) */
    int32_t last[DRUM_NV];                      /* the voice's last output (the tail's start) */
    int32_t choke[DRUM_NV];                     /* a choked open hat: its fade out, Q15 (0: not choked) */
    uint16_t dk[DRUM_NV];                       /* the sample's DECAY factor a sample (DECAY_K), 0: none */
    uint8_t kit[DRUM_NV], synth[DRUM_NV], lane[DRUM_NV], over[DRUM_NV];
    uint32_t seed;                              /* the synth voices' noise seeds (drum_synth.c ds_on) */
    int16_t set;                                /* SMP_SETS' PERC: its index + 1, 0 not looked for, -1 none */
} drum_part_t;

static drum_part_t *drum_part_of(const track_t *t)
{
    return t >= &trk[0] && t < &trk[NPART] ? (drum_part_t *)eng_part(t) : 0;   /* (the shared area, engines.c) */
}
/* the lane voice v counts as (LANES' LEVEL, LANE MIX's PAN: voice.c lpan_render, from RAM: no call; the mutes): its
 * s[6], set by drum_note_on */
static inline __attribute__((always_inline)) uint32_t drum_vlane(const track_t *t, const voice_t *v)
{
    (void)t;
    return (uint32_t)v->s[6] & 15u;
}
static int drum_lane_has(const track_t *t, uint32_t l) { (void)t; (void)l; return 1; }   /* (no user kits) */
static voice_t *drum_reuse(track_t *t, uint32_t note) { (void)t; (void)note; return 0; }   /* (SPAMPY: a free voice) */

static int32_t drum_perc_set(drum_part_t *P)
{
    uint32_t i;
    if (!P->set) {
        P->set = -1;
        for (i = 0; i < SMP_NSETS; i++)
            if (str_eq(SMP_SETS[i].name, "PERC"))
                P->set = (int16_t)(i + 1u);
    }
    return P->set > 0 && !smp_set_missing((uint32_t)P->set - 1u) ? P->set - 1 : -1;
}

/* BODY: a low reinforcement (+) or its removal (-) on lane l's voice k; 0 bypassed (SPAMPY's drum_body_apply) */
static inline int32_t drum_body(drum_part_t *P, uint32_t k, int32_t body, int32_t s)
{
    P->bodylp[k] += (s - P->bodylp[k]) >> 5;
    if (body >= 0)
        return s + (int32_t)(((int64_t)P->bodylp[k] * body * 14000) >> 15);
    return s - (int32_t)(((int64_t)P->bodylp[k] * (-body) * 8192) >> 15);
}

static void drum_note_on(track_t *t, voice_t *v)
{
    drum_part_t *P = drum_part_of(t);
    uint32_t k = (uint32_t)(v - t->v), note = v->note, kit = drum_kit(t), ln = drum_lane(note), i;
    int32_t tune = t->p[lp_tune(ln)], dec = t->p[lp_dec(ln)];
    if (!P || k >= DRUM_NV)
        return;
    tune = clamp(tune, -8, 7);
    dec = clamp(dec, -4, 3);
    P->tail[k] = v->active && !P->over[k] ? P->last[k] : 0;   /* (a sounding voice taken: from where it was) */
    P->last[k] = 0;
    P->over[k] = 0;
    P->choke[k] = 0;
    P->kit[k] = (uint8_t)kit;
    P->lane[k] = (uint8_t)ln;
    v->s[6] = (int32_t)ln;                       /* (drum_vlane) */
    P->bodylp[k] = 0;
    if (note == 42u || note == 44u)             /* a closed or pedal hat chokes the open one */
        for (i = 0; i < DRUM_NV; i++)
            if (i != k && t->v[i].active && t->v[i].note == 46u && !P->over[i] && !P->choke[i])
                P->choke[i] = 32767;
    v->env = 1 << 24;                            /* (the ADSR held: the voice is its own envelope, drum_done) */
    if (kit >= DRUM_SAMPLED) {                   /* a synthesised kit */
        const dkit_t *d = &DS_KITS[kit - DRUM_SAMPLED];
        P->synth[k] = 1;
        P->seed = P->seed * 1664525u + 1013904223u;
        ds_on(&P->ds[k], d->s, d->crush, note, v->vel, tune * 16, (uint32_t)(dec + 4), P->seed);
        return;
    }
    P->synth[k] = 0;                             /* the GM sample kit: the note's zone of PERC */
    {
        int32_t si = drum_perc_set(P), shift;
        uint32_t zi = 0xFFFFu;
        const smp_zone_t *z;
        if (si >= 0)
            for (i = 0; i < SMP_SETS[si].nz; i++)
                if (note >= SMP_ZONES[SMP_SETS[si].z0 + i].lo && note <= SMP_ZONES[SMP_SETS[si].z0 + i].hi)
                    zi = SMP_SETS[si].z0 + i;
        if (zi == 0xFFFFu) {                     /* (no sample there, or the factory samples not written: silent) */
            P->over[k] = 1;
            return;
        }
        v->s[4] = (int32_t)zi;
        v->ph[0] = v->ph[1] = 0;
        v->s[0] = v->s[1] = v->s[2] = 0;
        z = smp_zone(zi);
        v->s[3] = sample_next(z, v, 0);
        shift = kit == 1u ? (note <= 36u ? -5 : -2) : kit == 3u ? 2 : kit == 4u ? -1 : 0;   /* DEEP BRIGHT DUST */
        shift = shift * 16 + tune * 16;
        v->s[5] = (int32_t)((pow2_q16((int32_t)note * 16 + shift - z->root16) >> 8) * (z->rate >> 8));
        P->filter[k] = 0;
        P->env[k] = 32767;
        P->denv[k] = 32767;
        P->dk[k] = dec < 0 ? DECAY_K[DK_SMP[dec + 4]] : 0u;
    }
}

/* the voice's amplitude: its own (drum_render), at full here */
static int32_t drum_amp(track_t *t, voice_t *v, int32_t env)
{
    (void)t;
    (void)v;
    (void)env;
    return 32767;
}
/* .. ended: its sound over, its fade done */
static int drum_done(track_t *t, voice_t *v)
{
    const drum_part_t *P = drum_part_of(t);
    uint32_t k = (uint32_t)(v - t->v);
    return !P || k >= DRUM_NV || (P->over[k] && !P->tail[k]);
}

static __attribute__((noinline)) void drum_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    drum_part_t *P = drum_part_of(t);
    uint32_t k = (uint32_t)(v - t->v), i, ln;
    int32_t y[CTL], body, tail, ck;
    vmod_t ml;
    int heard;
    if (!P || k >= DRUM_NV)
        return;
    n = n > CTL ? CTL : n;
    ln = P->lane[k];
    body = clamp(t->p[lp_body(ln)], -4, 4);
    heard = !((drum_mute[(uint32_t)(t - trk) % NTRK] >> ln) & 1u);
    m = vmod_level(m, &ml, clamp(t->p[lp_ln(ln)], 0, 127));   /* the lane's LEVEL (dsp.c) */
    if (P->over[k]) {
        for (i = 0; i < n; i++)
            y[i] = 0;
    } else if (P->synth[k]) {                    /* a synthesised kit's voice */
        if (!ds_render(&P->ds[k], y, n))
            P->over[k] = 1;
    } else {                                     /* the sample kit's: SPAMPY's drums_mix */
        const smp_zone_t *z = smp_zone((uint32_t)v->s[4]);
        uint32_t frac = v->ph[1], stepq = (uint32_t)v->s[5], kt = P->kit[k];
        int32_t g = v->vel * 258;
        g += g * 3 >> 2;                         /* x1.75 (+5 dB): as loud as the synthesised kits */
        for (i = 0; i < n; i++) {
            int32_t s;
            if (P->over[k]) {
                y[i] = 0;
                continue;
            }
            frac += stepq;
            while (frac >= 65536u) {
                frac -= 65536u;
                v->s[2] = v->s[3];
                if (v->ph[0] >= z->n) {
                    P->over[k] = 1;
                    break;
                }
                v->s[3] = sample_next(z, v, 0);
            }
            if (P->over[k]) {
                y[i] = 0;
                continue;
            }
            s = v->s[2] + (((v->s[3] - v->s[2]) * (int32_t)(frac >> 1)) >> 15);
            s = mulq15(s, g);
            if (kt == 1u || kt == 4u) {          /* DEEP, DUST: darker; DUST 8 bits */
                P->filter[k] += (s - P->filter[k]) >> (kt == 1u ? 2 : 1);
                s = P->filter[k];
                if (kt == 4u)
                    s = (s >> 8) * 256;
            } else if (kt == 2u) {               /* TIGHT: a short fade */
                s = mulq15(s, P->env[k]);
                P->env[k] -= (P->env[k] >> 11) + 1;
                if (P->env[k] <= 0)
                    P->over[k] = 1;
            }
            if (P->dk[k]) {                      /* a shorter DECAY: the sample fades out */
                s = mulq15(s, P->denv[k]);
                P->denv[k] = (int32_t)(((uint32_t)P->denv[k] * P->dk[k]) >> 16);
                if (P->denv[k] < 8)
                    P->over[k] = 1;
            }
            y[i] = s;
        }
        v->ph[1] = frac;
    }
    tail = P->tail[k];
    ck = P->choke[k];
    for (i = 0; i < n; i++) {
        int32_t s = y[i];
        if (body)
            s = drum_body(P, k, body, s);
        if (ck) {                                /* choked: gone in ~1.5 ms */
            s = mulq15(s, ck);
            ck -= ck >> 4;
            if (ck < 64) {
                ck = 0;
                P->over[k] = 1;
                P->choke[k] = 0;
                s = 0;
            }
        }
        P->last[k] = s;
        s += tail;                               /* (a hit on a sounding voice: the old sound falling away) */
        tail -= tail / 16 + (tail > 0 ? 1 : tail < 0 ? -1 : 0);
        if (heard)
            out[i] += voice_amp2(s, m, i);
    }
    if (P->choke[k])
        P->choke[k] = ck ? ck : 0;
    P->tail[k] = tail;
}

/* SPAMPY's keys: a white key a lane, F3 (key 0) KICK .. G5 COWBELL; a black key its left neighbour's */
static int32_t drum_keys(const track_t *t, uint32_t k)
{
    static const int8_t W[12] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6};   /* from F */
    int32_t i = W[k % 12u];
    (void)t;
    if (i < 0)
        i = W[(k + 11u) % 12u];
    return DRUM_LANE_NOTE[(uint32_t)((int32_t)(k / 12u) * 7 + i) & 15u];
}

/* the kits, each a sound: KIT, the rest the lanes' (the track's own: LANES, LANE MIX) */
#define DK(k, nm) {nm, {k, 0, 0, 0, 0, 0, 0, 0}, {0, 100, 127, 100}, 0, 0, FX(0, 0, 0, 16), TAG(PT_DRUM)}
static const preset_t DRUM_PRESETS[] = {
    DK(0, "ACOUSTIC"), DK(1, "DEEP"), DK(2, "TIGHT"), DK(3, "BRIGHT"), DK(4, "DUST"), DK(5, "808"), DK(6, "909"),
    DK(7, "606"), DK(8, "80S"), DK(9, "VINTAGE"), DK(10, "TRAP"), DK(11, "DRILL"), DK(12, "BOOMBAP"),
    DK(13, "LO-FI"), DK(14, "PHONK"), DK(15, "HOUSE"), DK(16, "TECHNO"), DK(17, "ELECTRO"), DK(18, "DISCO"),
    DK(19, "GARAGE"), DK(20, "JUNGLE"), DK(21, "DUBSTEP"), DK(22, "DEMBOW"), DK(23, "AMAPIANO"), DK(24, "AFRO"),
    DK(25, "SYNTHWV"), DK(26, "CHIP"), DK(27, "GLITCH"), DK(28, "INDUSTR"), DK(29, "HYPER"), DK(30, "AMBIENT"),
    DK(31, "JAZZ"), DK(32, "LM-1"), DK(33, "LINNDRUM"), DK(34, "SYNWAVE"),
};
#undef DK
_Static_assert(sizeof(DRUM_PRESETS) / sizeof(DRUM_PRESETS[0]) == DRUM_KITS, "a sound per kit");

static const engine_t ENG_DRUM = {
    .name = "DRUM",
    .page_title = {"KIT", "-"},
    .edit = {
        {"KIT", N_DRUM_KIT, 0, DRUM_KITS - 1, DRUM_SAMPLED, F_ENUM, 0},   /* (808 at power-on, as SPAMPY's) */
        {"-", 0, 0, 0, 0, F_INT, 0},
        {"-", 0, 0, 0, 0, F_INT, 0},
        {"-", 0, 0, 0, 0, F_INT, 0},
        {"-", 0, 0, 0, 0, F_INT, 0},
        {"-", 0, 0, 0, 0, F_INT, 0},
        {"-", 0, 0, 0, 0, F_INT, 0},
        {"-", 0, 0, 0, 0, F_INT, 0},
    },
    .presets = DRUM_PRESETS,
    .npresets = NELEM(DRUM_PRESETS),
    .note_on = drum_note_on,
    .render = drum_render,
    .amp = drum_amp,
    .knob = {P_E0, P_DIST, P_DLY, P_REV},
    .poly = DRUM_NV,
    .sampled = 1,
    .oneshot = 1,
    .keys = drum_keys,
    .ownenv = 1,
    .done = drum_done,
};
