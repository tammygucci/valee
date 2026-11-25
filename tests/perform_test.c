/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Host test of the FX hold layer (firmware/src/perform.c) with ((Vallée, the SPAMPY sound) SPAMPY's PUNCH-IN FX
 * (firmware/src/punch.c; after SPAMPY's tests/punch_test.c), same sources as the firmware (through hostsim.c), through
 * the layer's own path (perf_press, perf_begin, perf_block).   build/host/perform_test [DEMO.wav]
 * 1. each of the 27 effects: with FX held (the ring recording) and its key held it changes the mix, stays bounded,
 *    and once the key and FX are up the mix is exactly the dry one again; the ring (sl_buf) given back.
 * 2. idle: nothing held, no knob: the mix bit-identical, no stage busy.
 * 3. the keys: every one of the 27 keys from F3 its own effect (perf_key), the white 16 then the black 11.
 * 4. the effects do what their names say: the kills, OCT UP, MONO, AUTO-PAN, FADE OUT, REWIND, LOOP 8T (the loop
 *    exact), FREEZE (rings on after the sound stops), LOOP 16 on the grid, the last pressed of two plays.
 * 5. the SLICER: while the ring is taken its recordings are dropped, STUT plays live (sl_lent).
 * 6. the macros: CRUSH, FILTER (KNOB 1 left: the highs down), DEPTH (KNOB 4: the effect's level down).
 * Demo (WAV): a beat, then each effect for a beat. */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int check(const char *what, int ok)
{
    printf("perform: %-76s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

#define NS (FS * 8u)
static int32_t il[NS], ir[NS], ol[NS], or_[NS];
typedef int32_t (*gen_f)(uint32_t t, int ch);
static double tone_hz;
static int32_t g_sine(uint32_t t, int ch) { (void)ch; return (int32_t)(100000.0 * sin(2 * M_PI * tone_hz * t / FS)); }
static int32_t g_left(uint32_t t, int ch) { return ch ? 0 : g_sine(t, 0); }
static int32_t g_burst(uint32_t t, int ch)            /* a chord for 0.5 s, then nothing */
{
    double x;
    if (t >= FS / 2u) return 0;
    x = sin(2 * M_PI * 220 * t / FS) + sin(2 * M_PI * 277.2 * t / FS + ch) + sin(2 * M_PI * 329.6 * t / FS) +
        0.5 * sin(2 * M_PI * 880 * t / FS);
    return (int32_t)(10000.0 * x);
}
static int32_t g_mix(uint32_t t, int ch)              /* a decaying 110 Hz tone every quarter, noise every 1/16 */
{
    uint32_t q = t % (FS / 2u), s16 = t % (FS / 8u);
    int32_t tone = (sine_i((uint32_t)t * 10712093u) * (int32_t)(FS / 2u - q) / (int32_t)(FS / 2u)) >> 1;
    int32_t nz = s16 < 2000u ? (int32_t)(((t * 2654435761u) >> 18) & 16383u) - 8192 : 0;
    return (tone + (ch ? -nz : nz)) * 3;
}

static void perf_reset(void)
{
    memset(&pf, 0, sizeof pf);
    pf.lc = PF_TOP;
    for (uint32_t k = 0; k < NTRK; k++)
        pf.mg[k] = 32768;
    perf_held = perf_act = perf_latched = 0;
    perf_kill = perf_cut = perf_solo = 0;
    perf_auto_on = 0;
    memset((void *)perf_k, 0, sizeof perf_k);
    sl_lent = 0;
    memset(sl, 0, sizeof sl);
    punch.req = punch.cur = -1;
    punch.g = 0;
    perf_mask = 1u << 3;                              /* (the FX button's bit) */
    fm1_in.buttons = 0;
    kb_lock = 0;
}
/* the layer on g: FX held [arm, unarm), effects e1 held [on, off) and e2 [on2, off2) (-1: none), at 120 BPM */
static int busy_after;                                /* a stage busy in the last second */
static void run2(gen_f g, uint32_t ns, uint32_t arm, uint32_t unarm, int e1, uint32_t on, uint32_t off, int e2,
                 uint32_t on2, uint32_t off2, int playing)
{
    uint32_t t, i;
    host_tracks_init();
    perf_reset();
    song.g[G_BPM] = 120;
    song.playing = (uint8_t)playing;
    beat_pos = 0;
    busy_after = 0;
    for (t = 0; t + CTL <= ns; t += CTL) {
        int b;
        fm1_in.buttons = t >= arm && t < unarm ? perf_mask : 0u;
#define AT(x) ((x) >= t && (x) < t + CTL)              /* (in this block) */
        if (e1 >= 0 && AT(on)) perf_press((uint32_t)e1, 1);
        if (e1 >= 0 && AT(off)) perf_press((uint32_t)e1, 0);
        if (e2 >= 0 && AT(on2)) perf_press((uint32_t)e2, 1);
        if (e2 >= 0 && AT(off2)) perf_press((uint32_t)e2, 0);
        for (i = 0; i < CTL; i++)
            il[t + i] = ol[t + i] = g(t + i, 0), ir[t + i] = or_[t + i] = g(t + i, 1);
        b = perf_begin(CTL);
        if (b)
            perf_block(ol + t, or_ + t, CTL);
        if (t + FS > ns && b)
            busy_after = 1;
        beat_pos = (beat_pos + CTL) % beat_samples();
    }
}
static void run_fx(int e, gen_f g, uint32_t ns, uint32_t on, uint32_t off, int playing)
{
    run2(g, ns, on > FS / 4u ? on - FS / 4u : 0u, off, e, on, off, -1, 0, 0, playing);
}
static double rms(const int32_t *x, uint32_t a, uint32_t b)
{
    double e = 0;
    uint32_t i;
    for (i = a; i < b; i++) e += (double)x[i] * x[i];
    return sqrt(e / (double)(b - a));
}
static double tone_pow(const int32_t *x, uint32_t a, uint32_t b, double hz)   /* Goertzel */
{
    double w = 2 * M_PI * hz / FS, c = 2 * cos(w), s1 = 0, s2 = 0, s0;
    uint32_t i;
    for (i = a; i < b; i++) s0 = x[i] + c * s1 - s2, s2 = s1, s1 = s0;
    return s1 * s1 + s2 * s2 - c * s1 * s2;
}
static int dry(uint32_t a, uint32_t b)                /* out == in on [a, b), both sides */
{
    uint32_t t;
    for (t = a; t < b; t++)
        if (ol[t] != il[t] || or_[t] != ir[t]) {
            if (getenv("PT_DEBUG")) printf("  (not dry at %u: %d %d, %d %d)\n", t, ol[t], il[t], or_[t], ir[t]);
            return 0;
        }
    return 1;
}
static int bounded(uint32_t a, uint32_t b)
{
    uint32_t t;
    for (t = a; t < b; t++)
        if (ol[t] <= -(1 << 22) || ol[t] >= (1 << 22) || or_[t] <= -(1 << 22) || or_[t] >= (1 << 22))
            return 0;
    return 1;
}
static double kill_ratio(int e, double hz)            /* the held effect's level on a tone (the filters settled) */
{
    tone_hz = hz;
    run_fx(e, g_sine, FS * 2u, FS / 2u, FS * 3u / 2u, 0);
    return rms(ol, FS * 4u / 5u, FS * 7u / 5u) / rms(ol, FS / 10u, FS * 2u / 5u);
}

int main(int argc, char **argv)
{
    int bad = 0;
    uint32_t e, t, i;
    char what[120];
    setvbuf(stdout, 0, _IONBF, 0);
    /* 1. every effect: held 1 s .. 1.5 s (FX from 0.5 s), playing */
    {
        int ok = 1, okd = 1, okb = 1, okr = 1;
        for (e = 0; e < PF_M1; e++) {
            uint64_t diff = 0;
            run2(g_mix, FS * 3u, FS / 2u, FS * 3u / 2u, (int)e, FS, FS * 3u / 2u, -1, 0, 0, 1);
            for (t = FS; t < FS * 3u / 2u; t++)
                diff += (uint64_t)llabs((long long)ol[t] - il[t]);
            if (diff <= 2000u * (FS / 2u / CTL)) {
                printf("perform:   %s: diff %llu\n", PUNCH_NAME[e], (unsigned long long)diff);
                ok = 0;
            }
            okd &= dry(0, FS - CTL) && dry(FS * 3u / 2u + 3u * CTL, FS * 3u);
            okb &= bounded(0, FS * 3u);
            okr &= !sl_lent && punch.cur < 0 && !busy_after;
        }
        bad += check("27 effects: each changes the mix while held (FX held, its key)", ok);
        bad += check("  dry before, exactly dry again once the key and FX are up", okd);
        bad += check("  bounded; the ring given back, the layer idle after", okb && okr);
    }
    /* 2. idle */
    run2(g_mix, FS, NS, NS, -1, 0, 0, -1, 0, 0, 1);
    bad += check("idle: nothing held, no knob: the mix bit-identical, no stage busy", dry(0, FS - CTL) && !busy_after && !sl_lent);
    run2(g_mix, FS * 2u, FS / 4u, FS / 2u, -1, 0, 0, -1, 0, 0, 1);
    bad += check("FX held alone: the ring records (borrowed), the mix untouched, given back after",
                 dry(0, FS - CTL) && !sl_lent && !busy_after);
    /* 3. the keys */
    {
        uint32_t seen = 0, ok = 1;
        for (t = 0; t < 27u; t++) {
            uint32_t k = perf_key(t);
            ok &= k < PF_M1 && punch_key_of_fx(k) == t && (key_black(t) ? k >= 16u : k == key_place(t));
            seen |= 1u << (k % 32u);
        }
        bad += check("keys: F3 .. G5 each its own effect (16 white, then 11 black), lit by its key",
                     ok && seen == PF_ALL && perf_key(1) == PX_KLOW && perf_key(25) == PX_FADE && perf_key(27) == PF_N);
    }
    /* 4. what the names say */
    {
        double a, b2;
        int ok = kill_ratio(PX_KLOW, 60) < 0.05 && fabs(kill_ratio(PX_KLOW, 1000) - 1) < 0.12 &&
                 kill_ratio(PX_KHIGH, 9000) < 0.05 && fabs(kill_ratio(PX_KHIGH, 500) - 1) < 0.12 &&
                 kill_ratio(PX_KMID, 900) < 0.08 && fabs(kill_ratio(PX_KMID, 50) - 1) < 0.12;
        bad += check("KILL LOW / MID / HIGH: 24 dB/oct at 250 Hz and 2.5 kHz", ok);
        tone_hz = 220;
        run_fx(PX_OCT, g_sine, FS * 2u, FS / 2u, FS * 3u / 2u, 0);
        a = tone_pow(ol, FS * 3u / 5u, FS * 7u / 5u, 440), b2 = tone_pow(ol, FS * 3u / 5u, FS * 7u / 5u, 220);
        snprintf(what, sizeof what, "OCT UP: 220 Hz in, 440 Hz out (power x %.0f), as loud", a / b2);
        bad += check(what, a > 30 * b2 && fabs(rms(ol, FS * 3u / 5u, FS * 7u / 5u) / rms(ol, 0, FS / 2u) - 1) < 0.3);
        tone_hz = 440;
        run_fx(PX_MONO, g_left, FS, FS / 4u, FS * 3u / 4u, 0);
        for (t = FS / 4u + 128u, ok = 1; t < FS * 3u / 4u - CTL; t++)
            if (abs(ol[t] - or_[t]) > 64) {
                if (ok && getenv("PT_DEBUG")) printf("  (MONO at %u: %d %d)\n", t, ol[t], or_[t]);
                ok = 0;
            }
        bad += check("MONO: a left-only tone on both sides", ok && rms(or_, FS / 4u + 128u, FS * 3u / 4u) > 0.4 * rms(ol, 0, FS / 4u));
        run_fx(PX_PAN, g_sine, FS * 2u, FS / 2u, FS * 3u / 2u, 0);
        {
            uint32_t lw = 0, rw = 0, q = FS / 2u / 8u;
            for (t = FS / 2u; t + q < FS * 3u / 2u; t += q) {
                double el = rms(ol, t, t + q), er = rms(or_, t, t + q);
                lw += el > 4 * er, rw += er > 4 * el;
            }
            bad += check("AUTO-PAN: hard left and hard right once a beat", lw >= 3 && rw >= 3);
        }
        run_fx(PX_FADE, g_sine, FS * 3u, FS / 2u, FS * 5u / 2u, 0);
        a = rms(ol, FS - 400u, FS + 400u) / rms(ol, 0, FS / 2u);
        bad += check("FADE OUT: a quarter at a beat, nothing at two",
                     a > 0.18 && a < 0.32 && rms(ol, FS * 3u / 2u + 64u, FS * 5u / 2u) < 0.001 * rms(ol, 0, FS / 2u));
        tone_hz = 330;
        run_fx(PX_RWD, g_sine, FS * 2u, FS / 2u, FS * 3u / 2u, 0);
        for (t = FS / 2u + 64u, i = 0; t < FS * 3u / 2u - 3u * CTL; t++)
            if (abs(ol[t]) > 100000 / 2000) i = t;
        bad += check("REWIND: the past (FX held 0.25 s before) backwards, faster, then silence until the key is up",
                     i > FS / 2u + FS / 8u && i < FS / 2u + FS / 2u &&
                     tone_pow(ol, FS / 2u + FS / 5u, FS / 2u + FS * 3u / 10u, 330 * 2.5) >
                     tone_pow(ol, FS / 2u + FS / 5u, FS / 2u + FS * 3u / 10u, 330));
        run_fx(PX_LOOP8T, g_burst, FS * 2u, FS / 4u, FS * 3u / 2u, 0);
        for (t = FS * 3u / 5u, ok = 1; t < FS * 3u / 5u + FS / 2u; t++)
            ok &= ol[t] == ol[t + beat_samples() / 3u];   /* (a beat: fx.c beat_samples, the quarter x 24) */
        bad += check("LOOP 8T: a third of a beat, over and over, exact",
                     ok && rms(ol, FS * 3u / 5u, FS * 3u / 5u + FS / 2u) > 0.3 * rms(ol, FS / 10u, FS / 5u));
        run_fx(PX_FREEZE, g_burst, FS * 3u, FS * 2u / 5u, FS * 5u / 2u, 0);
        {
            double d = rms(ol, FS / 10u, FS * 2u / 5u), h1 = rms(ol, FS, FS * 2u), tail = rms(ol, FS * 5u / 2u + 256u, FS * 3u);
            snprintf(what, sizeof what, "FREEZE: rings on after the chord stops (%.2f of it), silent after", h1 / d);
            bad += check(what, h1 > 0.2 * d && h1 < 2.0 * d && tail < 0.01 * d);
        }
        /* LOOP 16, playing: from the last 1/16 line, the loop a 1/16 long */
        run_fx(PX_LOOP16, g_burst, FS * 2u, FS / 4u + 1024u, FS * 3u / 2u, 1);
        for (t = FS * 3u / 5u, ok = 1; t < FS * 3u / 5u + FS / 2u; t++) ok &= ol[t] == ol[t + beat_samples() / 4u];
        bad += check("LOOP 16 playing: a 1/16 replayed, exact", ok);
        /* two held: the last pressed plays; let it go, the one before again */
        run2(g_mix, FS * 2u, FS / 4u, FS * 2u, PX_MONO, FS / 2u, FS * 2u, PX_KLOW, FS, FS * 3u / 2u, 1);
        bad += check("two held: the last pressed plays (KILL LOW over MONO), let go: MONO again",
                     tone_pow(ol, FS + FS / 5u, FS * 3u / 2u, 110) < 0.05 * tone_pow(il, FS + FS / 5u, FS * 3u / 2u, 110) &&
                     rms(ol, FS * 8u / 5u, FS * 9u / 5u) > 0 && ol[FS * 17u / 10u] == or_[FS * 17u / 10u]);
    }
    /* 5. the SLICER */
    {
        int ok;
        host_tracks_init();
        perf_reset();
        sl[0].rec = 100; sl[0].loop = 1; sl[0].rec_on = 1;
        fm1_in.buttons = perf_mask;
        for (i = 0; i < CTL; i++) ol[i] = or_[i] = 1000;
        ok = perf_begin(CTL);
        perf_block(ol, or_, CTL);
        ok &= sl_lent && !sl[0].rec && !sl[0].loop;
        fm1_in.buttons = 0;
        sl[0].rec = 7;
        for (t = 0; t < 8u; t++)
            if (perf_begin(CTL)) perf_block(ol, or_, CTL);
        bad += check("SLICER: FX held takes its buffer (recordings dropped); let go: given back, dropped again",
                     ok && !sl_lent && !sl[0].rec);
    }
    /* 6. the macros */
    {
        int ok;
        tone_hz = 6000;
        run2(g_sine, FS, NS, NS, -1, 0, 0, -1, 0, 0, 0);
        perf_reset();
        perf_k[0] = -100;
        for (t = 0; t + CTL <= FS; t += CTL)
            if (perf_begin(CTL)) perf_block(ol + t, or_ + t, CTL);
        ok = rms(ol, FS / 2u, FS) < 0.1 * rms(il, FS / 2u, FS);
        bad += check("FILTER (KNOB 1 all left): a 6 kHz tone down 20 dB", ok);
        run2(g_sine, FS, NS, NS, -1, 0, 0, -1, 0, 0, 0);
        perf_reset();
        perf_k[1] = 100;
        for (t = 0; t + CTL <= FS; t += CTL)
            if (perf_begin(CTL)) perf_block(ol + t, or_ + t, CTL);
        bad += check("CRUSH (KNOB 2): the mix changed, bounded", !dry(FS / 2u, FS) && bounded(0, FS));
        tone_hz = 220;
        run_fx(PX_MONO, g_left, FS, FS / 4u, FS * 3u / 4u, 0);
        {
            double full = rms(or_, FS / 2u, FS * 3u / 4u);
            host_tracks_init();
            perf_reset();
            perf_k[3] = 50;                           /* DEPTH 50: half the effect */
            for (t = 0; t + CTL <= FS; t += CTL) {
                fm1_in.buttons = perf_mask;
                if (AT(FS / 4u)) perf_press(PX_MONO, 1);
                for (i = 0; i < CTL; i++) il[t + i] = ol[t + i] = g_left(t + i, 0), ir[t + i] = or_[t + i] = 0;
                if (perf_begin(CTL)) perf_block(ol + t, or_ + t, CTL);
            }
            perf_press(PX_MONO, 0);
            bad += check("DEPTH (KNOB 4 at 50): the effect at half", fabs(rms(or_, FS / 2u, FS * 3u / 4u) / full - 0.5) < 0.1);
        }
    }
    if (argc > 1) {   /* demo: a beat dry, then each effect for a beat with a beat dry between */
        FILE *f = fopen(argv[1], "wb");
        uint32_t beat = FS / 2u, total = beat * (1u + 2u * PF_M1);
        if (f) {
            host_tracks_init();
            perf_reset();
            song.g[G_BPM] = 120;
            song.playing = 1;
            beat_pos = 0;
            wav_hdr(f, total);
            fm1_in.buttons = perf_mask;
            for (t = 0; t + CTL <= total; t += CTL) {
                int32_t l[CTL], r[CTL];
                if (t + CTL > beat && (t + CTL - beat) / beat != (t < beat ? ~0u : (t - beat) / beat)) {
                    uint32_t k = (t + CTL - beat) / beat;   /* (a new beat in this block) */
                    if (k % 2u == 0u) perf_press(k / 2u, 1);
                    else perf_press(k / 2u, 0);
                }
                for (i = 0; i < CTL; i++) l[i] = g_mix(t + i, 0), r[i] = g_mix(t + i, 1);
                if (perf_begin(CTL)) perf_block(l, r, CTL);
                for (i = 0; i < CTL; i++) wav_put(f, l[i] >> 2, r[i] >> 2);
                beat_pos = (beat_pos + CTL) % beat_samples();
            }
            fclose(f);
        }
    }
    printf("perform: %s\n", bad ? "FAILED" : "all checks ok");
    return bad;
}
