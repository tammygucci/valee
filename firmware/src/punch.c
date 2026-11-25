/* SPDX-License-Identifier: GPL-3.0-only */
/* ((Vallée, the SPAMPY sound) SPAMPY's PUNCH-IN FX (punch.c of SPAMPY, after SLOOP 2.6), the effects of the FX hold
 * layer (perform.c): hold FX, hold a key (the 16 white ones, F3..G5, and the 11 black ones) and the whole mix goes
 * through that effect while the key is held; let go and the mix comes back. Beat-synced to the tempo; loops start on
 * the grid of the running transport. Every change crossfades. As SPAMPY's, but for:
 *  - the ring: SPAMPY keeps its own 64 KB ring of the mix, always recording. Vallée's pool has no room for it
 *    (the six tracks, the pattern pool, the song), so the ring is the SLICER's recordings borrowed (sl_buf, as the
 *    FX layer's buffer was before): it records while the FX layer is open (FX held or locked) or an effect plays,
 *    and is cleared when it starts. It is the same 0.74 s long, kept at 22.05 kHz (pr_put, pr_get: a pair of samples
 *    as their mean, read between them): its replays are a little darker than SPAMPY's. While borrowed, a STUT track
 *    plays live (slicer.c sl_lent). A loop pressed before the ring holds its division's start waits for the next one
 *  - FREEZE: SPAMPY's holds its reverb (another reverb than Vallée's); here two grains of the ring's last 186 ms,
 *    as the FX layer's FREEZE was. Phase 4 (SPAMPY's mix stage) may bring SPAMPY's back
 *  - the clock: seq.c beat_pos (samples into the beat), fx.c beat_samples; called from perform.c perf_block (in XIP:
 *    AUDIO_FAR) */
#define PUNCH_N 32768u                    /* the ring's samples (44.1 kHz positions; a power of two) */
#define PR_NS (PUNCH_N / 2u)              /* .. stored: a pair a value */
_Static_assert(PR_NS <= NTRK * SL_LEN, "the ring inside sl_buf");
#define PX_FS 44100u
#define PUNCH_NFX 27u                     /* 16 on the white keys, 11 on the black keys */
enum { PX_LOOP4, PX_LOOP8, PX_LOOP16, PX_LOOP32, PX_STUT, PX_REV, PX_STOP, PX_HALF,
       PX_LPF, PX_HPF, PX_TEL, PX_CRUSH, PX_DOWN, PX_GATE, PX_ECHO, PX_WOBBLE,
       /* the black keys, F#3 .. F#5 */
       PX_KLOW, PX_KMID, PX_KHIGH, PX_RWD, PX_OCT, PX_FLANGE, PX_PAN, PX_FREEZE, PX_LOOP8T, PX_MONO, PX_FADE };
static const char *const PUNCH_NAME[PUNCH_NFX] = {
    "LOOP 4", "LOOP 8", "LOOP 16", "LOOP 32", "STUTTER", "REVERSE", "STOP", "HALF",
    "LOW", "HIGH", "PHONE", "CRUSH", "ALIAS", "GATE", "ECHO", "WOBBLE",
    "KILL LOW", "KILL MID", "KILL HIGH", "REWIND", "OCT UP", "FLANGER", "AUTO-PAN", "FREEZE", "LOOP 8T", "MONO",
    "FADE OUT"};
#define PX_OCT_W 2048u                    /* OCT UP: the window of its two read taps (46 ms) */
#define PX_FRZ_CAP 8192u                  /* FREEZE: the samples it catches (186 ms) */
#define PX_GRAIN 2048u                    /* .. its two overlapping grains (46 ms) */
#define PX_RWD_ACC 17                     /* REWIND: speed change per sample (Q16): +1 to -3 in 0.35 s */
#define PX_RWD_MAX (3 << 16)
static struct {
    int8_t req;                   /* the effect asked for (-1 none): perform.c, the last pressed held */
    int8_t cur;                   /* effect playing (fading out when req differs) */
    int32_t g;                    /* wet gain Q15 */
    int32_t top;                  /* .. its most (KNOB 4 DEPTH: perform.c) */
    uint32_t w;                   /* ring write index */
    uint32_t fill;                /* samples recorded since the ring was taken (up to PUNCH_N) */
    int32_t pend;                 /* the even sample of the pair being written */
    uint32_t start, t, len;       /* loop / reverse: start index, samples since, length */
    uint32_t pre;                 /* loops: samples of dry still to go to the next division */
    uint32_t k, sub;              /* position in the loop; HALF: odd / even sample */
    uint32_t dspd;                /* tape stop: speed step per sample */
    uint32_t gp;                  /* gate: samples into the 1/16 step */
    uint32_t rp;                  /* tape stop / rewind: read position Q16 */
    uint32_t spd;                 /* tape stop speed Q16 */
    uint32_t lfo;                 /* wobble, flanger, auto-pan phase */
    int32_t held_l, held_r, hn;   /* downsample */
    int32_t f1l, f2l, f1r, f2r, f3l, f4l;   /* filter states */
    int32_t cut;                  /* sweep, 0..127 << 8 */
    int32_t z[8][2];              /* kills: the SVF states (left 0..3, right 4..7) */
    int32_t v;                    /* REWIND: speed Q16 (signed) */
    uint32_t fv;                  /* FADE OUT: level Q23 */
    uint32_t gpos[2], gs[2];      /* FREEZE: the grains' places, starts */
    uint32_t seed;
} punch = {.req = -1, .cur = -1, .top = 32767, .seed = 1u};

static int px_loop(int32_t fx) { return fx <= PX_STUT || fx == PX_LOOP8T; }   /* on the grid */
static int px_replay(int32_t fx) { return fx <= PX_HALF || fx == PX_LOOP8T || fx == PX_FREEZE; }   /* a span of the ring */

/* the ring, in sl_buf: sample v stored with its pair (v & ~1) as their mean; read back a quarter of a stored step
 * from it (pair (2j, 2j + 1): 2j between j - 1 and j, 2j + 1 between j and j + 1); the pair being written: its
 * first sample as it came */
#define PR_S (&sl_buf[0][0])
static void pr_put(uint32_t v, int32_t x)
{
    x = clamp(x, -32768, 32767);
    if (!(v & 1u))
        punch.pend = x;
    else
        PR_S[(v >> 1) & (PR_NS - 1u)] = (int16_t)((punch.pend + x) >> 1);
}
static int32_t ring_raw(uint32_t v)                 /* the stored 16 bits (mix / 8) */
{
    uint32_t j = v >> 1, jw = punch.w >> 1;
    int32_t a;
    if (j == jw && !(punch.w & 1u))                 /* the pair being written: its first sample */
        return punch.pend;
    a = PR_S[j & (PR_NS - 1u)];
    if (!(v & 1u))
        return (PR_S[(j - 1u) & (PR_NS - 1u)] + 3 * a) >> 2;
    if (j + 1u == jw && !(punch.w & 1u))            /* (the next pair not stored yet) */
        return a;
    return (3 * a + PR_S[(j + 1u) & (PR_NS - 1u)]) >> 2;
}
static int32_t ring_at(uint32_t i) { return ring_raw(i) << 3; }
static int32_t ring_raw_q16(uint32_t p)             /* read at Q16 position, linear */
{
    int32_t a = ring_raw(p >> 16), b = ring_raw((p >> 16) + 1u);
    return a + (((b - a) * (int32_t)((p >> 1) & 0x7FFFu)) >> 15);
}
static int32_t ring_q16(uint32_t p) { return ring_raw_q16(p) << 3; }

static void punch_start(int32_t fx)
{
    uint32_t beat = beat_samples(), len, ph = beat_pos;
    static const uint8_t DIV[8] = {1, 2, 4, 8, 6, 1, 1, 2};   /* loops: beat / DIV */
    punch.cur = (int8_t)fx;
    punch.t = 0;
    len = fx <= PX_HALF ? beat / DIV[fx] : fx == PX_LOOP8T ? beat / 3u : fx == PX_FREEZE ? PX_FRZ_CAP : beat;
    if (len > PUNCH_N - 1024u)
        len = PUNCH_N - 1024u;
    if (len < 64u)
        len = 64u;
    if (fx == PX_REV && len > punch.fill)           /* ((Vallée) as much as the ring holds */
        len = punch.fill < 64u ? 64u : punch.fill;
    punch.len = len;
    punch.pre = 0;
    punch.start = punch.w;
    if (px_loop(fx) && song.playing) {                /* on the grid: */
        uint32_t back = ph % len;
        if (len - back <= 1536u || back > punch.fill) {   /* just before a division ((Vallée) or the ring without */
            punch.pre = len - back;                   /* its start): from the next one */
        } else {                                        /* else from the last division start */
            punch.start = punch.w - back;
            punch.t = back;
        }
    }
    punch.rp = punch.w << 16;
    punch.spd = 65536u;
    punch.dspd = 65536u / len + 1u;
    punch.sub = 0;
    punch.k = punch.t % len;
    punch.lfo = 0;
    punch.hn = 0;
    punch.cut = 127 << 8;
    punch.f1l = punch.f2l = punch.f1r = punch.f2r = punch.f3l = punch.f4l = 0;
    memset(punch.z, 0, sizeof punch.z);
    punch.v = 65536;
    punch.fv = 32767u << 8;
    punch.gpos[0] = 0;
    punch.gpos[1] = PX_GRAIN / 2u;
    punch.gs[0] = punch.gs[1] = punch.start;
    if (fx == PX_PAN && song.playing)                   /* AUTO-PAN: a turn a beat, on the grid */
        punch.lfo = ph * (0xFFFFFFFFu / beat);
    if (fx == PX_FLANGE)                                /* FLANGER: the sweep starts at its shortest delay */
        punch.lfo = 0xC0000000u;
}

/* ((Vallée) FREEZE: a grain of what it caught (start .. start + PX_FRZ_CAP), by its triangle window */
static int32_t px_grain(uint32_t g)
{
    uint32_t p = punch.gpos[g], tri = p < PX_GRAIN / 2u ? p : PX_GRAIN - p;
    int32_t y;
    if (!p) {
        punch.seed = punch.seed * 1664525u + 1013904223u;
        punch.gs[g] = punch.start + (((punch.seed >> 16) * (PX_FRZ_CAP - PX_GRAIN)) >> 16);
    }
    y = ring_raw(punch.gs[g] + p) * (int32_t)tri;
    punch.gpos[g] = p + 1u < PX_GRAIN ? p + 1u : 0u;
    return y;
}

/* the wet sample for the mono ring effects (after the ring got this sample); no divide per
 * sample: k is the position in the loop, counted up and wrapped */
static int32_t punch_ring_fx(int32_t fx)
{
    uint32_t len = punch.len, k = punch.k;
    int32_t y;
    switch (fx) {
    case PX_REV:
        y = ring_at(punch.start - k);
        break;
    case PX_STOP:
        y = ring_q16(punch.rp);
        punch.rp += punch.spd;
        punch.spd = punch.spd > punch.dspd ? punch.spd - punch.dspd : 0u;   /* to a stop in a beat */
        return ((y >> 3) * (int32_t)(punch.spd >> 4)) >> 9;
    case PX_WOBBLE: {
        int32_t d = 700 + (sine_i(punch.lfo) * 180 >> 15);
        punch.lfo += 4u * (0xFFFFFFFFu / PX_FS);        /* 4 Hz */
        return ring_q16((punch.w << 16) - ((uint32_t)d << 16));
    }
    case PX_FREEZE:                                     /* ((Vallée) its first 186 ms live, then the grains */
        if (punch.t < PX_FRZ_CAP)
            return ring_at(punch.w);
        return (px_grain(0) + px_grain(1)) >> 7;        /* (the windows sum to PX_GRAIN / 2: >> 10, x 8) */
    default:                                        /* loops, HALF */
        y = ring_at(punch.start + k);
        break;
    }
    if (fx != PX_HALF || (punch.sub ^= 1u) == 0u)       /* HALF: every other sample */
        punch.k = k + 1u >= len ? 0u : k + 1u;
    /* declick at the seams */
    return k < 64u ? (y * (int32_t)k) >> 6 : len - k < 64u ? (y * (int32_t)(len - k)) >> 6 : y;
}

/* the loops, STUTTER, HALF, REVERSE ((Vallée) and FREEZE) replay ring samples [start, start + len) (REVERSE: up to
 * start, backwards). The ring keeps recording the mix, and after PUNCH_N samples (0.74 s) the writer comes round into
 * that span: it skips those slots once the loop is captured, so a held loop plays for as long as the key is held */
static int punch_owns(int32_t fx, uint32_t w)
{
    uint32_t d = w - punch.start, len = punch.len;
    if (fx < 0 || !px_replay(fx) || fx == PX_STOP || punch.pre)
        return 0;
    if (fx == PX_REV)                               /* ((Vallée) 2 more each side: a read between two stored */
        return d >= 3u && ((d + len + 1u) & (PUNCH_N - 1u)) < len + 4u;   /* values takes a neighbour */
    return d >= len + 2u && ((d + 2u) & (PUNCH_N - 1u)) < len + 4u;
}

/* the kills: the SVF of dsp.c tsvf_lp with its high-pass out too (Butterworth: tsvf_coef reso 40, damping 5799
 * Q12), two of them in a row (Linkwitz-Riley, 24 dB/oct); one loop per kill, for the block */
#define PX_SVF(A1, A2, A3, Z0, Z1, IN, LP, HP) do {                                            \
        int32_t v3_ = (IN) - Z1, v1_ = ((A1) * Z0 + (A2) * v3_) >> 13;                         \
        int32_t v2_ = Z1 + (((A2) * Z0 + (A3) * v3_) >> 13);                                   \
        HP;                                                                                     \
        Z0 = clamp(2 * v1_ - Z0, -150000, 150000);                                              \
        Z1 = clamp(2 * v2_ - Z1, -150000, 150000);                                              \
        LP = v2_;                                                                               \
    } while (0)
#define PX_HPOUT(H, IN) (H = (IN) - ((v1_ * 5799) >> 12) - v2_)
static void px_kill_run(int32_t fx, const tsvf_t *lo, const tsvf_t *hi, const int32_t *in, int32_t *out, uint32_t n,
                        int32_t (*z)[2])
{
    uint32_t i;
    int32_t x, a, b, h, t;
    int32_t a1 = lo->a1, a2 = lo->a2, a3 = lo->a3, b1 = hi->a1, b2 = hi->a2, b3 = hi->a3;
    int32_t z00 = z[0][0], z01 = z[0][1], z10 = z[1][0], z11 = z[1][1];
    int32_t z20 = z[2][0], z21 = z[2][1], z30 = z[3][0], z31 = z[3][1];
    if (fx == PX_KLOW) {                                /* above ~250 Hz */
        for (i = 0; i < n; i++) {
            x = clamp(in[i], -280000, 280000) >> 1;
            PX_SVF(a1, a2, a3, z00, z01, x, t, PX_HPOUT(h, x));
            PX_SVF(a1, a2, a3, z10, z11, h, t, PX_HPOUT(b, h));
            out[i] = b << 1;
        }
    } else if (fx == PX_KHIGH) {                        /* below ~2.5 kHz */
        for (i = 0; i < n; i++) {
            x = clamp(in[i], -280000, 280000) >> 1;
            PX_SVF(b1, b2, b3, z00, z01, x, t, (void)0);
            PX_SVF(b1, b2, b3, z10, z11, t, a, (void)0);
            out[i] = a << 1;
        }
    } else {                                            /* KILL MID: below ~250 Hz + above ~2.5 kHz */
        for (i = 0; i < n; i++) {
            x = clamp(in[i], -280000, 280000) >> 1;
            PX_SVF(a1, a2, a3, z00, z01, x, t, (void)0);
            PX_SVF(a1, a2, a3, z10, z11, t, a, (void)0);
            PX_SVF(b1, b2, b3, z20, z21, x, t, PX_HPOUT(h, x));
            PX_SVF(b1, b2, b3, z30, z31, h, t, PX_HPOUT(b, h));
            out[i] = (a + b) << 1;
        }
    }
    (void)t;
    z[0][0] = z00, z[0][1] = z01, z[1][0] = z10, z[1][1] = z11;
    z[2][0] = z20, z[2][1] = z21, z[3][0] = z30, z[3][1] = z31;
}
#undef PX_HPOUT
#undef PX_SVF

/* l, r: the mix after the master level (perform.c perf_block), n samples, in place. punch.req: the effect wanted */
static inline __attribute__((always_inline)) void punch_process(int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t i;
    int32_t want = punch.req;
    uint32_t beat, step, echo_d, binc, fdec;
    static int32_t kl[CTL], kr[CTL];                    /* (static: the ISR stack is small) */
    tsvf_t c1, c2;
    if (punch.cur < 0 && want < 0) {                   /* idle: only the ring */
        for (i = 0; i < n; i++)
            pr_put(punch.w++, (l[i] + r[i]) >> 4);
        punch.fill = punch.fill + n < PUNCH_N ? punch.fill + n : PUNCH_N;
        return;
    }
    if (punch.cur < 0 && want >= 0)
        punch_start(want);
    beat = beat_samples();                              /* per block: no divide per sample */
    step = beat / 4u ? beat / 4u : 1u;
    echo_d = beat * 3u / 4u;
    if (echo_d > PUNCH_N - 64u) echo_d = PUNCH_N - 64u;
    binc = 0xFFFFFFFFu / beat;                          /* a turn a beat */
    fdec = (32767u << 8) / (2u * beat);                 /* FADE OUT: to nothing in two beats */
    if (punch.cur == PX_GATE)
        punch.gp = (song.playing ? beat_pos : punch.t) % step;
    {   /* filter sweeps: per block */
        int32_t fx = punch.cur;
        if (fx == PX_LPF && punch.g > 0)
            punch.cut = punch.cut > (34 << 8) ? punch.cut - 96 : 34 << 8;   /* closes over ~1 s */
        if (fx == PX_HPF && punch.g > 0)
            punch.cut = punch.cut > (88 << 8) ? punch.cut - 64 : 88 << 8;
        if (fx >= PX_KLOW && fx <= PX_KHIGH) {
            static tsvf_t klo, khi;                   /* the kills: 251 Hz, 2.44 kHz (Butterworth); made once */
            if (!klo.a1) {
                tsvf_coef(&klo, 43 << 8, 40);
                tsvf_coef(&khi, 89 << 8, 40);
            }
            if (n <= CTL) {
                px_kill_run(fx, &klo, &khi, l, kl, n, &punch.z[0]);
                px_kill_run(fx, &klo, &khi, r, kr, n, &punch.z[4]);
            }
        } else {
            tsvf_coef(&c1, fx == PX_LPF ? punch.cut : fx == PX_HPF ? (127 << 8) - punch.cut + (40 << 8) : 96 << 8, 90);
            tsvf_coef(&c2, 58 << 8, 40);              /* PHONE: low cut ~ 500 Hz */
        }
    }
    for (i = 0; i < n; i++) {
        int32_t x = l[i], y = r[i], wl = x, wr = y, m = (x + y) >> 1;
        int32_t fx = punch.cur;
        uint32_t target = want == fx ? (uint32_t)punch.top : 0u;
        if (!punch_owns(fx, punch.w))                   /* (a held loop keeps its material) */
            pr_put(punch.w, m >> 3);
        switch (fx) {
        case PX_LPF:
            wl = tsvf_lp(&c1, x >> 1, &punch.f1l, &punch.f2l) << 1;
            wr = tsvf_lp(&c1, y >> 1, &punch.f1r, &punch.f2r) << 1;
            break;
        case PX_HPF:
            wl = x - (tsvf_lp(&c1, x >> 1, &punch.f1l, &punch.f2l) << 1);
            wr = y - (tsvf_lp(&c1, y >> 1, &punch.f1r, &punch.f2r) << 1);
            break;
        case PX_TEL: {
            int32_t b = tsvf_lp(&c1, m >> 1, &punch.f1l, &punch.f2l);        /* < 2.5 kHz */
            b -= tsvf_lp(&c2, b, &punch.f3l, &punch.f4l);                   /* > 500 Hz */
            wl = wr = softclip(b << 2) >> 1;
            break;
        }
        case PX_CRUSH:
            wl = x >= 0 ? x & ~0x7FF : -((-x) & ~0x7FF);   /* towards zero: no offset */
            wr = y >= 0 ? y & ~0x7FF : -((-y) & ~0x7FF);
            break;
        case PX_DOWN:
            if (--punch.hn <= 0) {
                punch.hn = 8;
                punch.held_l = x;
                punch.held_r = y;
            }
            wl = punch.held_l;
            wr = punch.held_r;
            break;
        case PX_GATE: {
            uint32_t p = punch.gp;
            int32_t gg = p < step / 2u ? 32767 : 0;
            punch.gp = p + 1u >= step ? 0u : p + 1u;
            if (p < 64u) gg = (int32_t)p * 512;
            else if (p >= step / 2u && p < step / 2u + 64u) gg = 32767 - (int32_t)(p - step / 2u) * 512;
            wl = ((x >> 3) * gg) >> 12;
            wr = ((y >> 3) * gg) >> 12;
            break;
        }
        case PX_KLOW:
        case PX_KMID:
        case PX_KHIGH:
            wl = kl[i & (CTL - 1u)];                    /* (px_kill_run, for the block) */
            wr = kr[i & (CTL - 1u)];
            break;
        case PX_RWD: {                                  /* the record pulled back: slows, stops, spins backwards */
            uint32_t back = (uint16_t)(punch.w - (punch.rp >> 16));   /* samples behind the writer */
            int32_t room = (int32_t)(punch.fill < PUNCH_N - 512u ? punch.fill : PUNCH_N - 512u) - (int32_t)back, e;
            if (room <= 0) {                            /* all of it heard: silence until the key is up */
                wl = wr = 0;
                break;
            }
            e = room < 1024 ? room * 32 : 32767;
            wl = wr = (((ring_raw_q16(punch.rp) * e) >> 15) << 3);
            punch.rp += (uint32_t)punch.v;
            if (punch.v > -PX_RWD_MAX)
                punch.v -= PX_RWD_ACC;
            break;
        }
        case PX_OCT: {                                  /* two taps read at twice the speed, half a window apart */
            uint32_t d1 = PX_OCT_W - punch.k, d2 = d1 > PX_OCT_W / 2u ? d1 - PX_OCT_W / 2u : d1 + PX_OCT_W / 2u;
            int32_t g1 = (int32_t)(d1 <= PX_OCT_W / 2u ? d1 : PX_OCT_W - d1) << 5;   /* triangles: g1 + g2 = 1 */
            int32_t g2 = 32768 - g1;
            punch.k = punch.k + 1u >= PX_OCT_W ? 0u : punch.k + 1u;
            wl = wr = (ring_raw(punch.w - d1) * g1 + ring_raw(punch.w - d2) * g2) >> 12;
            break;
        }
        case PX_FLANGE: {                               /* 0.6..6 ms, swept over a bar, left and right a quarter apart */
            uint32_t dl = (26u << 16) + (uint32_t)((sine_i(punch.lfo) + 32768) * 240),
                     dr = (26u << 16) + (uint32_t)((sine_i(punch.lfo + 0x40000000u) + 32768) * 240);
            int32_t el = ring_raw_q16((punch.w << 16) - dl), er = ring_raw_q16((punch.w << 16) - dr);
            punch.lfo += binc >> 2;
            pr_put(punch.w, (m >> 3) + (((el + er) * 9000) >> 15));   /* feedback (the ring is the delay line) */
            wl = ((x + (el << 3)) * 5) >> 3;
            wr = ((y + (er << 3)) * 5) >> 3;
            break;
        }
        case PX_PAN: {                                  /* the mix in mono, left to right and back once a beat */
            uint32_t a = (uint32_t)(((sine_i(punch.lfo) * 28000) >> 15) + 32768) << 14;   /* 0..a quarter turn */
            int32_t gl = (sine_i(a + 0x40000000u) * 46341) >> 15, gr = (sine_i(a) * 46341) >> 15;   /* x sqrt 2 */
            punch.lfo += binc;
            wl = ((m >> 4) * gl) >> 11;                 /* (>> 4: no overflow on a hot mix, gl up to 46341) */
            wr = ((m >> 4) * gr) >> 11;
            break;
        }
        case PX_MONO:
            wl = wr = m;
            break;
        case PX_FADE: {
            int32_t v = (int32_t)(punch.fv >> 8), gq = (v * v) >> 15;   /* squared: the ear hears it even */
            punch.fv = punch.fv > fdec ? punch.fv - fdec : 0u;
            wl = ((x >> 3) * gq) >> 12;
            wr = ((y >> 3) * gq) >> 12;
            break;
        }
        case PX_ECHO: {
            int32_t e = (ring_raw(punch.w - echo_d) * 18000) >> 12;   /* (x 8 after: no overflow on a hot mix) */
            pr_put(punch.w, (m + e) >> 3);              /* feedback */
            wl = x + e;
            wr = y + e;
            break;
        }
        default:
            if (fx < 0)
                break;
            if (punch.pre) {                            /* waiting for the division */
                if (!--punch.pre) {
                    punch.start = punch.w + 1u;
                    punch.k = 0;
                }
                break;
            }
            wl = wr = punch_ring_fx(fx);
            break;
        }
        if ((uint32_t)punch.g < target)
            punch.g = punch.g + 512 > (int32_t)target ? (int32_t)target : punch.g + 512;
        else if ((uint32_t)punch.g > target)
            punch.g = punch.g < 512 + (int32_t)target ? (int32_t)target : punch.g - 512;
        l[i] = x + ((((wl - x) >> 3) * (punch.g >> 3)) >> 9);   /* (no 32-bit overflow) */
        r[i] = y + ((((wr - y) >> 3) * (punch.g >> 3)) >> 9);
        punch.w++;
        punch.t++;
        if (punch.g == 0 && want != fx)                 /* faded out: none until the next block, which starts the
                                                         * next one with its own set-up (kills, filters, gate:
                                                         * made per block); at most 31 samples dry */
            punch.cur = -1;
    }
    punch.fill = punch.fill + n < PUNCH_N ? punch.fill + n : PUNCH_N;
}

/* ((Vallée) the ring taken (cleared: what it held was the SLICER's) or given back */
static void punch_take(void)
{
    memset(PR_S, 0, PR_NS * sizeof(int16_t));
    punch.fill = 0;
    punch.pend = 0;
    punch.cur = -1;
    punch.g = 0;
}

/* a key (0..26, from F3) -> its white key index 0..15, -1 = a black key */
static int32_t punch_key(uint32_t k)
{
    static const int8_t W[12] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6};   /* from F */
    int32_t i = W[k % 12u];
    return i < 0 ? -1 : (int32_t)(k / 12u) * 7 + i;
}
/* the black keys, F#3 .. F#5, and their effects 16..26 */
static const uint8_t PUNCH_BLACK[PUNCH_NFX - 16u] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
/* the keyboard (seq.c keyboard_block) while FX is held: a key -> its effect (-1 none) */
static int32_t punch_fx_of_key(uint32_t k)
{
    int32_t w = punch_key(k);
    uint32_t i;
    if (w >= 0)
        return w < 16 ? w : -1;
    for (i = 0; i < PUNCH_NFX - 16u; i++)
        if (PUNCH_BLACK[i] == k)
            return (int32_t)(16u + i);
    return -1;
}
static uint32_t punch_key_of_fx(uint32_t fx)          /* an effect -> its key (the lit key) */
{
    static const uint8_t K[7] = {0, 2, 4, 6, 7, 9, 11};   /* the white keys of an octave, from F */
    fx %= PUNCH_NFX;
    return fx < 16u ? fx / 7u * 12u + K[fx % 7u] : PUNCH_BLACK[fx - 16u];
}
