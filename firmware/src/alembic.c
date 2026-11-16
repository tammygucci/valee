/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) GLO > ALEMBIC: the whole mix through four vessels, in this order:
 *   DRIVE   SOFT HARD FOLD FUZZ        the INSERT's shapers (ins_drive): BLEND drives harder and mixes more in
 *   MOTION  PHASR FLANG CHOR WOBBL     the INSERT's sweeps (ins_swept), left and right a quarter apart; WOBBL its
 *                                      chorus line alone, slow and narrow: a vibrato (BLEND: how much)
 *   SPACE   ECHO VERB BOTH             the mix sent into the delay and reverb buses as they are set (GLO > DELAY,
 *                                      REVERB), as the FX layer's THROW sends it (perform.c perf_pre)
 *   GRAIN   WOW DUST CRUSH FILTR       WOW: a tape's wow and flutter (VINYL's moving delay, perform.c perf_mdl, in the
 *                                      stage's own line) and its top rolled off; DUST: VINYL's crackle; CRUSH and
 *                                      FILTR the INSERT's (ins_drive, ins_filter), BLEND fewer bits / a lower cutoff
 * DRIVE, MOTION and SPACE take the dry mix (before the buses), GRAIN the mix with the buses' wet in it (mix_block).
 * Each stage's flavour (G_ADRV ..) and BLEND (G_ADRVB .., 0..127): BLEND 0 and faded is the stage off, nothing runs and
 * the mix is bit for bit as without it. A flavour changed fades the old out (2.9 ms, SL_SLOPE), then the new in from
 * rest, as the INSERT's TYPE does (track_insert). The INSERT's code works on a voice's scale: the mix goes through it at
 * a quarter (its peaks within the INSERT's clamps) and comes back x4 with its 2 low bits (dry: to the bit).
 * State: three INSERT states a side (the .bss: the pool keeps its 8 KiB), SPACE's send, DUST's random, WOW's phases */
enum { AS_DRV, AS_MOV, AS_SPC, AS_GRN, AS_N };
#define IT_WOBBL (IT_N + 0u)                    /* ALEMBIC's own kinds, in ins_t.type beside the INSERT's */
#define IT_WOW (IT_N + 1u)
#define IT_DUST (IT_N + 2u)
static const uint8_t ALM_DRV_T[4] = {IT_SOFT, IT_HARD, IT_FOLD, IT_FUZZ};
static const uint8_t ALM_MOV_T[4] = {IT_PHASER, IT_FLANGER, IT_CHORUS, IT_WOBBL};
static const uint8_t ALM_GRN_T[4] = {IT_WOW, IT_DUST, IT_CRUSH, IT_FILTER};
static const int16_t ALM_MOV_P[4][3] = {      /* RATE (F_LFOHZ), DEPTH, FDBK as the INSERT's A B C */
    {40, 100, 60},                              /* PHASR: 0.41 Hz, wide, a peak beside the notches */
    {26, 110, 70},                              /* FLANG: 0.2 Hz, the jet */
    {54, 80, 0},                                /* CHOR: 0.85 Hz */
    {86, 40, 0},                                /* WOBBL: 4.6 Hz, +-0.7 ms: ~25 cents either way */
};
typedef struct {
    ins_t s[3][2];                              /* DRIVE, MOTION, GRAIN: left, right */
    int32_t sw;                                 /* SPACE: the send now, Q15 */
    uint8_t sty;                                /* .. the flavour sending */
    uint32_t seed, wph, fph;                    /* DUST's random; WOW's wow and flutter */
    int32_t ck;                                 /* DUST's pop */
    uint16_t lvl[AS_N];                         /* each stage's, the last block's peak of what it changed (the
                                                 * vessels, ui_pgraph.c graph_alembic) */
} alm_t;
static alm_t alm = {.seed = 0x5EEDu};
static int32_t alm_l[CTL], alm_r[CTL];          /* a stage's block at a quarter */

static inline int32_t alm_b(uint32_t g) { return clamp(song.g[g], 0, 127); }
static inline uint32_t alm_f(uint32_t g, uint32_t n) { return (uint32_t)clamp(song.g[g], 0, (int32_t)n - 1); }

/* WOW on one side: the line (ins_t.line, at a quarter of the quarter) read 1.4 ms back, the read swung by a wow of
 * 0.55 Hz (up to +-1 ms: +-0.35 %) and a flutter of 6.5 Hz (+-0.07 ms), d the depth (BLEND), through a one-pole
 * low-pass closing to ~5 kHz with it; ph, fp: the phases at the block's start (both sides the same: a tape) */
#define WOW_INC ((uint32_t)(0.55 / FS * 4294967296.0))
#define FLT_INC ((uint32_t)(6.5 / FS * 4294967296.0))
static __attribute__((noinline)) void alm_wow(ins_t *s, int32_t *b, uint32_t n, int32_t tw, int32_t d, uint32_t ph,
                                              uint32_t fp)
{
    uint32_t i;
    int32_t k = 32767 - d * 120;
    for (i = 0; i < n; i++, ph += WOW_INC, fp += FLT_INC) {
        int32_t x = b[i], r0, r1, v, fr;
        uint32_t q = (uint32_t)((62 << 16) + ((osc_sine(ph) * d * 46) >> 6) + ((osc_sine(fp) * d * 3) >> 6));   /* Q16 */
        s->w += clamp(tw - s->w, -SL_SLOPE, SL_SLOPE);
        s->line[s->wp] = (int16_t)clamp(x >> 2, -32767, 32767);
        r0 = (int32_t)(s->wp - (q >> 16)) & (int32_t)(IL_LEN - 1u);
        r1 = (r0 - 1) & (int32_t)(IL_LEN - 1u);
        fr = (int32_t)((q >> 1) & 0x7FFFu);
        v = (s->line[r0] + (((s->line[r1] - s->line[r0]) * fr) >> 15)) << 2;
        s->wp = (uint16_t)((s->wp + 1u) & (IL_LEN - 1u));
        s->lp1 += mulq16(v - s->lp1, (uint32_t)k << 1);
        b[i] = pf_mix(x, s->lp1, s->w);
    }
}

/* DUST: VINYL's crackle on both sides (the same pop), ~43 pops a second at BLEND 127, fewer and softer below it */
static __attribute__((noinline)) void alm_dust(int32_t *l, int32_t *r, uint32_t n, int32_t bl)
{
    uint32_t i;
    int32_t a = 64 + bl / 2;
    for (i = 0; i < n; i++) {
        alm.seed = alm.seed * 1664525u + 1013904223u;
        if (bl && (alm.seed >> 15) < (uint32_t)bl)
            alm.ck = ((int32_t)(alm.seed << 11) >> 21) * a / 127;
        l[i] += alm.ck;
        r[i] += alm.ck;
        alm.ck = alm.ck * 3 / 4;                /* (dying in a few samples) */
    }
}

/* a stage of INSERT states k (DRIVE MOTION GRAIN) of kind ty with its A B C (pp) at the share tw (Q15) on the mix (ml,
 * mr); d: WOW's depth. Nothing when it is off and faded (the mix bit for bit) */
static __attribute__((noinline)) void alm_stage(uint32_t k, uint32_t ty, const int16_t *pp, int32_t tw, int32_t d,
                                                int32_t *ml, int32_t *mr, uint32_t n)
{
    ins_t *s = alm.s[k == AS_GRN ? 2u : k];
    uint32_t i, c;
    int32_t pk = 0;
    if (s[0].type != ty && !s[0].w)             /* faded out: the new kind from rest */
        s[0].type = s[1].type = (uint8_t)ty;
    if (s[0].type != ty)
        tw = 0;                                 /* (the old one fading out first) */
    if (!s[0].w) {
        if (!tw) {
            alm.lvl[k] = 0;
            return;                             /* dry: bit for bit */
        }
        memset(s, 0, 2u * sizeof *s);
        s[0].type = s[1].type = (uint8_t)ty;
        if (ty != IT_WOBBL)                     /* (the sweeps' right side a quarter on: wide; WOBBL one pitch) */
            s[1].ph = 0x40000000u;
    }
    ty = s[0].type;
    for (i = 0; i < n; i++) {
        alm_l[i] = ml[i] >> 2;
        alm_r[i] = mr[i] >> 2;
    }
    if (ty == IT_DUST) {
        s[0].w = s[1].w = tw;                   /* (pops: no glide to make) */
        alm_dust(alm_l, alm_r, n, tw ? d : 0);
    } else
        for (c = 0; c < 2u; c++) {
            int32_t *b = c ? alm_r : alm_l;
            if (ty == IT_WOW)
                alm_wow(&s[c], b, n, tw, d, alm.wph, alm.fph);
            else if (ty == IT_FILTER)
                ins_filter(&s[c], pp, b, n, tw);
            else if (ty >= IT_PHASER) {
                s[c].type = ty == IT_WOBBL ? IT_CHORUS : (uint8_t)ty;   /* (WOBBL: the chorus, its copy alone below) */
                ins_swept(&s[c], pp, b, n, tw);
                s[c].type = (uint8_t)ty;
                if (ty == IT_WOBBL)             /* out = x + w ((x / 2 + v) - x): x + w (2 v - x) is 2 out - x */
                    for (i = 0; i < n; i++)
                        b[i] = 2 * b[i] - ((c ? mr[i] : ml[i]) >> 2);
            } else
                ins_drive(&s[c], pp, b, n, tw);
        }
    if (ty == IT_WOW) {
        alm.wph += n * WOW_INC;
        alm.fph += n * FLT_INC;
    }
    for (i = 0; i < n; i++) {                   /* back x4, the 2 bits under the quarter: the dry to the bit */
        int32_t yl = (alm_l[i] << 2) + (ml[i] & 3), yr = (alm_r[i] << 2) + (mr[i] & 3), e;
        e = yl - ml[i];
        pk = e > pk ? e : -e > pk ? -e : pk;
        e = yr - mr[i];
        pk = e > pk ? e : -e > pk ? -e : pk;
        ml[i] = yl;
        mr[i] = yr;
    }
    alm.lvl[k] = (uint16_t)clamp(pk >> 3, 0, 65535);
}

/* SPACE: the mix into the delay's send (sd; a stereo MODE: the left there, the right into se) and the reverb's (sr),
 * at BLEND, gliding; a flavour changed: the send down first, then the other bus(es) */
static __attribute__((noinline)) void alm_space(const int32_t *ml, const int32_t *mr, int32_t *sd, int32_t *se, int32_t *sr,
                                                uint32_t n)
{
    uint32_t i, f = alm_f(G_ASPC, 3u);
    int32_t tw = alm_b(G_ASPCB) * 258, pk = 0;
    if (f != alm.sty && !alm.sw)
        alm.sty = (uint8_t)f;
    if (f != alm.sty)
        tw = 0;
    for (i = 0; i < n; i++) {
        int32_t m, x;
        uint32_t g;
        alm.sw += clamp(tw - alm.sw, -SL_SLOPE, SL_SLOPE);
        if (!alm.sw)
            continue;
        g = (uint32_t)alm.sw << 1;
        m = mulq16((ml[i] + mr[i]) >> 1, g);
        if (alm.sty != 1u) {                    /* ECHO, BOTH: the delay */
            if (se) {
                sd[i] += mulq16(ml[i], g);
                se[i] += mulq16(mr[i], g);
            } else
                sd[i] += m;
        }
        if (alm.sty != 0u)                      /* VERB, BOTH: the reverb */
            sr[i] += m;
        x = m < 0 ? -m : m;
        pk = x > pk ? x : pk;
    }
    alm.lvl[AS_SPC] = (uint16_t)clamp(pk >> 3, 0, 65535);
}

/* is any stage on or fading (mix_block: else nothing at all) */
static inline int alm_pre_on(void)
{
    return (song.g[G_ADRVB] | song.g[G_AMOVB] | song.g[G_ASPCB]) || alm.s[0][0].w || alm.s[1][0].w || alm.sw;
}
static inline int alm_post_on(void) { return song.g[G_AGRNB] || alm.s[2][0].w; }

/* before the buses: DRIVE, MOTION on the dry mix, then SPACE's sends of it */
static __attribute__((noinline)) void alm_pre(int32_t *ml, int32_t *mr, int32_t *sd, int32_t *se, int32_t *sr, uint32_t n)
{
    int32_t b = alm_b(G_ADRVB);
    int16_t pp[3];
    pp[0] = (int16_t)b;                         /* DRIVE: 1x .. 12.8x */
    pp[1] = 112;                                /* TONE: ~7.5 kHz */
    pp[2] = 112;                                /* LEVEL: unity */
    alm_stage(AS_DRV, ALM_DRV_T[alm_f(G_ADRV, 4u)], pp, ins_mixq(b * 2), 0, ml, mr, n);
    alm_stage(AS_MOV, ALM_MOV_T[alm_f(G_AMOV, 4u)], ALM_MOV_P[alm_f(G_AMOV, 4u)], ins_mixq(alm_b(G_AMOVB)), 0, ml, mr, n);
    alm_space(ml, mr, sd, se, sr, n);
}

/* after the buses (wet in the mix): GRAIN */
static __attribute__((noinline)) void alm_post(int32_t *ml, int32_t *mr, uint32_t n)
{
    int32_t b = alm_b(G_AGRNB);
    uint32_t f = alm_f(G_AGRN, 4u);
    int16_t pp[3];
    if (f == 2u) {                              /* CRUSH: 16 bits .. 5, 44.1 kHz .. ~2.5 kHz, its low-pass closing */
        pp[0] = (int16_t)(127 - b * 90 / 127);
        pp[1] = (int16_t)(127 - b * 100 / 127);
        pp[2] = (int16_t)(127 - b * 40 / 127);
    } else {                                    /* FILTR: a low-pass from the top down to ~400 Hz, a little resonance */
        pp[0] = (int16_t)(127 - b * 75 / 127);
        pp[1] = 40;
        pp[2] = 0;
    }
    alm_stage(AS_GRN, ALM_GRN_T[f], pp, ins_mixq(f == 2u ? b * 2 : b * 4), b, ml, mr, n);
}

/* mix_block's calls (the audio ISR's: a call each, the tests and the work here, out of its loops): before the buses on
 * the dry mix and its sends; after them, GRAIN on the mix with the buses' wet folded in (the master adds wet: 0 now) */
static __attribute__((noinline)) void alm_pre_blk(uint32_t n)
{
    if (alm_pre_on())
        alm_pre(mix_l, mix_r, send_d, song.g[G_DMODE] != DM_MONO ? send_e : 0, send_r, n);
}
static __attribute__((noinline)) void alm_post_blk(uint32_t n)
{
    uint32_t i;
    if (!alm_post_on())
        return;
    for (i = 0; i < n; i++) {
        mix_l[i] += wet[i];
        mix_r[i] += wet[i];
        wet[i] = 0;
    }
    alm_post(mix_l, mix_r, n);
}
