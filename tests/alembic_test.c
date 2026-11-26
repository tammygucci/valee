/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) Host test of GLO > ALEMBIC (src/alembic.c): the whole mix through DRIVE, MOTION, SPACE and GRAIN, on the
 * real sources (as insert_test.c):
 * 1. bit for bit: every flavour of every stage at BLEND 0 renders the phrase the defaults render (nothing runs); BLEND
 *    100 of each changes it.
 * 2. each flavour on a loud mix (a chord of saws, ~1.5 x a voice's scale, and noise) at BLEND 1, 64, 127: finite and
 *    bounded (DUST: within its pops), no DC added; the input gone, the stage's output dies away (DUST: only its pops).
 * 3. BLEND glides: from 0 to 127 the first samples barely move; a flavour changed fades the old one out, then the new
 *    one comes in; BLEND back to 0: the stage at rest, the mix bit for bit again.
 * 4. SPACE: ECHO feeds the delay's send only, VERB the reverb's only, BOTH both; a stereo delay: left to left, right
 *    to right; nothing at BLEND 0.
 * 5. the pages: GLO > ALEMBIC and ALEMBIC 2, their knobs (DRIVE BLEND MOTION BLEND, SPACE BLEND GRAIN BLEND); a project
 *    keeps them; a project without them (the 2.5 globals) loads them off.
 * 6. cost (host ns): all four stages at their heaviest against six track INSERTs of the same kinds. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <math.h>
#include <sys/wait.h>
#include <unistd.h>

static int bad;
static void ck(const char *what, int ok)
{
    printf("alembic: %-96s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}
static uint32_t xs = 0x2545F491u;
static int32_t noise(int32_t amp)
{
    xs ^= xs << 13;
    xs ^= xs >> 17;
    xs ^= xs << 5;
    return (int32_t)(((int64_t)(int32_t)xs * amp) >> 31);
}
static void alm_off(void)
{
    uint32_t g;
    for (g = G_ADRV; g <= G_AGRNB; g++)
        song.g[g] = 0;
    song.g[G_AMED] = song.g[G_AMEDB] = 0;               /* (MEDIUM: ALEMBIC 3) */
    song.g[G_AWEAR] = song.g[G_AWOW] = 64;
    memset(&alm, 0, sizeof alm);
    memset(&amd, 0, sizeof amd);
    alm.seed = 0x5EEDu;
}

/* ------------------------------------------------------------------ 1 --- */
static int32_t out_buf[2 * CTL];
static uint64_t hash;
static void mixb(uint32_t n)
{
    uint32_t i, k;
    while (n--) {
        mix_block(out_buf, CTL);
        for (i = 0; i < 2u * CTL; i++)
            for (k = 0; k < 4u; k++) {
                hash ^= ((uint32_t)out_buf[i] >> (8u * k)) & 0xFFu;
                hash *= 0x100000001B3ull;
            }
    }
}
static int ph_g = -1, ph_f, ph_b;                       /* the stage (0..3, -1: the defaults), its flavour, BLEND */
/* the hash of a phrase on tracks 1 and 2 (a pad, a pluck; their reverb and delay sends) in a fork()ed child */
static uint64_t phrase(void)
{
    int fd[2];
    uint64_t h = 0;
    pid_t pid;
    if (pipe(fd))
        return 0;
    fflush(stdout);
    if (!(pid = fork())) {
        host_tracks_init();
        alm_off();
        song.master_q12 = MASTER_FULL;
        host_preset(&trk[0], 0, 1);
        host_preset(&trk[1], 0, 8);
        trk[0].p[P_REV] = 60;
        trk[1].p[P_DLY] = 70;
        hash = 0xCBF29CE484222325ull;
        if (ph_g >= 0) {
            song.g[G_ADRV + 2 * ph_g] = (int16_t)ph_f;
            song.g[G_ADRVB + 2 * ph_g] = (int16_t)ph_b;
        }
        trk_note_on(&trk[0], 48, 100);
        trk_note_on(&trk[0], 55, 90);
        trk_note_on(&trk[1], 67, 120);
        mixb(FS / 2u / CTL);
        trk_note_off(&trk[1], 67);
        trk_note_on(&trk[1], 72, 110);
        mixb(FS / 4u / CTL);
        trk_note_off(&trk[0], 48);
        trk_note_off(&trk[0], 55);
        trk_note_off(&trk[1], 72);
        mixb(FS / CTL);
        h = hash;
        if (write(fd[1], &h, sizeof h) != sizeof h)
            _exit(1);
        _exit(0);
    }
    close(fd[1]);
    if (read(fd[0], &h, sizeof h) != sizeof h)
        h = 0;
    close(fd[0]);
    waitpid(pid, 0, 0);
    return h;
}
static const uint32_t NFLAV[4] = {4, 4, 3, 4};
static void test_identity(void)
{
    uint64_t h0;
    int same = 1, diff = 1;
    uint32_t g, f;
    ph_g = -1;
    h0 = phrase();
    for (g = 0; g < 4u; g++)
        for (f = 0; f < NFLAV[g]; f++) {
            ph_g = (int)g;
            ph_f = (int)f;
            ph_b = 0;
            same &= phrase() == h0;
            ph_b = 100;
            diff &= phrase() != h0;
        }
    ph_g = -1;
    ck("every flavour at BLEND 0 renders the defaults' phrase bit for bit (nothing runs)", h0 && same);
    ck("every flavour at BLEND 100 changes it", diff);
}

/* ------------------------------------------------------------------ 2 --- */
#define NB (FS / CTL)                                   /* a second of blocks */
static int32_t al_l[CTL], al_r[CTL], sd[CTL], se[CTL], sv[CTL];
static double t_ph;
static void loud(int32_t *l, int32_t *r, double amp)    /* a chord of saws (A2 E3 C#4) and a little noise */
{
    uint32_t i;
    for (i = 0; i < CTL; i++, t_ph += 1.0 / FS) {
        double s = 0;
        static const double F[3] = {110.0, 164.8, 277.2};
        uint32_t k;
        for (k = 0; k < 3u; k++)
            s += 2.0 * (t_ph * F[k] - floor(t_ph * F[k] + 0.5));
        l[i] = (int32_t)(amp * s / 3.0) + noise(2000);
        r[i] = (int32_t)(amp * s / 3.0 * 0.8) + noise(2000);
    }
}
/* stage g, flavour f, BLEND b: one block through ALEMBIC as mix_block runs it (pre for DRIVE MOTION SPACE, post GRAIN) */
static void run_blk(uint32_t g, int32_t *l, int32_t *r)
{
    memset(sd, 0, sizeof sd);
    memset(se, 0, sizeof se);
    memset(sv, 0, sizeof sv);
    (void)g;
    if (alm_pre_on())
        alm_pre(l, r, sd, se, sv, CTL);
    if (alm_post_on())
        alm_post(l, r, CTL);
}
static void test_bounded(void)
{
    static const int16_t BL[3] = {1, 64, 127};
    uint32_t g, f, k, b, i;
    int ok_b = 1, ok_dc = 1, ok_tail = 1;
    char what[160];
    double worst = 0, wdc = 0;
    for (g = 0; g < 4u; g++)
        for (f = 0; f < NFLAV[g]; f++)
            for (k = 0; k < 3u; k++) {
                double sin_ = 0, sout = 0, peak = 0;
                int32_t tail, lo = 0x7FFFFFFF, hi = -0x7FFFFFFF;
                alm_off();
                t_ph = 0;
                song.g[G_ADRV + 2u * g] = (int16_t)f;
                song.g[G_ADRVB + 2u * g] = BL[k];
                for (b = 0; b < NB; b++) {
                    int32_t l0[CTL];
                    loud(al_l, al_r, 160000.0);
                    memcpy(l0, al_l, sizeof l0);
                    run_blk(g, al_l, al_r);
                    for (i = 0; i < CTL; i++) {
                        double a = fabs((double)al_l[i]) > fabs((double)al_r[i]) ? fabs((double)al_l[i]) : fabs((double)al_r[i]);
                        peak = a > peak ? a : peak;
                        if (b >= NB / 4u) {
                            sin_ += l0[i];
                            sout += al_l[i];
                        }
                    }
                }
                for (b = 0; b < NB; b++) {      /* the input gone (what stays: its swing; a constant offset is the
                                                 * master's DC blocker's, fx.c master_pre) */
                    memset(al_l, 0, sizeof al_l);
                    memset(al_r, 0, sizeof al_r);
                    run_blk(g, al_l, al_r);
                    if (b >= NB / 2u)
                        for (i = 0; i < CTL; i++) {
                            lo = al_l[i] < lo ? al_l[i] : lo;
                            hi = al_l[i] > hi ? al_l[i] : hi;
                            lo = al_r[i] < lo ? al_r[i] : lo;
                            hi = al_r[i] > hi ? al_r[i] : hi;
                        }
                }
                tail = hi - lo;
                {
                    double dc = fabs(sout - sin_) / (3.0 * NB / 4.0 * CTL);
                    ok_b &= peak < 160000.0 * 1.6 && peak == peak;
                    ok_dc &= dc < 0.003 * 160000.0;
                    wdc = dc > wdc ? dc : wdc;
                    worst = peak / 160000.0 > worst ? peak / 160000.0 : worst;
                    if (!(g == 3u && f == 1u))  /* (DUST: pops go on in the silence) */
                        ok_tail &= tail <= 8;   /* (FLANG's feedback: a swing of 2 LSB of its quarter at most) */
                    else
                        ok_tail &= tail <= 8200;   /* (a pop: +-1024 at the quarter) */
                }
            }
    snprintf(what, sizeof what, "every flavour at BLEND 1 64 127 on a loud mix: bounded (worst %.2f x the input, limit 1.6)",
             worst);
    ck(what, ok_b);
    snprintf(what, sizeof what, "  no DC added (the means over 0.75 s: worst %.1f apart, limit 0.3 %% of the input's peak)", wdc);
    ck(what, ok_dc);
    ck("  the input gone: no swing left after half a second (8 at most; DUST: its pops only)", ok_tail);
    alm_off();
}

/* ------------------------------------------------------------------ 3 --- */
static void test_glide(void)
{
    int32_t l0[CTL], step = 0;
    uint32_t i, b;
    int ok;
    alm_off();
    song.g[G_ADRV] = 2;                                 /* FOLD, the most changed */
    loud(al_l, al_r, 160000.0);
    memcpy(l0, al_l, sizeof l0);
    song.g[G_ADRVB] = 127;
    run_blk(0, al_l, al_r);
    for (i = 0; i < 4u; i++)                            /* (the first samples: a step of the glide each) */
        step = abs(al_l[i] - l0[i]) > step ? abs(al_l[i] - l0[i]) : step;
    ck("BLEND 0 -> 127: the first samples barely move (a glide, no step)", step < 2000);
    for (b = 0; b < 20u; b++) {
        loud(al_l, al_r, 160000.0);
        run_blk(0, al_l, al_r);
    }
    song.g[G_ADRV] = 0;                                 /* FOLD -> SOFT */
    loud(al_l, al_r, 160000.0);
    run_blk(0, al_l, al_r);
    ok = alm.s[0][0].type == IT_FOLD && alm.s[0][0].w < 32768;   /* (FOLD fading out first) */
    for (b = 0; b < 20u; b++) {
        loud(al_l, al_r, 160000.0);
        run_blk(0, al_l, al_r);
    }
    ok &= alm.s[0][0].type == IT_SOFT && alm.s[0][0].w == 32768;
    ck("a flavour changed (FOLD -> SOFT): the old one fades out, then the new one in", ok);
    song.g[G_ADRVB] = 0;
    for (b = 0; b < 20u; b++) {
        loud(al_l, al_r, 160000.0);
        run_blk(0, al_l, al_r);
    }
    loud(al_l, al_r, 160000.0);
    memcpy(l0, al_l, sizeof l0);
    ok = !alm_pre_on();
    run_blk(0, al_l, al_r);
    ok &= !memcmp(l0, al_l, sizeof l0);
    ck("BLEND back to 0: the stage at rest, the mix bit for bit", ok);
    alm_off();
}

/* ------------------------------------------------------------------ 4 --- */
static void test_space(void)
{
    uint32_t f, b, i;
    int ok = 1;
    for (f = 0; f < 3u; f++) {
        int64_t ed = 0, ee = 0, er = 0;
        alm_off();
        song.g[G_ASPC] = (int16_t)f;
        song.g[G_ASPCB] = 127;
        for (b = 0; b < 40u; b++) {
            loud(al_l, al_r, 100000.0);
            for (i = 0; i < CTL; i++)
                al_r[i] = 0;                              /* (the left only) */
            memset(sd, 0, sizeof sd);
            memset(se, 0, sizeof se);
            memset(sv, 0, sizeof sv);
            alm_pre(al_l, al_r, sd, se, sv, CTL);
            for (i = 0; i < CTL; i++) {
                ed += llabs(sd[i]);
                ee += llabs(se[i]);
                er += llabs(sv[i]);
            }
        }
        if (f == 0u)
            ok &= ed > 0 && er == 0 && ee == 0;
        else if (f == 1u)
            ok &= ed == 0 && ee == 0 && er > 0;
        else
            ok &= ed > 0 && ee == 0 && er > 0;
    }
    ck("SPACE: ECHO the delay's send only, VERB the reverb's only, BOTH both; the left to the left", ok);
    alm_off();
    song.g[G_ASPC] = 2;
    loud(al_l, al_r, 100000.0);
    memset(sd, 0, sizeof sd);
    memset(sv, 0, sizeof sv);
    alm_pre(al_l, al_r, sd, 0, sv, CTL);
    ok = 1;
    for (i = 0; i < CTL; i++)
        ok &= !sd[i] && !sv[i];
    ck("  BLEND 0: nothing sent", ok && !alm_pre_on());
    alm_off();
}

/* ------------------------------------------------------------------ 5 --- */
static void test_pages(void)
{
    int ok;
    uint8_t rec[PROJ_STORE4];
    ui_power_on();
    go_title("ALEMBIC"); frame();
    ok = cur_page()->graph == GR_ALEM && cur_page()->fam == FAM_GLO && cur_page()->id[0] == G_ADRV &&
         cur_page()->id[3] == G_AMOVB;
    turn(EN_K2, 5);
    turn(EN_K3, 2);
    ok &= song.g[G_ADRVB] == 5 && song.g[G_AMOV] == 2;
    ok &= str_eq(GP[G_ADRV].label, "DRIVE") && str_eq(GP[G_AMOV].label, "MOTION") && str_eq(GP[G_ASPC].label, "SPACE") &&
          str_eq(GP[G_AGRN].label, "GRAIN") && str_eq(GP[G_AGRNB].label, "BLEND");
    go_title("ALEMBIC 2"); frame();
    ok &= cur_page()->id[0] == G_ASPC && cur_page()->id[3] == G_AGRNB;
    turn(EN_K4, 9);
    ok &= song.g[G_AGRNB] == 9;
    ck("the pages: ALEMBIC (DRIVE BLEND MOTION BLEND), ALEMBIC 2 (SPACE BLEND GRAIN BLEND), on GLO", ok);
    song.g[G_ADRV] = 3; song.g[G_ADRVB] = 40; song.g[G_AMOV] = 1; song.g[G_AMOVB] = 50;
    song.g[G_ASPC] = 2; song.g[G_ASPCB] = 60; song.g[G_AGRN] = 3; song.g[G_AGRNB] = 70;
    project_save(1);
    alm_off();
    project_load(1);
    ok = song.g[G_ADRV] == 3 && song.g[G_ADRVB] == 40 && song.g[G_AMOV] == 1 && song.g[G_AMOVB] == 50 &&
         song.g[G_ASPC] == 2 && song.g[G_ASPCB] == 60 && song.g[G_AGRN] == 3 && song.g[G_AGRNB] == 70;
    ck("a project keeps ALEMBIC (SKF4, by GSID)", ok);
    {   /* the globals of 2.5 (28): ALEMBIC's ids absent, they load as their defaults (off) */
        int16_t g[G_COUNT];
        uint32_t n;
        for (n = 0; n < G_COUNT; n++)
            g[n] = song.g[n];
        n = gsid_put(rec, g);
        rec[0] = 28;
        for (n = 0; n < G_COUNT; n++)
            g[n] = GP[n].def;
        ok = gsid_get(g, rec, sizeof rec) == 1u + 2u * 28u && g[G_ADRVB] == 0 && g[G_AGRNB] == 0 && g[G_ADRV] == 0 &&
             g[G_DMODE] == song.g[G_DMODE];
        ck("  the globals of 2.5 (28 ids): ALEMBIC off, the rest as stored", ok);
    }
    alm_off();
}

/* ---------------------------------------------- 7: ALEMBIC 3, MEDIUM --- */
/* each medium on two tones (220 Hz left, 330 Hz right), switched on at 0.5 s and off at 2 s: before and after it the mix
 * bit for bit, the stage at rest after (wound down); on: the level within 6 dB, no sample step past the tones' own
 * x1.2 (VINYL: its pops); the page, its knobs, a project keeping it, an older project's globals loading it off */
static void test_medium(void)
{
    static const char *const NM[4] = {"CASST", "TAPE", "VHS", "VINYL"};
    int32_t L[CTL], R[CTL], L0[CTL], R0[CTL];
    uint32_t f, b, i;
    int ok;
    for (f = 0; f < 4u; f++) {
        int32_t step = 0, prev = 0;
        double ein = 0, eout = 0, t = 0;
        int before = 1, after = 1;
        char what[96];
        alm_off();
        song.g[G_AMED] = (int16_t)f;
        for (b = 0; b < 3u * FS / CTL; b++) {
            uint32_t s0 = b * CTL;
            if (s0 >= FS / 2u && s0 < FS / 2u + CTL)
                song.g[G_AMEDB] = 100;
            if (s0 >= 2u * FS && s0 < 2u * FS + CTL)
                song.g[G_AMEDB] = 0;
            for (i = 0; i < CTL; i++, t += 1.0) {
                L0[i] = L[i] = (int32_t)(20000.0 * sin(2.0 * M_PI * 220.0 * t / FS));
                R0[i] = R[i] = (int32_t)(14000.0 * sin(2.0 * M_PI * 330.0 * t / FS));
            }
            alm_medium(L, R, CTL);
            for (i = 0; i < CTL; i++) {
                int32_t d = L[i] - prev;
                prev = L[i];
                if (b && abs(d) > step)
                    step = abs(d);
                if (s0 < FS / 2u && (L[i] != L0[i] || R[i] != R0[i]))
                    before = 0;
                if (s0 > FS * 29u / 10u && (L[i] != L0[i] || R[i] != R0[i]))
                    after = 0;
                if (s0 > FS && s0 < 2u * FS) {
                    ein += (double)L0[i] * L0[i];
                    eout += (double)L[i] * L[i];
                }
            }
        }
        snprintf(what, sizeof what, "MEDIUM %s: off bit for bit, on and off with no click, level %+.1f dB, at rest after",
                 NM[f], 10.0 * log10(eout / ein));
        ck(what, before && after && !amd.on && fabs(10.0 * log10(eout / ein)) < 6.0 &&
                     step < (f == 3u ? 6000 : 20000.0 * 2.0 * M_PI * 220.0 / FS * 1.2));
    }
    alm_off();
    ui_power_on();
    go_title("ALEMBIC 3"); frame();
    ok = cur_page()->graph == GR_ALEM3 && cur_page()->fam == FAM_GLO && cur_page()->id[0] == G_AMED &&
         cur_page()->id[1] == G_AMEDB && cur_page()->id[2] == G_AWEAR && cur_page()->id[3] == G_AWOW;
    turn(EN_K1, 2);
    turn(EN_K2, 30);
    ok &= song.g[G_AMED] == 2 && song.g[G_AMEDB] == 30;
    ok &= str_eq(GP[G_AMED].label, "MEDIUM") && str_eq(GP[G_AWEAR].label, "WEAR") && str_eq(GP[G_AWOW].label, "WOW") &&
          GP[G_AMEDB].def == 0 && GP[G_AWEAR].def == 64 && GP[G_AWOW].def == 64;
    ck("the page ALEMBIC 3: MEDIUM BLEND WEAR WOW, on GLO, off by default", ok);
    song.g[G_AMED] = 3; song.g[G_AMEDB] = 77; song.g[G_AWEAR] = 99; song.g[G_AWOW] = 12;
    project_save(1);
    alm_off();
    project_load(1);
    ck("a project keeps MEDIUM, BLEND, WEAR, WOW",
       song.g[G_AMED] == 3 && song.g[G_AMEDB] == 77 && song.g[G_AWEAR] == 99 && song.g[G_AWOW] == 12);
    {   /* the globals of before (36: ALEMBIC 1 and 2): MEDIUM off, WEAR and WOW their defaults */
        uint8_t rec[PROJ_STORE4];
        int16_t g[G_COUNT];
        uint32_t n;
        for (n = 0; n < G_COUNT; n++)
            g[n] = song.g[n];
        gsid_put(rec, g);
        rec[0] = 36;
        for (n = 0; n < G_COUNT; n++)
            g[n] = GP[n].def;
        ck("  an older project (36 globals): MEDIUM off",
           gsid_get(g, rec, sizeof rec) == 1u + 2u * 36u && g[G_AMEDB] == 0 && g[G_AWEAR] == 64 && g[G_AWOW] == 64);
    }
    alm_off();
}

/* ------------------------------------------------------------------ 6 --- */
static double ns_of(int which)                          /* 0: ALEMBIC, every stage on; 1: six INSERTs */
{
    static int32_t b[NTRK][CTL];
    uint32_t i, k, nb = 2u * NB;
    uint64_t t0;
    alm_off();
    memset(ins, 0, sizeof ins);
    host_tracks_init();
    song.g[G_ADRV] = 2; song.g[G_ADRVB] = 100;          /* FOLD */
    song.g[G_AMOV] = 0; song.g[G_AMOVB] = 100;          /* PHASR (its four stages) */
    song.g[G_ASPC] = 2; song.g[G_ASPCB] = 100;          /* BOTH */
    song.g[G_AGRN] = 0; song.g[G_AGRNB] = 100;          /* WOW */
    for (k = 0; k < NTRK; k++) {
        trk[k].p[P_ITYPE] = (int16_t)(k % 3u == 0u ? IT_FOLD : k % 3u == 1u ? IT_PHASER : IT_FILTER);
        trk[k].p[P_IA] = trk[k].p[P_IB] = trk[k].p[P_IC] = 64;
        trk[k].p[P_IMIX] = 100;
    }
    loud(al_l, al_r, 100000.0);
    for (k = 0; k < NTRK; k++)
        memcpy(b[k], al_l, sizeof al_l);
    t0 = now_ns();
    for (i = 0; i < nb; i++) {
        if (which)
            for (k = 0; k < NTRK; k++)
                track_insert(&trk[k], b[k], CTL);
        else
            run_blk(0, al_l, al_r);
    }
    return (double)(now_ns() - t0) / (nb * CTL);
}
static void test_cost(void)
{
    double a = 1e9, s = 1e9;
    uint32_t r;
    char what[200];
    for (r = 0; r < 3u; r++) {                           /* (the best of three: the host's noise) */
        double x = ns_of(0), y = ns_of(1);
        a = x < a ? x : a;
        s = y < s ? y : s;
    }
    snprintf(what, sizeof what, "cost: ALEMBIC, all four stages (FOLD, PHASR, BOTH, WOW) %.1f ns / sample, six INSERTs %.1f "
             "(x%.2f, limit x1.6)", a, s, a / s);
    ck(what, a <= s * 1.6);
    alm_off();
    host_tracks_init();
}

int main(void)
{
    test_identity();
    test_bounded();
    test_glide();
    test_space();
    test_pages();
    test_medium();
    test_cost();
    if (bad) {
        printf("alembic: %d FAILED\n", bad);
        return 1;
    }
    printf("alembic tests passed\n");
    return 0;
}
