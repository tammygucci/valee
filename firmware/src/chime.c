/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) The opening sound, under the intro (main.c, web/emu/felucca_web.c: chime_start at power-on; audio.c mixes it
 * after the click, the DAC only, MASTER's level). Our own, after what the Gateway report's tapes were made of:
 *   a binaural beat: 110 Hz on the left, 116 Hz on the right (6 Hz, theta; heard on headphones), in and out slowly,
 *   under it a soft 55 Hz, and a surf of filtered noise swelling and falling twice,
 *   then, as the third eye opens, six bells rising in fourths (A3 D#4 G4 C#5 F#5 B5: the "mystic chord"), FM with an
 *   inharmonic ratio, and as the name comes up a theremin's glide down from E6 to A5.
 * 6.8 s (the last bell rung down to nothing alone), then nothing (chime_render returns at once). Included by fx.c, beside click.c. */
#define CH_HZ(f) ((uint32_t)((f) * 97391.5487f))   /* a phase step: f x 2^32 / 44100 */
enum { CK_BINAURAL, CK_SINE, CK_SURF, CK_BELL, CK_GLIDE };
static const struct {
    uint16_t at, dur;            /* ms from chime_start */
    uint32_t inc, inc2;          /* its pitch (BINAURAL: the left, the right; GLIDE: from, to) */
    uint16_t amp;                /* its peak, Q15 */
    uint8_t kind, pan;           /* pan 0 left .. 255 right */
} CHIME[] = {
    {0, 4400, CH_HZ(110.0f), CH_HZ(116.0f), 5200, CK_BINAURAL, 128},
    {300, 4100, CH_HZ(55.0f), 0, 3600, CK_SINE, 128},
    {0, 4400, 0, 0, 4200, CK_SURF, 128},
    {1600, 2400, CH_HZ(220.0f), 0, 4000, CK_BELL, 90},
    {1780, 2400, CH_HZ(311.13f), 0, 3600, CK_BELL, 170},
    {1960, 2400, CH_HZ(392.0f), 0, 3200, CK_BELL, 110},
    {2140, 2400, CH_HZ(554.37f), 0, 2900, CK_BELL, 150},
    {2320, 2400, CH_HZ(739.99f), 0, 2500, CK_BELL, 70},
    {2500, 4300, CH_HZ(987.77f), 0, 2600, CK_BELL, 190},   /* (the last: rings on alone, its own decay, to silence) */
    {2600, 2800, CH_HZ(1318.5f), CH_HZ(880.0f), 1800, CK_GLIDE, 140},
};
#define CH_NV (sizeof CHIME / sizeof CHIME[0])
#define CH_LAST (CH_NV - 2u)                  /* (the last bell, B5) */
#define CH_END_MS 6820u                       /* (the last bell and the glide rung out: ui_draw.c INTRO_MS waits) */
static struct {
    uint32_t on, t;              /* playing; samples since the start */
    uint32_t ph[CH_NV], ph2[CH_NV], vib, rnd;
    int32_t env[CH_NV];          /* the bells' decay, Q16 */
    int32_t lp, lp2;             /* the surf's filters */
} chime;
static void chime_start(void)
{
    memset(&chime, 0, sizeof chime);
    chime.rnd = 0x1234567u;
    chime.on = 1;
}
static void ch_out(int32_t *o, int32_t l, int32_t r)
{
    l += o[0];
    r += o[1];
    o[0] = l > 32767 ? 32767 : l < -32767 ? -32767 : l;
    o[1] = r > 32767 ? 32767 : r < -32767 ? -32767 : r;
}
static __attribute__((noinline)) void chime_render(int32_t *out, uint32_t n)
{
    uint32_t i, v, ms;
    int32_t g;
    if (!chime.on)
        return;
    ms = chime.t / 44u;                                 /* (44.1 samples a millisecond: near enough) */
    if (ms >= CH_END_MS) {
        chime.on = 0;
        return;
    }
    g = (int32_t)song.master_q12 * 9 / 4;               /* (MASTER, as the rest of the DAC's signal; x 2.25) */
    for (v = 0; v < CH_NV; v++) {
        uint32_t a = CHIME[v].at, d = CHIME[v].dur, e, kind = CHIME[v].kind;
        int32_t gain, idx;
        uint32_t inc = CHIME[v].inc;
        if (ms < a || ms >= a + d)
            continue;
        e = ms - a;
        if (kind == CK_BELL) {                          /* struck, then rings down */
            if (!chime.env[v])
                chime.env[v] = 65536;
            gain = (int32_t)(((int64_t)CHIME[v].amp * chime.env[v]) >> 16);
            if (e < 6u)
                gain = gain * (int32_t)(e + 1u) / 7;
        } else {                                        /* in over 0.9 s (GLIDE 0.3), out over the last 0.9 (0.4) */
            uint32_t in = kind == CK_GLIDE ? 300u : 900u, outm = kind == CK_GLIDE ? 400u : 900u;
            gain = (int32_t)CHIME[v].amp * (int32_t)(e < in ? e : in) / (int32_t)in;
            if (a + d - ms < outm)
                gain = gain * (int32_t)(a + d - ms) / (int32_t)outm;
            if (kind == CK_GLIDE)
                inc = CHIME[v].inc - (uint32_t)((uint64_t)(CHIME[v].inc - CHIME[v].inc2) * (e < 900u ? e : 900u) / 900u);
            if (kind == CK_SURF) {                      /* two swells: the gain and the brightness rise and fall */
                int32_t sw = 16384 - sine_i(0x40000000u + (uint32_t)((uint64_t)e * 0x100000000ull / 1950u)) / 2;
                gain = (int32_t)((int64_t)gain * (6000 + sw) >> 15);
            }
        }
        if (kind == CK_BELL) {                          /* (its end: a 0.3 s tail, no click; the last bell 1 s) */
            uint32_t tl = v == CH_LAST ? 1000u : 300u;
            if (a + d - ms < tl)
                gain = (int32_t)((int64_t)gain * (int32_t)(a + d - ms) * (int32_t)(a + d - ms) / (int32_t)(tl * tl));
        }
        idx = kind == CK_BELL ? (int32_t)(((int64_t)52000 * chime.env[v]) >> 16) + 6000 : 0;
        for (i = 0; i < n; i++) {
            int32_t s, s2, m;
            uint32_t vi = inc;
            if (kind == CK_BINAURAL) {                  /* the left its tone, the right its own */
                chime.ph[v] += CHIME[v].inc;
                chime.ph2[v] += CHIME[v].inc2;
                s = (int32_t)(((int64_t)sine_i(chime.ph[v]) * gain) >> 15);
                s2 = (int32_t)(((int64_t)sine_i(chime.ph2[v]) * gain) >> 15);
                ch_out(&out[2u * i], (int32_t)(((int64_t)s * g) >> 12), (int32_t)(((int64_t)s2 * g) >> 12));
                continue;
            }
            if (kind == CK_SURF) {                      /* white noise, two poles low, its corner with the swell */
                int32_t k = 1500 + gain;
                chime.rnd = chime.rnd * 1664525u + 1013904223u;
                s = (int32_t)(chime.rnd >> 17) - 16384;
                chime.lp += ((s - chime.lp) * k) >> 15;
                chime.lp2 += ((chime.lp - chime.lp2) * k) >> 15;
                s = (int32_t)(((int64_t)chime.lp2 * gain) >> 13);
                s = (int32_t)(((int64_t)s * g) >> 12);
                ch_out(&out[2u * i], s, (s * 7) / 8);
                continue;
            }
            if (kind == CK_GLIDE)                       /* (the vibrato: 5.5 Hz) */
                vi = inc + (uint32_t)((int64_t)inc * sine_i(chime.vib + i * CH_HZ(5.5f)) / 1200000);
            chime.ph[v] += vi;
            chime.ph2[v] += kind == CK_BELL ? vi / 2u * 7u : vi * 2u;   /* (the bells' ratio 3.5: no octaves) */
            m = kind == CK_BELL ? sine_i(chime.ph2[v]) : 0;
            s = sine_i(chime.ph[v] + (uint32_t)(m * idx));
            s = (int32_t)(((int64_t)s * gain) >> 15);
            s = (int32_t)(((int64_t)s * g) >> 12);
            ch_out(&out[2u * i], s * (int32_t)(255u - CHIME[v].pan) >> 7, s * (int32_t)CHIME[v].pan >> 7);
        }
        if (kind == CK_BELL)                            /* (about 0.55 s to a third; the last, alone, 2.2 s) */
            chime.env[v] -= (int32_t)(((int64_t)chime.env[v] * (int32_t)n) >> (v == CH_LAST ? 16 : 14)) + 1;
        if (chime.env[v] < 1)                           /* (never 0: 0 strikes it again) */
            chime.env[v] = 1;
    }
    chime.vib += n * CH_HZ(5.5f);
    chime.t += n;
}
