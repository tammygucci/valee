/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* PERFORM: the effects of the FX hold layer, on the master. A key pressed while FX is held belongs to the
 * layer (seq.c keyboard_block): it plays no note, sends no MIDI, records nothing, and holds its effect until
 * it is let go (ui_input.c opens the layer and shows the map). ((Vallée, the SPAMPY sound) The effects are SPAMPY's
 * PUNCH-IN FX (punch.c), on all 27 keys: the 16 white keys from F3 (LOOP 4 8 16 32, STUTTER, REVERSE, STOP, HALF, LOW,
 * HIGH, PHONE, CRUSH, ALIAS, GATE, ECHO, WOBBLE) and the 11 black keys (KILL LOW MID HIGH, REWIND, OCT UP, FLANGER,
 * AUTO-PAN, FREEZE, LOOP 8T, MONO, FADE OUT). One plays at a time: the last pressed of those held; letting it go
 * returns to the one held before. The tracks' cuts are GLO's black keys (ui_layer.c mute_keys: perf_cut, perf_solo).
 * KNOB 1..4 with FX: the macros FILTER (LPF left, HPF right), CRUSH, THROW (the dry mix into the delay and reverb
 * sends), DEPTH (the effect's level: down from full).
 * MENU > FX LATCH ON (#40, a hand that cannot hold FX, a key and a knob together): a key pressed with FX turns its
 * effect on, the next press turns it off (perf_latched; letting the key or FX go does nothing), the macros keep
 * their values when FX is let go, FX + OCT- turns everything off (ui_layer.c), so does the menu or a dialog.
 * Chain: the effect (punch.c) -> LPF -> HPF -> CRUSH, after the master level and before master_block (the limiter);
 * THROW and the mutes act before the buses (fx.c mix_block / mix_part).
 * Idle (no key, no knob, FX not held, no ramp left) every stage is skipped: the output is bit-identical. */
static volatile uint32_t beat_pos, beat_n;     /* (seq.c's: samples into the beat, the beat of the bar) */
#include "punch.c"                             /* ((Vallée) SPAMPY's PUNCH-IN FX */
enum { PF_M1 = PUNCH_NFX, PF_N = PUNCH_NFX };   /* the effects 0..26 (punch.c PX_*); PF_N: none */
#define PF_BIT(e) (1u << (e))
/* ((Vallée) THE SPELL's keys (MENU > FX: G5 KEY, F5 .. F3 KEY, menu_items.c TRANS_N; a nibble each, 1..14): 14 of
 * the 27, where the FX layer's old ones were (REPEAT 1/8 1/16 1/32 -> LOOP 8 16 32, LPF -> LOW, TAPE STOP -> STOP, OCT
 * DN -> HALF, DUB -> ECHO, VINYL -> WOBBLE) */
static const uint8_t SPELL_FX[14] = {PX_LOOP8, PX_LOOP16, PX_LOOP32, PX_REV, PX_LPF, PX_HPF, PX_STOP, PX_FREEZE,
                                     PX_OCT, PX_HALF, PX_GATE, PX_ECHO, PX_FLANGE, PX_WOBBLE};
#define PF_ALL ((1u << PF_M1) - 1u)
#define PF_TOP (63 << 8)                      /* filter cutoff index, Q8 (PF_SVF): the LPF's open end */
#define PF_LOW (14 << 8)                      /* .. the LPF sweep's end, the HPF sweep's (and K1's) top */
static volatile uint32_t perf_mask;   /* main: the FX button's bit while its layer may own keys, 0 = none */
static volatile uint32_t kb_mask;     /* main: the button bits whose hold makes keys a layer's (ui_layer.c), 0 = none */
static volatile uint8_t kb_lock;      /* main: a layer locked open (#83, no button held): 1 keys are the layer's, 2 FX's */
static volatile uint8_t perf_solo;    /* main: tracks soloed in the GLO layer (the others muted as by a black key) */
static volatile uint8_t perf_cut __attribute__((section(".pool")));   /* main: ((2.0, D 1A) ui_layer.c mute_keys) the
                                       * tracks cut by a black key held in GLO or PERFORM (FX's: perf_held, here) */
static volatile uint8_t perf_kill;    /* main: every effect off (the menu, a dialog, UBOOT) */
static volatile int16_t perf_k[4];    /* main: the knob macros, 0 = untouched: FILTER -100..100 (- LPF, + HPF),
                                       * CRUSH 0..100, THROW 0..100, DEPTH cut 0..100 (the buffer effects' level) */
static volatile uint32_t kb_layer;    /* ISR: the keys held that are the layer's, a bit per key */
static volatile uint32_t perf_held;   /* ISR: the effects held (PF_*) */
static volatile uint8_t perf_latch_on;   /* main: MENU > FX LATCH ON (#40): a key with FX toggles its effect */
static volatile uint32_t perf_latched;   /* ISR: the effects latched (FX LATCH; main clears it: OCT-, the menu) */
/* ((Vallée) THE SPELL's transition key, its last white key (G5): held, the effect MENU > CONTROL > TRANSITION FX
 * names plays (a PF_* of the white keys'), as the FX layer's keys do; let go, it ends (ui_world.c wl_poll: main) */
static volatile uint8_t perf_auto_on, perf_auto_fx;
static volatile uint32_t perf_act;    /* ISR: .. running (a 1/16 one from its start on) */
static uint32_t perf_ord[PF_N], perf_seq;   /* press order: the last pressed buffer effect plays */

/* the ZDF state-variable filter at cutoff index i (30 Hz * 2^(i / 7), 0..63: up to 15.4 kHz), k = 1:
 * a1 a2 a3 in Q14 (tools: g = tan(pi fc / fs), a1 = 1 / (1 + g (g + k)), a2 = g a1, a3 = g a2) */
static const int16_t PF_SVF[64][3] = {
    {16349, 35, 0}, {16345, 39, 0}, {16341, 43, 0}, {16337, 47, 0}, {16332, 52, 0}, {16327, 57, 0}, {16321, 63, 0},
    {16314, 70, 0}, {16307, 77, 0}, {16299, 85, 0}, {16290, 94, 1}, {16280, 103, 1}, {16269, 114, 1},
    {16257, 126, 1}, {16244, 139, 1}, {16229, 153, 1}, {16213, 169, 2}, {16196, 186, 2}, {16176, 205, 3},
    {16154, 227, 3}, {16130, 250, 4}, {16104, 275, 5}, {16075, 303, 6}, {16043, 334, 7}, {16007, 368, 8},
    {15968, 406, 10}, {15925, 447, 13}, {15877, 492, 15}, {15824, 541, 19}, {15766, 596, 22}, {15702, 655, 27},
    {15631, 720, 33}, {15553, 791, 40}, {15467, 869, 49}, {15372, 953, 59}, {15267, 1046, 72}, {15151, 1146, 87},
    {15024, 1255, 105}, {14883, 1374, 127}, {14729, 1502, 153}, {14559, 1640, 185}, {14372, 1790, 223},
    {14166, 1950, 268}, {13940, 2121, 323}, {13692, 2304, 388}, {13420, 2499, 465}, {13122, 2704, 557},
    {12797, 2921, 667}, {12441, 3147, 796}, {12053, 3382, 949}, {11631, 3624, 1129}, {11173, 3870, 1341},
    {10677, 4119, 1589}, {10140, 4365, 1879}, {9562, 4605, 2218}, {8940, 4832, 2612}, {8274, 5040, 3070},
    {7563, 5219, 3602}, {6807, 5359, 4219}, {6006, 5444, 4934}, {5163, 5456, 5766}, {4282, 5369, 6733},
    {3372, 5149, 7863}, {2449, 4745, 9190},
};

static struct {
    uint32_t act;                      /* the effects held (latched, THE SPELL's) */
    uint8_t busy, armed, mute;         /* armed: FX held or locked (the ring records); mute: tracks cut */
    int32_t lc, hc, la, ha;            /* the macro's LPF and HPF: cutoffs (Q8 index) and shares (Q15) */
    int32_t lz[4], hz[4];              /* their states: L ic1 ic2, R ic1 ic2 */
    int16_t lk[3], hk[3];              /* their coefficients this block */
    int32_t cw, cc, chl, chr;          /* CRUSH share, amount, held sample */
    uint32_t cn;
    int32_t td;                        /* THROW: the share of the dry mix sent, Q15 */
    int32_t mg[NTRK];                  /* mute gains, Q15 (32768 = open) */
} pf = {.lc = PF_TOP, .mg = {[0 ... NTRK - 1] = 32768}};

/* every effect can run at any tempo ((Vallée) punch.c: a loop longer than the ring plays its first 0.72 s) */
static uint32_t perf_avail(void) { return ~0u; }

#ifndef PERF_KEYS_LATCH
#define PERF_KEYS_LATCH 0
#endif
/* seq.c keyboard_block: a layer key down / up (e: PF_*, PF_N = no effect) */
static void perf_press(uint32_t e, int down)
{
    if (e >= PF_N)
        return;
    if (PERF_KEYS_LATCH && perf_latch_on && e < PF_M1) {   /* ((Vallée) off: a locked FX layer's keys play as held, an
                                           * effect ends when its key is let go; its knobs' macros still stay) FX LATCH: a press turns it on or off, letting go does nothing ((2.0, D 1A)
                                           * not a black key: its tap is the track's MUTE, held a cut, as unlatched) */
        if (down) {
            perf_latched ^= PF_BIT(e);
            perf_ord[e] = ++perf_seq;
        } else if (perf_held & PF_BIT(e)) {   /* ((2.0.2, #57) held as the layer locked: let go, it stays on, latched, */
            perf_latched |= PF_BIT(e);        /* so its next press or the layer closing turns it off (it stayed held */
            perf_held &= ~PF_BIT(e);          /* for good before, on every page) */
        }
        return;
    }
    if (down) {
        perf_held |= PF_BIT(e);
        perf_ord[e] = ++perf_seq;
    } else {
        perf_held &= ~PF_BIT(e);
    }
}

static void perf_start(void) { }                 /* seq_start ((Vallée) punch.c follows seq.c beat_pos) */

/* the SLICER's recordings dropped (when the buffer is taken and given back) */
static void perf_drop_slicer(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        sl[k].rec = sl[k].loop = sl[k].rec_on = 0;
}

/* the effect to play: the last pressed of those held (PF_N none) */
static uint32_t perf_pick(uint32_t a)
{
    uint32_t e, best = PF_N;
    a &= PF_ALL;
    for (e = 0; e < PF_M1; e++)
        if (((a >> e) & 1u) && (best == PF_N || perf_ord[e] - perf_ord[best] < 0x80000000u))
            best = e;
    return best;
}

/* each block, after the keys (seq.c) and before the parts: what runs, the macro's filters, the mutes.
 * Returns 1 when a stage has something to do */
static __attribute__((noinline)) AUDIO_RAM int perf_begin(uint32_t n)
{
    uint32_t aut = perf_auto_on ? PF_BIT(SPELL_FX[perf_auto_fx % 14u]) : 0u;   /* ((Vallée) THE SPELL's keys */
    uint32_t held = perf_kill ? 0u : (perf_held | perf_latched | aut) & PF_ALL;
    uint32_t mute = perf_kill ? 0u : (uint32_t)perf_cut | (perf_solo ? ~(uint32_t)perf_solo & ((1u << NTRK) - 1u) : 0u);
    uint32_t armed = !perf_kill && (held || (fm1_in.buttons & perf_mask) || (kb_lock & 2u));   /* (FX held or locked) */
    uint32_t k;
    int32_t m;
    (void)n;
    if (!held && !mute && !armed && !pf.busy && !(perf_k[0] | perf_k[1] | perf_k[2])) {   /* idle */
        perf_act = 0;
        return 0;
    }
    pf.act = held;
    perf_act = held;
    pf.armed = (uint8_t)armed;
    /* the filters: KNOB 1 left the LPF, right the HPF */
    m = perf_k[0];
    {
        int32_t lt = PF_TOP, ht = 0;
        if (m < 0)
            lt = PF_TOP + m * (PF_TOP - PF_LOW) / 100;
        if (m > 0)
            ht = m * (PF_TOP - PF_LOW) / 100;
        pf.lc += clamp(lt - pf.lc, -1536, 1536);    /* ((Vallée) ~6 ms over the range: no click, no fade */
        pf.hc += clamp(ht - pf.hc, -1536, 1536);
        for (k = 0; k < 3u; k++) {                  /* the coefficients, between two table rows */
            int32_t i = pf.lc >> 8, f = pf.lc & 255, j = pf.hc >> 8, g = pf.hc & 255;
            pf.lk[k] = (int16_t)(PF_SVF[i][k] + (((PF_SVF[i < 63 ? i + 1 : 63][k] - PF_SVF[i][k]) * f) >> 8));
            pf.hk[k] = (int16_t)(PF_SVF[j][k] + (((PF_SVF[j < 63 ? j + 1 : 63][k] - PF_SVF[j][k]) * g) >> 8));
        }
    }
    pf.mute = (uint8_t)mute;                        /* ((2.0) the cuts held; a MUTE and a ramp back: fx.c mix_part) */
    pf.busy = 1;
    return 1;
}

/* a track muted: its signal ramps to 0 and back (fx.c mix_part, after the SLICER). (2.0, D 1A) Its MUTE (latched) and a
 * black key held (FX's, GLO's, PERFORM's; GLO's solo) alike: muted is silent now (a MUTE alone refused new notes and
 * let the sounding ones ring out) */
static __attribute__((noinline)) AUDIO_RAM void perf_mute(uint32_t k, int32_t *b, uint32_t n)
{
    uint32_t i;
    int32_t g = pf.mg[k], t = ((pf.mute >> k) & 1u) || trk[k].p[P_MUTE] ? 0 : 32768;
    for (i = 0; i < n; i++) {
        g += clamp(t - g, -SL_SLOPE, SL_SLOPE);
        b[i] = mulq16(b[i], (uint32_t)g << 1);
    }
    pf.mg[k] = g;
}

/* the delay's feedback and return (fx.c), Q15: the song's ((Vallée) DUB went with the old effects) */
static void perf_dub_dly(int32_t *fb, int32_t *mix) { (void)fb; (void)mix; }

/* (2.5) THROW and DUB into a stereo delay (a stereo MODE, fx.c; se its right send): as perf_pre and perf_dub, the left mix
 * into sd and the right into se (the reverb's send their mean, as before). In XIP, its own loops: perf_pre's and
 * perf_dub's stay as they were (in RAM) */
static __attribute__((noinline)) void perf_pre_st(const int32_t *ml, const int32_t *mr, int32_t *sd, int32_t *se,
                                                  int32_t *sr, uint32_t n)
{
    uint32_t i;
    int32_t m = perf_k[2] * 327;
    for (i = 0; i < n; i++) {
        pf.td += clamp(m - pf.td, -SL_SLOPE, SL_SLOPE);
        if (pf.td) {
            sd[i] += mulq16(ml[i], (uint32_t)pf.td << 1);
            se[i] += mulq16(mr[i], (uint32_t)pf.td << 1);
            sr[i] += mulq16((ml[i] + mr[i]) >> 1, (uint32_t)pf.td << 1);
        }
    }
}

/* before the buses: THROW (KNOB 3) adds the dry mix to the delay and reverb sends ((2.5) se: a stereo delay's right
 * send, perf_pre_st; 0: MONO) */
static __attribute__((noinline)) AUDIO_RAM void perf_pre(const int32_t *ml, const int32_t *mr, int32_t *sd, int32_t *se,
                                               int32_t *sr, uint32_t n)
{
    uint32_t i;
    int32_t m = perf_k[2] * 327;
    if (se) {
        AUDIO_FAR(perf_pre_st)(ml, mr, sd, se, sr, n);
        return;
    }
    for (i = 0; i < n; i++) {
        pf.td += clamp(m - pf.td, -SL_SLOPE, SL_SLOPE);
        if (pf.td) {
            int32_t x = mulq16((ml[i] + mr[i]) >> 1, (uint32_t)pf.td << 1);
            sd[i] += x;
            sr[i] += x;
        }
    }
}

/* x towards y by w (Q15, 32768 = y) */
static inline int32_t pf_mix(int32_t x, int32_t y, int32_t w)
{
    return w == 32768 ? y : x + mulq16(y - x, (uint32_t)w << 1);
}

/* one channel of a state-variable filter (k = 1): x in Q15 >> 2 */
static inline int32_t pf_svf(int32_t x, int32_t *z, const int16_t *c, int hp)
{
    int32_t v3 = x - z[1], v1 = (c[0] * z[0] + c[1] * v3) >> 14, v2 = z[1] + ((c[1] * z[0] + c[2] * v3) >> 14);
    z[0] = 2 * v1 - z[0];
    z[1] = 2 * v2 - z[1];
    return hp ? x - v1 - v2 : v2;
}

/* the UI: (was OCT UP / DN's shimmer on KNOB 4) none now */
static int perf_harm_on(void) { return 0; }

/* (in XIP, AUDIO_FAR) the effect: the ring taken while armed or playing (cleared), given back after */
static __attribute__((noinline)) void perf_punch(int32_t *bl, int32_t *br, uint32_t n)
{
    uint32_t e = perf_pick(pf.act);
    if (pf.armed || e != PF_N || punch.cur >= 0) {
        if (!sl_lent) {
            sl_lent = 1;
            perf_drop_slicer();
            punch_take();
        }
        punch.req = e == PF_N ? -1 : (int8_t)e;
        punch.top = 32767 - perf_k[3] * 327;
        punch_process(bl, br, n);
    } else if (sl_lent) {
        sl_lent = 0;
        perf_drop_slicer();
        punch.req = punch.cur = -1;
    }
}

/* after the master level: the effect (punch.c), LPF, HPF, CRUSH on l / r (stereo, Q15) */
static __attribute__((noinline)) AUDIO_RAM void perf_block(int32_t *bl, int32_t *br, uint32_t n)
{
    uint32_t i, sh, hold;
    int32_t c = perf_k[1], lt, ht, ct;
    if (c)
        pf.cc = c;
    ct = c ? 32768 : 0;
    sh = 4u + (uint32_t)pf.cc * 7u / 100u;
    hold = sh - 3u;
    if (pf.armed || pf.act || punch.cur >= 0 || sl_lent)
        AUDIO_FAR(perf_punch)(bl, br, n);
    for (i = 0; i < n; i++) {
        int32_t l = bl[i], r = br[i];
        lt = perf_k[0] < 0 || pf.lc < PF_TOP ? 32768 : 0;
        pf.la += clamp(lt - pf.la, -SL_SLOPE, SL_SLOPE);
        if (pf.la) {
            int32_t fl = pf_svf(clamp(l >> 2, -32767, 32767), &pf.lz[0], pf.lk, 0) << 2;
            int32_t fr = pf_svf(clamp(r >> 2, -32767, 32767), &pf.lz[2], pf.lk, 0) << 2;
            l = pf_mix(l, fl, pf.la);
            r = pf_mix(r, fr, pf.la);
        } else {
            pf.lz[0] = pf.lz[1] = pf.lz[2] = pf.lz[3] = 0;
        }
        ht = perf_k[0] > 0 || pf.hc > 0 ? 32768 : 0;
        pf.ha += clamp(ht - pf.ha, -SL_SLOPE, SL_SLOPE);
        if (pf.ha) {
            int32_t fl = pf_svf(clamp(l >> 2, -32767, 32767), &pf.hz[0], pf.hk, 1) << 2;
            int32_t fr = pf_svf(clamp(r >> 2, -32767, 32767), &pf.hz[2], pf.hk, 1) << 2;
            l = pf_mix(l, fl, pf.ha);
            r = pf_mix(r, fr, pf.ha);
        } else {
            pf.hz[0] = pf.hz[1] = pf.hz[2] = pf.hz[3] = 0;
        }
        {   /* CRUSH: fewer bits, a held sample (KNOB 2) */
            pf.cw += clamp(ct - pf.cw, -SL_SLOPE, SL_SLOPE);
            if (pf.cw) {
                if (++pf.cn >= hold) {
                    pf.cn = 0;
                    pf.chl = ((l >> sh) << sh) + (1 << (sh - 1u));
                    pf.chr = ((r >> sh) << sh) + (1 << (sh - 1u));
                }
                l = pf_mix(l, pf.chl, pf.cw);
                r = pf_mix(r, pf.chr, pf.cw);
            }
        }
        bl[i] = l;
        br[i] = r;
    }
    pf.busy = pf.act || pf.armed || punch.cur >= 0 || sl_lent || pf.la || pf.ha || pf.cw || pf.td || pf.mute ||
              perf_k[0] || perf_k[1] || perf_k[2];
}
