/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Engine table (order = PRESETS browsing order and the engine numbers of the editor protocol), the
 * the parts' sounds at power-on. */
/* Skiff: the parts' shared engine memory (eng_mem, after the engines): each engine's per-part state lives in its
 * part's area, never in arrays of its own for all the parts */
static void *eng_part(const track_t *t);
#include "dsp.c"
#include "eng_sample.c"          /* (Vallée: the factory sample block; SPAMPY's sets, phase 1b) */
#include "eng_drum.c"           /* DRUM: the 8-lane kit (drum_voice.c); (Vallée) SPAMPY's kits: phase 3 */
#include "eng_drumx.c"          /* (Skiff) DRUM-X: retired with the SPAMPY sound (ENGI_DRUMX never matches); cleanup */
#include "eng_summon.c"          /* (Vallée) SUMMON: SUMMONER (XENO), AURA (TEEN) */
#include "sloop/sloop_engines.c"    /* ((Vallée) the SPAMPY sound: SLOOP's engines, as SPAMPY uses them) */
#include "eng_fm6.c"            /* FM6: 6-operator FM, msfa ported (fm6_core.c, Apache-2.0) */
/* (fm4_convert.c: DIGITAL -> FM6 conversion retired: SPAMPY plays DIGITAL itself; old DIGITAL tracks map to FM6, core.h) */
/* (DIGITAL: sloop/eng_digital.c) */
#if FELUCCA_SLICE
#include "eng_slice.c"
#endif

/* Skiff: a track runs one engine at a time, and an engine switch fades the old one out before the new one starts
 * (voice.c engine_block), so a part's engines share one area, as big as the biggest's (PHYS: its 3 slots): ~48 KB for
 * the four parts instead of ~99 KB. Only the audio interrupt touches it, through the engines' own accessors (their
 * notes, blocks and renders); the switch zeroes it, the state each engine starts from at power-on (GRAIN rebuilds
 * its index, PHYS its model, from zero). Data the main loop writes (FM6's patches, WHEEL's bars) stays out. */
typedef union {
    drum_lane_t drum[DV_NLANE];
    drumx_lane_t drumx[DV_NLANE];
    fm6_note_t fm6[FM6_POLY];
    drw_part_t drawbar;              /* ((Vallée) SLOOP's WHEEL */
    p5_part_t prophet;               /* ((Vallée) SLOOP's engines, sloop/sloop_engines.c: their part state here, as */
    cz_part_t cz;                    /* SPAMPY's eng_arena: zeroed by the switch) */
    sid_part_t sid;
    obv_part_t obervave;
#if FELUCCA_SLICE
    int16_t slice[SLC_NV][SLC_RB];   /* SLICE's reverse windows */
#endif
} eng_mem_t;
static eng_mem_t eng_mem[NHEAVY] __attribute__((section(".pool")));
/* (Skiff phase 6) a light track's area (the tracks past NHEAVY): the engines it may play, no PHYS, no GRAIN (their
 * accessors give it none: a heavy engine forced there is silent); DRUM's lanes the biggest, 1.8 KB */
typedef union {
    drum_lane_t drum[DV_NLANE];
    drumx_lane_t drumx[DV_NLANE];
    fm6_note_t fm6[FM6_POLY];
    drw_part_t drawbar;
    /* ((Vallée) SLOOP's engines: not on the light tracks yet (the pool's 8 KB headroom); phase 2b, once PHYS and
     * GRAIN leave the heavy area, decides: until then eng_allowed keeps them off these tracks) */
#if FELUCCA_SLICE
    int16_t slice[SLC_NV][SLC_RB];
#endif
} eng_lite_t;
static eng_lite_t eng_lite[NTRK - NHEAVY] __attribute__((section(".pool")));
static void *eng_part(const track_t *t)
{
    uint32_t k = (uint32_t)(t - trk) % NPART;
    if (k >= NHEAVY)
        return &eng_lite[k - NHEAVY];
    return &eng_mem[k];
}
/* ((Vallée) SLOOP's engines ask for their part's state by the part's number (SPAMPY's engines.c accessors) */
_Static_assert(sizeof(eng_mem_t) >= sizeof(obv_part_t) && sizeof(eng_mem_t) >= sizeof(cz_part_t) &&
               sizeof(eng_mem_t) >= sizeof(p5_part_t) && sizeof(eng_mem_t) >= sizeof(sid_part_t), "SLOOP's parts");
static p5_part_t *p5_part(uint32_t p) { return p < NPART ? (p5_part_t *)eng_part(&trk[p]) : 0; }
static cz_part_t *cz_part(uint32_t p) { return (cz_part_t *)eng_part(&trk[p % NPART]); }
static sid_part_t *sid_part(uint32_t p) { return (sid_part_t *)eng_part(&trk[p % NPART]); }
static obv_part_t *obv_part(uint32_t p) { return (obv_part_t *)eng_part(&trk[p % NPART]); }
static uint32_t eng_size(const track_t *t)            /* its area's bytes (the engine switch zeroes them) */
{
    if ((uint32_t)(t - trk) % NPART >= NHEAVY)
        return sizeof(eng_lite_t);
    return sizeof(eng_mem_t);
}

/* the editor protocol, user presets and projects store these indices: append, never reorder */
/* ((Vallée, the SPAMPY sound) SPAMPY's numbers 0..18 (core.h); its reserved ones play nothing and are never offered
 * (eng_ok): an empty engine, as DIGITAL's was */
/* a reserved number: never offered (eng_ok), and a track that holds one anyway (a damaged store) plays silence, never
 * a call through 0 */
static void rsv_note_on(track_t *t, voice_t *v) { (void)t; v->stage = 0; }
static void rsv_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    (void)t; (void)out; (void)n; (void)m;
    v->stage = 0;
}
static const engine_t ENG_FM4_GONE = {.name = "-", .page_title = {"-", "-"}, .note_on = rsv_note_on, .render = rsv_render};
static const engine_t *const ENGINES[NENGINES] = {
    &ENG_ANALOG,                 /* 0 */
    &ENG_DIGITAL,                /* 1 */
    &ENG_PHASE,                  /* 2 */
    &ENG_LOFI,                   /* 3 */
    &ENG_SAMPLE,                 /* 4 */
    &ENG_FORMANT,                /* 5 VOICE */
    &ENG_TRIO,                   /* 6 */
    &ENG_DRAWBAR,                /* 7 WHEEL (SLOOP's drawbar organ) */
    &ENG_FM4_GONE,               /* 8: reserved */
    &ENG_FM6,                    /* 9 (ENGI_FM6) */
    &ENG_FM4_GONE,               /* 10: reserved */
    &ENG_WAVETABLE,              /* 11 MORPH (ENGI_NOISE) */
    &ENG_FM4_GONE,               /* 12: reserved (SLICE) */
    &ENG_PROPHET,                /* 13 */
    &ENG_CZ,                     /* 14 */
    &ENG_SID,                    /* 15 */
    &ENG_OBERVAVE,               /* 16 */
    &ENG_FM4_GONE,               /* 17: reserved (MORPH's second number in SPAMPY) */
    &ENG_DRONE,                  /* 18 */
    &ENG_DRUM,                   /* 19 (ENGI_DRUM, Vallée) */
    &ENG_SUMMON,                 /* 20 (ENGI_SUMMON, Vallée) */
};

#include "sloop/sloop_presets.c"   /* ((Vallée, the SPAMPY sound) SPAMPY's preset map and level trims */
/* ((Vallée) engine e's number (ENGINES[]), NENGINES if none */
static uint32_t eng_num(const engine_t *e)
{
    uint32_t i;
    for (i = 0; i < NENGINES && ENGINES[i] != e; i++)
        ;
    return i;
}

/* a track's engine number as an index (the audio paths: a compare, cheaper than % NENGINES; a bad number: 0) */
static inline uint32_t eng_idx(uint32_t e) { return e < NENGINES ? e : 0u; }

/* the sample source a track's sound plays that has no data (SAMPLE's SET, GRAIN's SRC, SLICE's SRC: an empty or
 * invalid user slot, a set or PIANO this build lacks): its name (it plays a sine: eng_sample.c smp_sine), 0 = none;
 * *code a small number for it (1.., per engine and source) so the UI says it once (ui_input.c sample_notice) */
static uint8_t snd_said[NTRK];   /* per track: the code last said, 0 = none (a project load clears them: said again) */
static const char *snd_missing(const track_t *t, uint32_t *code)
{
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    uint32_t si = (uint32_t)t->p[P_E0] % SMP_NALL;
#if FELUCCA_SLICE
    if (e == &ENG_SLICE) {
        si = slc_src_of(t->p);
        *code = 64u + si;
        return slc_get(si) ? 0 : N_SLC_SRC[si];
    }
#endif
    if (e != &ENG_SAMPLE)
        return 0;
    *code = 1u + si % 32u;
    return smp_set_missing(si) ? SMP_ALL_NAMES[si] : 0;
}

/* the order the engines are shown in (PRESETS browsing and its ENG knob, the EDIT layer's keys, the editor's list):
 * engine indices, never DIGITAL's reserved 1 (with FELUCCA_FM4 it follows FM6). The indices stay as they are (the
 * stores and the protocol hold them); only this table orders them */
static const uint8_t ENGINE_ORDER[NENG_SHOWN] = {
    0, ENGI_PROPHET, ENGI_OBERVAVE, 6, 9, 1, ENGI_CZ, 3, 2, 5, 7, ENGI_NOISE, ENGI_DRONE,   /* ((Vallée) SPAMPY's
                                  * engines, the most used first: the EDIT layer's keys hold the first 15) */
    ENGI_SUMMON,                 /* (Vallée) SUMMON */
    ENGI_DRUM,                   /* DRUM */
    ENGI_SID, 4,                 /* SID, SAMPLE (2 and 1 of SPAMPY's sounds): PRESETS' ENG knob, not the EDIT layer's keys */
};

/* the engines one can pick (engine 1 only with FELUCCA_FM4), in ENGINE_ORDER: eng_ok(e), the n-th of them
 * eng_vis(n), e's place among them eng_rank(e), the next / previous one eng_step(e, dir) (wraps) */
static int eng_ok(uint32_t e)
{
    return e < NENGINES && ENGINES[e] != &ENG_FM4_GONE;   /* ((Vallée) SPAMPY's reserved numbers: 8 10 12 17) */
}
static uint32_t eng_vis(uint32_t n) { return ENGINE_ORDER[n % NENG_SHOWN]; }
static uint32_t eng_rank(uint32_t e)
{
    uint32_t n;
    if (!eng_ok(e))
        e = 0;                                   /* ((Vallée) a reserved number: shown as ANALOG's place) */
    for (n = 0; n < NENG_SHOWN && ENGINE_ORDER[n] != e; n++)
        ;
    return n < NENG_SHOWN ? n : 0u;
}
static uint32_t eng_step(uint32_t e, int32_t dir)
{
    return eng_vis((eng_rank(e % NENGINES) + (dir > 0 ? 1u : NENG_SHOWN - 1u)) % NENG_SHOWN);
}
/* (Skiff phase 6) engine e on track k: every one on the first NHEAVY, the light ones after (no PHYS, no GRAIN: core.h
 * NHEAVY). The one rule: picking, presets, user presets, the editor, projects ask it */
static int eng_allowed(uint32_t k, uint32_t e)
{
    return k < NHEAVY || (e % NENGINES != ENGI_PROPHET && e % NENGINES != ENGI_CZ && e % NENGINES != ENGI_SID &&
                          e % NENGINES != ENGI_OBERVAVE);   /* ((Vallée) SLOOP's big part state: the heavy area only) */
}
/* ((Vallée, the SPAMPY sound) the engine the audio plays for a track: what it asks for, unless that may not play there
 * (a light track and one of SLOOP's big engines, whichever path asked: MIDI, a store, the editor): then a reserved
 * number, silence, never another engine's state past the light track's area */
static inline uint32_t eng_eff(const track_t *t)
{
    uint32_t e = eng_idx(t->eng_req);
    return eng_allowed((uint32_t)(t - trk) % NTRK, e) ? e : ENGI_GRAIN;   /* (8: reserved, silent) */
}
static uint32_t eng_step_for(uint32_t k, uint32_t e, int32_t dir)   /* eng_step past what track k may not play */
{
    uint32_t n = 0;
    do
        e = eng_step(e, dir);
    while (!eng_allowed(k, e) && ++n < NENG_SHOWN);
    return e;
}

/* the parts at power-on (engine, preset; the sequencers empty): bass, pad, lead, drums, (phase 6) a sample sound
 * and a second kit: ANALOG ACID, FM6 PAD (was DIGITAL PAD), LOFI PULSE LD, DRUM KIT, PIANO, DRUM KIT */
/* ((Vallée, the SPAMPY sound) as SPAMPY powers on (its ui.c TRK_DEF: ANALOG, DIGITAL RHODES, LOFI; its ANALOG and LOFI
 * picks were trimmed from its list, so it plays those engines' plain settings): ANALOG JP8 PAD, RHODES, LOFI CHIP CHORD,
 * then Vallée's DRUM, SAMPLE VIBES, DRUM */
static const uint8_t TRK_DEF[NPART][2] = {{0, 11}, {1, 0}, {3, 4}, {ENGI_DRUM, 0}, {ENGI_SAMPLE, 0}, {ENGI_DRUM, 0}};
static uint32_t trk_def_engine(uint32_t i) { return TRK_DEF[i % NPART][0]; }
