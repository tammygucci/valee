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
    uint16_t lvl_md;                            /* MEDIUM's likewise (amd: the pool) */
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

/* ---- (Vallée) MEDIUM (GLO > ALEMBIC 3): the whole mix as heard off an old medium, last (after GRAIN, the wet in):
 *   CASST  a cassette: wow and flutter, its top rolled off (two poles at 9 kHz), soft saturation, hiss, a little
 *          narrower, its level breathing with the wow
 *   TAPE   a reel-to-reel machine: barely a wow, a fast faint flutter, the tape's warmth (more saturation) and its
 *          head bump (+~3 dB round 110 Hz), the top at 16 kHz, faint hiss
 *   VHS    a VHS's linear track: a deep wow drifting (the transport's), a warble, the band 90 Hz .. 7 kHz (two poles),
 *          hiss, nearly mono, dropouts (WEAR)
 *   VINYL  a record: its warp (0.55 Hz, 33 rpm), the top at 13 kHz, crackle and pops, a low rumble
 * BLEND: how much of all that (the wow's depth, the tone, the saturation, the noise, the width, the dropouts: each
 * scaled, never a dry/wet mix, which would comb with the delayed tape). WEAR: the noise, the crackle, the dropouts.
 * WOW: the wow and flutter's depth (64 as the medium is, 127 twice). The tape runs 6 ms behind: switched on, its
 * delay winds up from 0 (0.8 s, a tape starting: a little pitch under), off it winds back down, then nothing runs
 * (the mix bit for bit). A medium changed: BLEND down, then the other up (the delay stays). State in the pool (4 KiB
 * of lines: .bss has none to spare) */
#define MD_LEN 512u                                     /* the tape's line, samples (11.6 ms; a power of 2) */
#define MD_BASE (265u << 16)                            /* the tape's delay, Q16 samples (6 ms) */
typedef struct {
    int32_t line[2][MD_LEN];
    uint32_t wp, base;                                  /* the next write; the delay now (Q16, to MD_BASE or 0) */
    uint32_t wph, fph, seed;                            /* the wow's and flutter's phases; the noise */
    int32_t a;                                          /* the medium's share now, Q15 (BLEND, gliding) */
    uint8_t type, on;                                   /* the medium running; the stage running */
    int32_t lp1[2], lp2[2], hp[2], bump[2], rum, ck;    /* the tone's filters; the rumble; a pop dying */
    int32_t drift, dto, drop, dtgt;                     /* VHS: its drift (Q16 samples) and target; the dropout's gain */
    uint32_t dcnt, tcnt;                                /* .. samples left of a dropout; to the drift's next target */
    uint16_t lvl;                                       /* the last block's peak of what it changed (the screen) */
} amd_t;
static amd_t amd __attribute__((section(".pool")));
typedef struct {
    uint32_t wow, flut;                                 /* the rates (phase per sample) */
    int32_t wd, fd;                                     /* their depths at WOW 64, Q16 samples */
    uint16_t lp, hp;                                    /* one-pole coefficients, Q16 */
    uint8_t poles, drv;                                 /* low-pass poles (1, 2); saturation's drive, 16ths */
    int16_t hiss, width;                                /* hiss at WEAR 127 (Q15 scale); the width at full, Q15 */
} md_t;
#define MD_HZ(f) ((uint32_t)((f) / 44100.0 * 4294967296.0))
#define MD_MS(m) ((int32_t)((m) * 44.1 * 65536.0))
static const md_t MD[4] = {
    {MD_HZ(0.7), MD_HZ(8.0), MD_MS(0.35), MD_MS(0.03), 47356, 419, 2, 20, 110, 27000},   /* CASST */
    {MD_HZ(0.25), MD_HZ(14.0), MD_MS(0.12), MD_MS(0.02), 58830, 233, 1, 26, 34, 32767},  /* TAPE */
    {MD_HZ(0.45), MD_HZ(3.2), MD_MS(0.6), MD_MS(0.06), 41362, 835, 2, 18, 140, 8000},    /* VHS */
    {MD_HZ(0.55), 0, MD_MS(0.28), 0, 55254, 205, 1, 16, 22, 30000},                       /* VINYL */
};
static inline int32_t md_noise(void)
{
    amd.seed = amd.seed * 1664525u + 1013904223u;
    return (int32_t)amd.seed >> 16;                     /* -32768 .. 32767 */
}
static __attribute__((noinline)) void alm_medium(int32_t *ml, int32_t *mr, uint32_t n)
{
    uint32_t f = alm_f(G_AMED, 4u), i, c, kin, kout, kbr, krum, pdrop, ppop;
    int32_t ta = alm_b(G_AMEDB) * 258, wear = alm_b(G_AWEAR), wow = alm_b(G_AWOW), pk = 0, wq, fq, hl, dq;
    const md_t *m;
    if (!amd.on) {
        if (!ta) {
            alm.lvl_md = 0;
            return;                                     /* dry: bit for bit */
        }
        memset(&amd, 0, sizeof amd);
        amd.on = 1;
        amd.type = (uint8_t)f;
        amd.seed = 0x7A9Eu;
        amd.drop = amd.dtgt = 32767;
    }
    if (f != amd.type && !amd.a) {                      /* faded out: the other medium from rest */
        amd.type = (uint8_t)f;
        memset(amd.lp1, 0, sizeof amd.lp1); memset(amd.lp2, 0, sizeof amd.lp2);
        memset(amd.hp, 0, sizeof amd.hp); memset(amd.bump, 0, sizeof amd.bump);
        amd.rum = amd.ck = amd.drift = amd.dto = 0;
        amd.drop = amd.dtgt = 32767;
        amd.dcnt = 0;
    }
    if (f != amd.type)
        ta = 0;                                         /* (the one running fades out first) */
    m = &MD[amd.type];
    /* per block (no divide per sample: the FM-1's core): the depths x WOW / 64 (Q8 samples), the hiss x WEAR, the
     * saturation's gains (the mix at a fifth into the tanh: a full mix peaks at ~4 x a voice, ~100 000, and only
     * its peaks are warmed, never clipped), the cassette's breath, the rumble, the odds of a pop and of a dropout */
    wq = ((m->wd >> 8) * wow) >> 6;
    fq = ((m->fd >> 8) * wow) >> 6;
    dq = (int32_t)((MD_MS(0.25) >> 8) * wow) >> 6;      /* VHS's drift, Q8 samples */
    hl = m->hiss * wear / 127;
    kin = (uint32_t)m->drv * 819u;                      /* x drv / 16 / 5, Q16 */
    kout = (uint32_t)(327680u / m->drv);                /* x 5 x 16 / drv, Q12 (mulq16 takes k up to 1.0 only) */
    kbr = (uint32_t)wear * 52u;                         /* the breath: ~0.4 dB at WEAR 127 */
    krum = (uint32_t)wear << 7;                         /* the rumble: ~-45 dBFS at WEAR 127 */
    pdrop = (uint32_t)wear >> 6;                        /* ~0.3 a second at WEAR 127, none under 64 .. */
    ppop = (uint32_t)wear / 4u + 1u;                    /* ~0..19 pops a second */
    for (i = 0; i < n; i++) {
        int32_t a, d, ns[2], pop = 0, x[2], y[2], g;
        uint32_t q, ka;
        amd.a += clamp(ta - amd.a, -SL_SLOPE, SL_SLOPE);
        a = amd.a;
        ka = (uint32_t)a << 1;                          /* the share, Q16 */
        if (ta)                                         /* the tape winding up (on) or down (off, BLEND faded) */
            amd.base = amd.base + 512u < MD_BASE ? amd.base + 512u : MD_BASE;
        else if (!a)
            amd.base = amd.base > 512u ? amd.base - 512u : 0u;
        /* the read: the delay, the wow and the flutter, VHS's drift (all x the share) */
        d = (int32_t)amd.base;
        if (a) {
            int32_t w = ((osc_sine(amd.wph) * wq) >> 7) + ((osc_sine(amd.fph) * fq) >> 7);   /* Q16 samples */
            if (amd.type == 2u) {                       /* VHS: the transport drifting, a new place every ~0.5 s */
                if (!amd.tcnt--) {
                    amd.tcnt = 16000u + ((uint32_t)md_noise() & 8191u);
                    amd.dto = (md_noise() * dq) >> 7;
                }
                amd.drift += (amd.dto - amd.drift) >> 14;
                w += amd.drift;
            }
            d += mulq16(w, ka);
            if (d < 0)
                d = 0;
        }
        amd.wph += m->wow;
        amd.fph += m->flut;
        /* the noise: hiss (each side its own), VINYL's crackle and rumble; x the share */
        ns[0] = ns[1] = 0;
        if (a && hl) {
            ns[0] = mulq16((md_noise() * hl) >> 15, ka);
            ns[1] = mulq16((md_noise() * hl) >> 15, ka);
        }
        if (amd.type == 3u && a) {
            if (wear && ((uint32_t)md_noise() & 0xFFFFu) < ppop)
                amd.ck = (md_noise() * (40 + wear * 2)) >> 11;   /* (up to ~-17 dBFS) */
            pop = mulq16(amd.ck, ka);
            amd.ck = (amd.ck * 5) >> 3;
            amd.rum += mulq16(md_noise() - amd.rum, 186u);   /* (20 Hz) */
            pop += mulq16(mulq16(amd.rum, krum), ka);
        }
        /* VHS: a dropout now and then (WEAR past half): the level dips to ~60 % for 30 .. 110 ms, eased */
        if (amd.type == 2u && a && pdrop) {
            if (!amd.dcnt) {
                if (((uint32_t)md_noise() & 0x3FFFFu) < pdrop) {
                    amd.dcnt = 1300u + ((uint32_t)md_noise() & 3071u);
                    amd.dtgt = 20000;
                } else
                    amd.dtgt = 32767;
            } else if (!--amd.dcnt)
                amd.dtgt = 32767;
        } else
            amd.dtgt = 32767;
        amd.drop += clamp(amd.dtgt - amd.drop, -40, 40);
        for (c = 0; c < 2u; c++) {
            int32_t v = (c ? mr[i] : ml[i]) + ns[c] + pop, s, r0, r1, fr, z, t;
            /* the saturation: the mix at a quarter, x drive, through the tanh, back; the share of it */
            s = (softclip(clamp(mulq16(v, kin), -2000000, 2000000)) * (int32_t)kout) >> 12;
            v += mulq16(s - v, ka);
            if (amd.type == 1u) {                       /* TAPE: the head bump (+~3 dB round 110 Hz) */
                amd.bump[c] += mulq16(v - amd.bump[c], 1019u);
                v += mulq16(mulq16(amd.bump[c], 26214u), ka);
            }
            amd.line[c][amd.wp] = v;
            q = (uint32_t)d;
            r0 = (int32_t)(amd.wp - (q >> 16)) & (int32_t)(MD_LEN - 1u);
            r1 = (r0 - 1) & (int32_t)(MD_LEN - 1u);
            fr = (int32_t)((q >> 2) & 0x3FFFu);         /* Q14 */
            z = amd.line[c][r0] + ((((amd.line[c][r1] - amd.line[c][r0]) >> 2) * fr) >> 12);
            /* the tone: the low-pass (1 or 2 poles), the low cut; the share of it */
            amd.lp1[c] += mulq16(z - amd.lp1[c], m->lp);
            t = amd.lp1[c];
            if (m->poles == 2u) {
                amd.lp2[c] += mulq16(t - amd.lp2[c], m->lp);
                t = amd.lp2[c];
            }
            amd.hp[c] += mulq16(t - amd.hp[c], m->hp);
            t -= amd.hp[c];
            z += mulq16(t - z, ka);
            if (amd.type == 0u && kbr)                  /* CASST: the level breathing with the wow */
                z -= mulq16(mulq16(z, (uint32_t)(osc_sine(amd.wph) + 32768) * kbr >> 16), ka);
            y[c] = mulq16(z, (uint32_t)amd.drop << 1);
            x[c] = c ? mr[i] : ml[i];
        }
        amd.wp = (amd.wp + 1u) & (MD_LEN - 1u);
        /* the width: the side x (1 - (1 - width) x the share) */
        g = 32768 - mulq16(32768 - m->width, ka);
        {
            int32_t mid = (y[0] + y[1]) >> 1, side = mulq16((y[0] - y[1]) >> 1, (uint32_t)g << 1);
            ml[i] = mid + side;
            mr[i] = mid - side;
        }
        for (c = 0; c < 2u; c++) {
            int32_t e = (c ? mr[i] : ml[i]) - x[c];
            e = e < 0 ? -e : e;
            pk = e > pk ? e : pk;
        }
    }
    alm.lvl_md = (uint16_t)clamp(pk >> 3, 0, 65535);
    if (!ta && !amd.a && !amd.base)
        amd.on = 0;                                     /* wound down: nothing runs from the next block */
}

/* is any stage on or fading (mix_block: else nothing at all) */
static inline int alm_pre_on(void)
{
    return (song.g[G_ADRVB] | song.g[G_AMOVB] | song.g[G_ASPCB]) || alm.s[0][0].w || alm.s[1][0].w || alm.sw;
}
static inline int alm_post_on(void) { return song.g[G_AGRNB] || alm.s[2][0].w || song.g[G_AMEDB] || amd.on; }

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
    alm_medium(ml, mr, n);                              /* ((Vallée) MEDIUM: last, the mix off its tape or record */
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
