/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* DRUM ((Vallée, the SPAMPY sound): SPAMPY's drum player as an engine, src/eng_drum.c and src/drum_synth.c) on the host,
 * through hostsim.c as regress.c.
 *   build/host/drum_test [DEMODIR]          (run_tests.sh: build/drum_demo)
 * 1. every kit (the GM sample kit, its four treatments, the 30 synthesised ones) x every lane (SPAMPY's 16): one hit
 *    through the engine is heard, stays under full scale and ends (its voice frees itself) within 6 s; a kit's
 *    loudest lane near the sampled kit's kick (SPAMPY's drumkit_test: a quarter .. three times its peak).
 * 2. the synthesised voice is SPAMPY's: drum_synth.c's own render, sample for sample, equals the engine's voice before
 *    the mix (velocity, TUNE, DECAY passed as SPAMPY passes them).
 * 3. the lanes' own: TUNE up raises a pitched lane (its zero crossings), DECAY down shortens a hit, up lengthens it,
 *    BODY + adds low energy and - takes it away, 0 is bit for bit; LEVEL 0 silences a lane, PAN hard left silences the
 *    right; the lanes' mutes; another lane's settings leave a lane bit for bit.
 * 4. voices: SPAMPY's 6 at most (a seventh hit takes the oldest), a closed hat chokes the open one (below -40 dB
 *    within 5 ms), a hit on a sounding voice no click (the step at the restart under 1/4 of the peak).
 * 5. the keys: SPAMPY's, F3 KICK .. G5 COWBELL, a black key its left neighbour's.
 * 6. cost: host time of 6 voices of a synthesised kit and of the sample kit (reported).
 * 7. demos into DEMODIR: each kit plays a bar. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <time.h>

static int fails;
static void check(const char *what, int ok)
{
    printf("drum_test: %-100s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}

static uint32_t voices_on(const track_t *t)
{
    uint32_t i, n = 0;
    for (i = 0; i < NVOICE; i++)
        n += t->v[i].active;
    return n;
}
/* the track's own output (no sends, no master): nb blocks, left and right into l, r (may be 0) */
static void part_blocks(uint32_t nb, int32_t *l, int32_t *r, int32_t *peak)
{
    int32_t o[2 * CTL];
    uint32_t i, k;
    for (i = 0; i < nb; i++) {
        mix_block(o, CTL);
        for (k = 0; k < CTL; k++) {
            if (l) l[i * CTL + k] = o[2 * k];
            if (r) r[i * CTL + k] = o[2 * k + 1];
            if (peak) {
                int32_t a = abs(o[2 * k]) > abs(o[2 * k + 1]) ? abs(o[2 * k]) : abs(o[2 * k + 1]);
                *peak = a > *peak ? a : *peak;
            }
        }
    }
}
/* track 1's own render (voice.c track_render: no mix, no master, nothing of before): nb blocks into x */
static void trk_blocks(uint32_t nb, int32_t *x, int32_t *peak)
{
    int32_t o[CTL];
    uint32_t i, k;
    for (i = 0; i < nb; i++) {
        track_render(&trk[0], o, CTL);
        for (k = 0; k < CTL; k++) {
            if (x) x[i * CTL + k] = o[k];
            if (peak && abs(o[k]) > *peak) *peak = abs(o[k]);
        }
    }
}
/* track 1 on DRUM, KIT kit, nothing sounding, the mix plain (no reverb, no master effects) */
static void kit_fresh(uint32_t kit)
{
    host_tracks_init();
    memset(trk[0].v, 0, sizeof trk[0].v);
    memset(eng_mem, 0, sizeof eng_mem);   /* (the parts' shared engine area: engines.c) */
    host_preset(&trk[0], ENGI_DRUM, 0);
    trk[0].p[P_E0] = (int16_t)kit;
    trk[0].p[P_REV] = trk[0].p[P_DLY] = trk[0].p[P_CHOR] = 0;
    trk[0].p[P_MUTE] = 0;
    song.master_q12 = 4096;
}

#define NMAX (FS * 6u)
static int32_t L[NMAX], R[NMAX], L2[NMAX];

/* one hit (note, velocity 110) of track 1 alone: its blocks until its voices are free (at most 6 s) into L, R; how many
 * blocks, the peak */
static uint32_t one_hit(uint32_t kit, uint32_t note, int32_t *peak)
{
    uint32_t b;
    kit_fresh(kit);
    trk_note_on(&trk[0], note, 110);
    *peak = 0;
    for (b = 0; b < NMAX / CTL; b++) {
        part_blocks(1, L + b * CTL, R + b * CTL, peak);
        if (!voices_on(&trk[0]))
            break;
    }
    return b;
}

/* 1. every kit, every lane */
static void kits(void)
{
    uint32_t kit, l, worst_b = 0, silent = 0, loud = 0, endless = 0, far = 0;
    int32_t ref, pk, kpk;
    one_hit(0, 36, &ref);                               /* the sampled kick */
    for (kit = 0; kit < DRUM_KITS; kit++) {
        kpk = 0;
        for (l = 0; l < NLANE; l++) {
            uint32_t b = one_hit(kit, DRUM_LANE_NOTE[l], &pk);
            silent += pk < 600;
            loud += pk >= 32767 * 4;
            endless += b >= NMAX / CTL;
            worst_b = b > worst_b ? b : worst_b;
            kpk = pk > kpk ? pk : kpk;
        }
        if (kit >= DRUM_SAMPLED && (kpk > ref * 3 || kpk < ref / 4)) {
            printf("drum_test:   %s: loudest lane %d, the sampled kick %d\n", N_DRUM_KIT[kit], kpk, ref);
            far++;
        }
    }
    printf("drum_test:   35 kits x 16 lanes: the longest hit %u ms; the sampled kick's peak %d\n",
           worst_b * CTL * 1000u / FS, ref);
    check("every kit x every lane: heard, bounded, ends within 6 s (its voice free)", !silent && !loud && !endless &&
          ref > 600);
    check("every synthesised kit's loudest lane near the sampled kit's kick (SPAMPY's drumkit_test)", !far);
}

/* 2. the synthesised voice is drum_synth.c's own */
static void synth_exact(void)
{
    static const uint8_t NOTES[5] = {36, 38, 42, 49, 63};
    uint32_t kit, i, b, bad = 0, vi;
    for (kit = DRUM_SAMPLED; kit < DRUM_KITS; kit += 7u)
        for (i = 0; i < 5u; i++) {
            drum_part_t *P;
            dsv_t ref;
            int32_t y[CTL], vo[CTL];
            vmod_t m;
            kit_fresh(kit);
            trk[0].p[lp_tune(drum_lane(NOTES[i]))] = (int16_t)(i ? (int32_t)i - 2 : 3);
            trk[0].p[lp_dec(drum_lane(NOTES[i]))] = (int16_t)((int32_t)i - 2);
            trk_note_on(&trk[0], NOTES[i], 97);
            P = drum_part_of(&trk[0]);
            for (vi = 0; vi < DRUM_NV && !trk[0].v[vi].active; vi++)
                ;
            if (vi == DRUM_NV) {
                bad++;
                continue;
            }
            ref = P->ds[vi];                             /* (as ds_on left it: seeded by the part) */
            memset(&m, 0, sizeof m);
            m.amp0 = m.amp1 = 32767;
            for (b = 0; b < 200u; b++) {
                int alive = ds_render(&ref, y, CTL);
                memset(vo, 0, sizeof vo);
                drum_render(&trk[0], &trk[0].v[vi], vo, CTL, &m);
                for (uint32_t k = 0; k < CTL; k++)
                    bad += vo[k] != voice_amp2(y[k], &m, k);
                if (!alive)
                    break;
            }
        }
    check("a synthesised kit's voice is drum_synth.c's render, sample for sample (TUNE, DECAY as SPAMPY passes them)",
          !bad);
}

static uint32_t crossings(const int32_t *x, uint32_t a, uint32_t e)
{
    uint32_t i, n = 0;
    for (i = a + 1u; i < e; i++)
        n += (x[i - 1] < 0) != (x[i] < 0);
    return n;
}
static uint32_t len_40db(const int32_t *x, uint32_t n, int32_t peak)   /* samples until it stays 40 dB down */
{
    uint32_t i, last = 0;
    for (i = 0; i < n; i++)
        if (abs(x[i]) * 100 > peak)
            last = i;
    return last;
}
static double low_energy(const int32_t *x, uint32_t n)                 /* a one-pole low-pass at ~150 Hz, its energy */
{
    double y = 0, e = 0, a = 1.0 - exp(-2.0 * M_PI * 150.0 / FS);
    uint32_t i;
    for (i = 0; i < n; i++) {
        y += a * (x[i] - y);
        e += y * y;
    }
    return e;
}

/* 3. the lanes' own */
static void lanes(void)
{
    int32_t pk0, pk1;
    uint32_t b0, b1, n;
    uint64_t h0 = 0, h1 = 0;
    int ok;
    /* TUNE: the 808's LOW TOM (a pitched lane) a few semitones up: more zero crossings */
    b0 = one_hit(DRUM_SAMPLED, 43, &pk0);
    n = b0 * CTL;
    memcpy(L2, L, n * 4u);
    kit_fresh(DRUM_SAMPLED);
    trk[0].p[lp_tune(9)] = 5;
    trk_note_on(&trk[0], 43, 110);
    part_blocks(b0, L, 0, 0);
    ok = crossings(L, 0, n / 2u) > crossings(L2, 0, n / 2u) * 5 / 4;
    check("TUNE +5 raises a pitched lane (the 808's LOW TOM)", ok);
    /* DECAY: the 909's OPEN HAT shorter at -4, longer at +3; the sample kit's too at -4 */
    b0 = one_hit(DRUM_SAMPLED + 1u, 46, &pk0);
    {
        uint32_t base = len_40db(L, b0 * CTL, pk0), lo, hi;
        kit_fresh(DRUM_SAMPLED + 1u);
        trk[0].p[lp_dec(5)] = -4;
        trk_note_on(&trk[0], 46, 110);
        pk1 = 0;
        part_blocks(NMAX / CTL, L, 0, &pk1);
        lo = len_40db(L, NMAX, pk0);
        kit_fresh(DRUM_SAMPLED + 1u);
        trk[0].p[lp_dec(5)] = 3;
        trk_note_on(&trk[0], 46, 110);
        part_blocks(NMAX / CTL, L, 0, 0);
        hi = len_40db(L, NMAX, pk0);
        printf("drum_test:   909 OPEN HAT 40 dB down: %u ms, DECAY -4 %u ms, +3 %u ms\n", base * 1000u / FS, lo * 1000u / FS,
               hi * 1000u / FS);
        ok = lo < base * 2 / 3 && hi > base * 3 / 2;
        b0 = one_hit(0, 49, &pk0);
        base = len_40db(L, b0 * CTL, pk0);
        kit_fresh(0);
        trk[0].p[lp_dec(11)] = -4;
        trk_note_on(&trk[0], 49, 110);
        part_blocks(NMAX / CTL, L, 0, 0);
        lo = len_40db(L, NMAX, pk0);
        ok &= lo < base / 2;
    }
    check("DECAY -4 shortens a hit, +3 lengthens it (the 909's OPEN HAT); -4 fades a sample out (ACOUSTIC CRASH)", ok);
    /* BODY: the 808 KICK at +4 more low energy, -4 less; 0 bit for bit with a neighbour's BODY set */
    {
        double e0, ep, em;
        b0 = one_hit(DRUM_SAMPLED, 36, &pk0);
        n = b0 * CTL;
        kit_fresh(DRUM_SAMPLED);
        trk_note_on(&trk[0], 36, 110);
        trk_blocks(b0, L, 0);
        memcpy(L2, L, n * 4u);
        e0 = low_energy(L, n);
        kit_fresh(DRUM_SAMPLED);
        trk[0].p[lp_body(0)] = 4;
        trk_note_on(&trk[0], 36, 110);
        trk_blocks(b0, L, 0);
        ep = low_energy(L, n);
        kit_fresh(DRUM_SAMPLED);
        trk[0].p[lp_body(0)] = -4;
        trk_note_on(&trk[0], 36, 110);
        trk_blocks(b0, L, 0);
        em = low_energy(L, n);
        kit_fresh(DRUM_SAMPLED);
        trk[0].p[lp_body(1)] = 4;                        /* (KICK 2's) */
        trk[0].p[lp_tune(2)] = 3;                        /* (SNARE's) */
        trk[0].p[lp_ln(4)] = 0;                          /* (HAT's) */
        trk_note_on(&trk[0], 36, 110);
        trk_blocks(b0, L, 0);
        printf("drum_test:   808 KICK low energy: BODY +4 x%.2f, -4 x%.2f\n", ep / e0, em / e0);
        check("BODY +4 adds low energy, -4 takes it away; another lane's BODY TUNE LEVEL leave a lane bit for bit",
              ep > e0 * 1.3 && em < e0 * 0.7 && !memcmp(L, L2, n * 4u));
    }
    /* LEVEL 0 silences a lane; PAN hard left: the right silent; a muted lane unheard */
    kit_fresh(DRUM_SAMPLED);
    trk[0].p[lp_ln(2)] = 0;
    trk_note_on(&trk[0], 38, 110);
    pk0 = 0;
    trk_blocks(200, L, &pk0);
    kit_fresh(DRUM_SAMPLED);
    trk[0].p[lp_pan(2)] = -64;
    trk_note_on(&trk[0], 38, 110);
    part_blocks(200, L, R, 0);
    for (b0 = 0, h0 = 0, h1 = 0; b0 < 200u * CTL; b0++)
        h0 += (uint64_t)abs(L[b0]), h1 += (uint64_t)abs(R[b0]);
    kit_fresh(DRUM_SAMPLED);
    drum_mute[0] = 1u << 2;
    trk_note_on(&trk[0], 38, 110);
    pk1 = 0;
    trk_blocks(200, L, &pk1);
    drum_mute[0] = 0;
    check("LEVEL 0 silences a lane, PAN hard left the right side, a muted lane unheard", !pk0 && h0 > 1000u && h1 * 50u < h0 &&
          !pk1);
    (void)b1;
}

/* 4. voices */
static void voices(void)
{
    int32_t pk = 0, pk2 = 0, before;
    uint32_t i, ok;
    kit_fresh(DRUM_SAMPLED + 1u);                        /* (909: long tails) */
    for (i = 0; i < 7u; i++)
        trk_note_on(&trk[0], DRUM_LANE_NOTE[(i * 5u) % NLANE], 110);
    ok = voices_on(&trk[0]) == DRUM_NV;
    part_blocks(10, 0, 0, 0);
    ok &= voices_on(&trk[0]) <= DRUM_NV;
    check("SPAMPY's 6 voices at most: a seventh hit takes one", ok);
    /* the choke: an open hat, then 100 ms later a closed one: the open one 40 dB down within 5 ms */
    kit_fresh(DRUM_SAMPLED + 1u);
    trk[0].p[lp_ln(4)] = 0;                              /* (the closed hat itself unheard: what is left, the open one) */
    trk_note_on(&trk[0], 46, 110);
    trk_blocks(FS / 10u / CTL, L, &pk);
    {
        voice_t *oh = 0;
        for (i = 0; i < NVOICE; i++)
            if (trk[0].v[i].active && trk[0].v[i].note == 46u)
                oh = &trk[0].v[i];
        trk_note_on(&trk[0], 42, 110);
        trk_blocks(8, L, 0);
        before = 0;
        for (i = 5u * FS / 1000u; i < 8u * CTL; i++)
            before = abs(L[i]) > before ? abs(L[i]) : before;
        check("a closed hat chokes the open one (40 dB down within 5 ms, its voice free)",
              oh && !oh->active && before * 100 < pk);
    }
    /* a hit on a sounding voice: no click (the step at the restart) */
    kit_fresh(DRUM_SAMPLED);
    for (i = 0; i < DRUM_NV; i++)
        trk_note_on(&trk[0], DRUM_LANE_NOTE[i], 120);
    part_blocks(3, L, 0, &pk2);
    trk_note_on(&trk[0], 36, 120);                     /* (a seventh: one of them taken while it sounds) */
    part_blocks(2, L2, 0, 0);
    {
        int32_t step = abs(L2[0] - L[3 * CTL - 1]);
        printf("drum_test:   a voice taken while it sounds: the step %d of the peak %d\n", step, pk2);
        check("a hit on a sounding voice: no click (the step at the restart under 1/4 of the peak)", step * 4 < pk2);
    }
}

/* 5. the keys */
static void keys(void)
{
    static const uint8_t WL[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};   /* white keys from F3 */
    uint32_t i, ok = ENGINES[ENGI_DRUM]->keys != 0;
    for (i = 0; ok && i < 16u; i++)
        ok &= ENGINES[ENGI_DRUM]->keys(&trk[0], WL[i]) == DRUM_LANE_NOTE[i];
    ok &= ENGINES[ENGI_DRUM]->keys(&trk[0], 1) == DRUM_LANE_NOTE[0] && ENGINES[ENGI_DRUM]->keys(&trk[0], 13) == DRUM_LANE_NOTE[7];
    check("the keys: SPAMPY's, F3 KICK .. G5 COWBELL, a black key its left neighbour's", ok);
}

/* 6. cost */
static void cost(void)
{
    uint32_t k, j;
    double ns[2];
    for (k = 0; k < 2u; k++) {
        struct timespec t0, t1;
        kit_fresh(k ? DRUM_SAMPLED + 1u : 0u);
        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (j = 0; j < 2000u; j++) {
            if (j % 100u == 0u) {
                static const uint8_t N[6] = {46, 49, 51, 36, 38, 39};
                uint32_t q;
                for (q = 0; q < 6u; q++)
                    trk_note_on(&trk[0], N[q], 100);
            }
            part_blocks(1, 0, 0, 0);
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        ns[k] = ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / (2000.0 * CTL);
    }
    printf("drum_test: cost: 6 voices, the whole mix: sample kit %.1f ns/sample, synthesised %.1f ns/sample (host)\n",
           ns[0], ns[1]);
}

/* 7. demos */
static void demos(const char *dir)
{
    static const uint8_t P[16][3] = {{36, 42, 0}, {0}, {42, 0}, {0}, {38, 42, 0}, {0}, {42, 0}, {70, 0},
                                     {36, 42, 0}, {36, 0}, {42, 0}, {37, 0}, {38, 39, 0}, {0}, {42, 63, 0}, {46, 0}};
    char path[512];
    uint32_t step = FS * 60u / 120u / 4u / CTL, kit, j, q, s, k;
    int32_t o[2 * CTL];
    FILE *f;
    snprintf(path, sizeof path, "%s/drum_kits.wav", dir);
    if (!(f = fopen(path, "wb")))
        return;
    wav_hdr(f, DRUM_KITS * 16u * step * CTL);
    for (kit = 0; kit < DRUM_KITS; kit++) {
        kit_fresh(kit);
        for (j = 0; j < 16u; j++) {
            for (q = 0; q < 3u && P[j][q]; q++)
                trk_note_on(&trk[0], P[j][q], q ? 90 : 115);
            for (s = 0; s < step; s++) {
                mix_block(o, CTL);
                for (k = 0; k < CTL; k++)
                    wav_put(f, clamp(o[2 * k], -32767, 32767), clamp(o[2 * k + 1], -32767, 32767));
            }
        }
    }
    fclose(f);
    printf("drum_test: demo: %s (each kit a bar)\n", path);
}

int main(int argc, char **argv)
{
    kits();
    synth_exact();
    lanes();
    voices();
    keys();
    if (!getenv("NOCOST"))
        cost();
    if (argc > 1)
        demos(argv[1]);
    printf("drum_test: %s\n", fails ? "FAILED" : "all checks ok");
    return fails != 0;
}
