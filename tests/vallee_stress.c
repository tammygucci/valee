/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) A stress test of what Vallée adds, on the UI test bench (tests/ui_test.c: the real UI, sequencer and mixer
 * sources against the stub display, buttons and knobs). Built with ASan + UBSan by tests/run_tests.sh; any memory
 * error or undefined behaviour stops it.
 *   1. GEN fuzz: N rounds of random knobs (as turned, and raw values past their ends), a write, the result checked
 *      (every step well formed, notes 0..127, chords at most 4, the tempo in range, every engine one that is built,
 *      each track's parameters inside their ranges), sometimes an undo (OCT-), sometimes played for a bar through
 *      the mixer (no NaN, no runaway level), sometimes drawn (the GEN page, HOME with the creature box).
 *   2. Monkey: M frames of random presses, holds, turns and keys over the whole UI (HOME, GEN, the creature box,
 *      every page a button opens, the menus), the sequencer playing at times, the audio rendered alongside.
 *   3. The creatures: every engine x every preset index up to 255 and 2000 hashed names: drawn, packed, in bounds.
 * vallee_stress [GEN_ROUNDS [MONKEY_FRAMES [SEED]]] (default 4000 20000 1) */
#define UI_TEST_MAIN ui_test_main
#include "ui_test.c"

#include <signal.h>
#include <unistd.h>
static uint64_t vs_rng = 0x9E3779B97F4A7C15ull;
static volatile uint32_t vs_ev_now;
/* an event taking more than 20 s (a frame never ending): where it was, then stop (a hang is a failure) */
static void vs_stuck(int sig)
{
    (void)sig;
    printf("vallee_stress: FAIL an event never ended (event %u): home %u menu %u menu_sel %u page %u confirm %u\n",
           vs_ev_now, ui.home, ui.menu, ui.menu_sel, ui.page, ui.confirm);
    fflush(stdout);
    _exit(3);
}
static uint32_t vs_r(void)
{
    vs_rng ^= vs_rng << 13;
    vs_rng ^= vs_rng >> 7;
    vs_rng ^= vs_rng << 17;
    return (uint32_t)(vs_rng >> 11);
}
static uint32_t vs_n(uint32_t n) { return n ? vs_r() % n : 0u; }

static uint32_t vs_errors;
static uint32_t vs_st_home, vs_st_world, vs_st_gen, vs_st_box, vs_st_menu, vs_st_play, vs_st_dialog, vs_st_writes, vs_st_pages;
static uint32_t vs_seen_page[64];
static void vs_fail(const char *what, uint32_t round, uint32_t a, uint32_t b)
{
    if (vs_errors++ < 20u)
        printf("vallee_stress: FAIL %s (round %u: %u %u)\n", what, round, a, b);
}

/* every track's steps and sound: well formed */
static void vs_check_tracks(uint32_t round)
{
    uint32_t k, i, j;
    for (k = 0; k < NTRK; k++) {
        const track_t *t = &trk[k];
        if (!eng_ok(t->eng_req % 256u) || t->eng_req >= NENGINES)
            vs_fail("engine not built", round, k, t->eng_req);
        if (t->p[P_SLEN] < 1 || t->p[P_SLEN] > (int16_t)NSTEP)
            vs_fail("LEN out of range", round, k, (uint32_t)t->p[P_SLEN]);
        for (i = 0; i < NSTEP; i++) {
            const step_t *s = &t->step[i];
            if (s->time > 2u)
                vs_fail("step time", round, k, i);
            if (s->n > 4u)
                vs_fail("more than 4 notes", round, k, i);
            for (j = 0; j < s->n && j < 4u; j++)
                if (s->note[j] > 127u)
                    vs_fail("note above 127", round, k, s->note[j]);
            if (s->vel > 127u)
                vs_fail("velocity above 127", round, k, s->vel);
        }
        for (i = 0; i < P_COUNT; i++) {
            const param_desc_t *d = track_desc(t, i);
            if (d && d->max > d->min && (t->p[i] < d->min || t->p[i] > d->max))
                vs_fail("parameter out of its range", round, k * 1000u + i, (uint32_t)(int32_t)t->p[i]);
        }
    }
    if (song.g[G_BPM] < GP[G_BPM].min || song.g[G_BPM] > GP[G_BPM].max)
        vs_fail("tempo out of range", round, (uint32_t)song.g[G_BPM], 0);
    for (i = 0; i < G_COUNT; i++)                       /* (every global inside its range: AKASHA's delay TIME was 100,
                                                         * past N_DTIME's end, and a reload changed the song) */
        if (GP[i].max > GP[i].min && (song.g[i] < GP[i].min || song.g[i] > GP[i].max))
            vs_fail("global out of its range", round, i, (uint32_t)(int32_t)song.g[i]);
}

/* a bar through the mixer: finite, bounded */
static void vs_play(uint32_t round, uint32_t blocks)
{
    int32_t o[2u * CTL];
    uint32_t b, i;
    int32_t peak = 0;
    seq_start();
    for (b = 0; b < blocks; b++) {
        mix_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++) {
            int32_t v = o[i] < 0 ? -o[i] : o[i];
            if (v > peak)
                peak = v;
        }
    }
    song.playing = 0;
    if (peak > (1 << 24))
        vs_fail("mixer level runaway", round, (uint32_t)peak, 0);
}

/* ((Vallée) CHORDS, as issue #1 asks: the keys D G C on the CHORUS give ii V I, a bar each, a part of 3 bars; the white
 * keys the mode's chords (C major: Dm G C, B dim; C minor: D dim), a black key its root's; END shortens the part */
static void vs_own_chords(void)
{
    gplan_t p;
    uint32_t i;
    static const uint8_t KEY_D = 9, KEY_G = 2, KEY_C = 7, KEY_BB = 5;   /* (key 0: F) */
    ui_power_on();
    gen_reset();
    memset(gch, 0, sizeof gch);
    gen.v[GK_PART] = GP_CHORUS;
    gen.v[GK_KEY] = 1;                                      /* (C) */
    gen.v[GK_BAR] = 0;
    gen_chord_key(KEY_D);
    gen_chord_key(KEY_G);
    gen_chord_key(KEY_C);
    if (!gc_mine(GP_CHORUS) || gc_n(GP_CHORUS) != 3u || gen.v[GK_BAR] != 3u)
        vs_fail("CHORDS: three keys, three bars", 0, gc_n(GP_CHORUS), gen.v[GK_BAR]);
    gen_plan(&p);
    p.scale = 1;                                            /* (as a C major style plans it) */
    p.ext = 0;
    for (i = 0; i < 3u; i++)
        gc_chord(&p, GP_CHORUS, i);
    if (p.n != 3u || p.bars != 3u || p.ch[0][0] != 2u || p.ch[0][1] != Q_MIN || p.ch[1][0] != 7u ||
        p.ch[1][1] != Q_MAJ || p.ch[2][0] != 0u || p.ch[2][1] != Q_MAJ)
        vs_fail("CHORDS: ii V I in C", 0, p.ch[0][0] * 100u + p.ch[0][1], p.ch[1][0] * 100u + p.ch[2][0]);
    for (i = 0; i < 64u; i++)                               /* (a bar each, round again after the third) */
        if (g_chord_at(&p, i) != (i % 48u) / 16u)
            vs_fail("CHORDS: the chord at a step", 0, i, g_chord_at(&p, i));
    gch[GP_CHORUS].r[3] = 7;                                /* (B: vii, dim in major) */
    gc_chord(&p, GP_CHORUS, 3);
    if (p.ch[3][0] != 11u || p.ch[3][1] != Q_DIM)
        vs_fail("CHORDS: B dim in C major", 0, p.ch[3][0], p.ch[3][1]);
    p.scale = 2;                                            /* (C minor: D the dim one) */
    gc_chord(&p, GP_CHORUS, 0);
    if (p.ch[0][0] != 2u || p.ch[0][1] != Q_DIM)
        vs_fail("CHORDS: D dim in C minor", 0, p.ch[0][0], p.ch[0][1]);
    gen.v[GK_BAR] = 0;
    gen_chord_key(KEY_BB);                                  /* (bar 1 again: a new progression; Bb, the bVII) */
    p.scale = 1;
    gc_chord(&p, GP_CHORUS, 0);
    if (gc_n(GP_CHORUS) != 1u || p.ch[0][0] != 10u || p.ch[0][1] != Q_MAJ)
        vs_fail("CHORDS: a black key, a new progression", 0, gc_n(GP_CHORUS), p.ch[0][0]);
    gen.v[GK_PART] = GP_CHORUS;
    gen.v[GK_BAR] = 0;
    gen_chord_key(KEY_D);
    gen_chord_key(KEY_G);
    gen_chord_key(KEY_C);
    gen_write();                                            /* (written: the tracks 3 bars long) */
    for (i = 0; i < GR_NROLE; i++)
        if (gen_role_on(i) && i != GR_PERC && trk[i].p[P_SLEN] != 48)
            vs_fail("CHORDS: a 3-bar part's tracks", 0, i, (uint32_t)trk[i].p[P_SLEN]);
    gen.win = GC_WIN;                                       /* (CHORDS window: AUTO again, the chords kept) */
    gen_knob(0, -1);
    if (gc_mine(GP_CHORUS) || gc_n(GP_CHORUS) != 3u)
        vs_fail("CHORDS: AUTO keeps them", 0, gc_mine(GP_CHORUS), gc_n(GP_CHORUS));
    {                                                       /* (a SONG of 3-bar verses, 2-bar choruses: every track's
                                                             * parts the same length, so they change together) */
        uint32_t e, k;
        gen.v[GK_PART] = GP_VERSE;
        gen.v[GK_BAR] = 0;
        gen_chord_key(KEY_C);
        gen_chord_key(KEY_G);
        gen_chord_key(KEY_D);
        gen.v[GK_PART] = GP_CHORUS;
        gen.v[GK_BAR] = 0;
        gen_chord_key(KEY_G);
        gen_chord_key(KEY_C);
        gen.v[GK_PART] = GP_SONG;
        gen.made = 0;
        gen_oct();
        for (e = 0; e < NENT; e++)
            for (k = 0; k < GR_NROLE; k++)
                if (k != GR_PERC && k != GR_LEAD && gen_role_on(k) && ent_on(GR_BASS, e) &&
                    ent_len48(k, e) * ent_reps(k, e) != ent_len48(GR_BASS, e) * ent_reps(GR_BASS, e))
                    vs_fail("CHORDS: a SONG's parts out of step", 0, e, k);
        vs_play(0, 400);
        gen_back();
    }
    memset(gch, 0, sizeof gch);
    gen.win = 0;
    printf("vallee_stress: own chords (issue #1): D G C on CHORUS is Dm G C, 3 bars; dim, bVII, AUTO: %s\n",
           vs_errors ? "FAIL" : "ok");
}

/* ((Vallée) parts one by one into a song, as asked on the FM-1: a new project, VERSE written, CHORUS written: two
 * sections (each track's list VERSE CHORUS, each its own patterns, the verse not written over by the chorus, nor by the
 * autosave); VERSE again: still two, the same patterns; INTRO: in front; OCT-: the lists and the pool as before; a song
 * of the user's own lists: left alone; PART SONG after: the whole arrangement */
static uint32_t vs_ents(uint32_t k)
{
    uint32_t e, n = 0;
    for (e = 0; e < NENT; e++)
        n += lists.ent[k][e][0] != 0;
    return n;
}
static void vs_parts_song(void)
{
    static step_t verse[NSTEP];
    uint32_t k, free0, vid;
    ui_power_on();
    project_new();
    gen_reset();
    memset(gch, 0, sizeof gch);
    gen.seed = 7;
    free0 = gen_pool_free();
    gen.v[GK_PART] = GP_VERSE;
    gen.made = 0;
    gen_oct();
    memcpy(verse, trk[GR_BASS].step, sizeof verse);
    vid = gen_sid[GR_BASS][GP_VERSE - GP_INTRO];
    gen.v[GK_PART] = GP_CHORUS;
    gen.dirty = 1;
    gen_oct();
    (void)keep_all();                                       /* (the autosave's round: the tracks kept where they are) */
    for (k = 0; k < GR_NROLE; k++)
        if (vs_ents(k) != 2u)
            vs_fail("parts: VERSE then CHORUS, two sections", 0, k, vs_ents(k));
    if (lists.ent[GR_BASS][0][0] != vid + 1u || gen_sid[GR_BASS][GP_CHORUS - GP_INTRO] == GEN_NOID ||
        lists.ent[GR_BASS][1][0] != gen_sid[GR_BASS][GP_CHORUS - GP_INTRO] + 1u)
        vs_fail("parts: the verse's and the chorus's patterns in order", 0, lists.ent[GR_BASS][0][0], vid);
    (void)gen_load(GR_BASS, vid);
    if (memcmp(trk[GR_BASS].step, verse, sizeof verse))
        vs_fail("parts: the verse written over", 0, vid, 0);
    gen.v[GK_PART] = GP_VERSE;                              /* (VERSE again: the same two, the same pattern) */
    gen.dirty = 1;
    gen.seed = 8;
    gen_oct();
    if (vs_ents(GR_BASS) != 2u || gen_sid[GR_BASS][GP_VERSE - GP_INTRO] != vid)
        vs_fail("parts: VERSE again, in place", 0, vs_ents(GR_BASS), gen_sid[GR_BASS][GP_VERSE - GP_INTRO]);
    gen.v[GK_PART] = GP_INTRO;                              /* (INTRO: in front) */
    gen.dirty = 1;
    gen_oct();
    if (vs_ents(GR_BASS) != 3u || lists.ent[GR_BASS][0][0] != gen_sid[GR_BASS][0] + 1u ||
        lists.ent[GR_BASS][1][0] != vid + 1u)
        vs_fail("parts: INTRO in front", 0, vs_ents(GR_BASS), lists.ent[GR_BASS][0][0]);
    vs_play(0, 300);
    if (!gen_back() || vs_ents(GR_BASS) > 1u || gen_pool_free() != free0)
        vs_fail("parts: OCT- the lists and the pool as before", 0, vs_ents(GR_BASS), gen_pool_free());
    project_new();                                          /* (a song of the user's: two entries of their own) */
    gen.made = 0;
    lists.ent[GR_BASS][0][0] = 1;
    lists.ent[GR_BASS][1][0] = 2;
    gen.v[GK_PART] = GP_VERSE;
    gen_oct();
    if (lists.ent[GR_BASS][0][0] != 1u || lists.ent[GR_BASS][1][0] != 2u || vs_ents(GR_KEYS) > 1u)
        vs_fail("parts: the user's own song left alone", 0, lists.ent[GR_BASS][0][0], lists.ent[GR_BASS][1][0]);
    gen_back();
    project_new();                                          /* (playing, SONG off: written live, on playing, the */
    gen.made = 0;                                           /* patterns waiting; stopped: kept, two sections) */
    gen.v[GK_PART] = GP_VERSE;
    seq_start();
    gen_oct();
    if (!song.playing)
        vs_fail("parts: playing, SONG off: still playing", 0, 0, 0);
    gen.v[GK_PART] = GP_CHORUS;
    gen.dirty = 1;
    gen_oct();
    song.playing = 0;
    if (keep_all() || vs_ents(GR_BASS) != 2u || !pool_used(gen_sid[GR_BASS][GP_CHORUS - GP_INTRO]))
        vs_fail("parts: playing, then stopped: two sections kept", 0, vs_ents(GR_BASS), 0);
    gen_back();
    project_new();                                          /* (parts, then PART SONG: the whole arrangement) */
    gen.made = 0;
    gen.v[GK_PART] = GP_CHORUS;
    gen_oct();
    gen.v[GK_PART] = GP_SONG;
    gen.dirty = 1;
    gen_oct();
    if (vs_ents(GR_BASS) != 8u)
        vs_fail("parts: then PART SONG, its arrangement", 0, vs_ents(GR_BASS), 0);
    gen_back();
    printf("vallee_stress: parts one by one into a song (VERSE, CHORUS, again, INTRO, OCT-, the user's own, SONG): %s\n",
           vs_errors ? "FAIL" : "ok");
}

static void vs_gen_fuzz(uint32_t rounds)
{
    uint32_t r, k;
    ui_power_on();
    gen_reset();
    for (r = 0; r < rounds; r++) {
        uint32_t mode = vs_n(10);
        if (mode < 7u) {                                    /* as turned: inside the knobs' ends */
            for (k = 0; k < GK_N; k++)
                if (k != GK_SEED)
                    gen.v[k] = (uint8_t)vs_n((uint32_t)GEN_MAX[k] + 1u);
        } else {                                            /* raw: past the ends (a stale or corrupted state) */
            for (k = 0; k < GK_N; k++)
                gen.v[k] = (uint8_t)vs_r();
        }
        gen.seed = (uint16_t)(vs_n(10) == 0u ? vs_r() : 1u + vs_n(999));
        if (vs_n(2) == 0u) {                                /* ((Vallée) CHORDS: a part's own, as keys and TYPE set */
            uint32_t pt = vs_n(GC_NPART), i;               /* them, or raw bytes (a stale state) */
            gch[pt].on = (uint8_t)vs_n(2);
            for (i = 0; i < 4u; i++) {
                gch[pt].r[i] = (uint8_t)(mode < 7u ? (vs_n(4) ? 1u + vs_n(19) : 0u) : vs_r());
                gch[pt].q[i] = (uint8_t)(mode < 7u ? (vs_n(2) ? 0u : GC_Q[vs_n(GC_NT - 1u)] + 1u) : vs_r());
            }
        }
        gen_write();
        vs_check_tracks(r);
        {                                                   /* MINE: a bar a chord, the part as long; chords well formed */
            gplan_t pl;
            uint32_t part = gen.v[GK_PART] % GP_N, i;
            gen_plan(&pl);
            if (gc_mine(part) && (pl.n != gc_n(part) || pl.bars != pl.n || !pl.n))
                vs_fail("own chords: not a bar each", r, pl.n, pl.bars);
            if (gc_mine(part) && part != GP_SONG && gen_role_on(GR_BASS) && trk[GR_BASS].p[P_SLEN] != (int16_t)(pl.bars * 16u))
                vs_fail("own chords: the part's length", r, pl.bars, (uint32_t)trk[GR_BASS].p[P_SLEN]);
            for (i = 0; i < pl.n && i < 8u; i++)
                if (pl.ch[i][0] > 11u || pl.ch[i][1] >= Q_N)
                    vs_fail("chord out of range", r, pl.ch[i][0], pl.ch[i][1]);
        }
        if (vs_n(4) == 0u) {                                /* OCT-: put back */
            gen_back();
            vs_check_tracks(r);
        }
        if (vs_n(16) == 0u) {                               /* (PART SONG: the parts as patterns, the lists) */
            gen.v[GK_PART] = GP_SONG;
            gen_oct();
            vs_check_tracks(r);
            if (vs_n(2) == 0u)
                vs_play(r, 40u + vs_n(200));
            if (vs_n(2) == 0u) {
                gen_back();
                vs_check_tracks(r);
            }
        }
        if (vs_n(6) == 0u)
            vs_play(r, 40u + vs_n(200));
        if (vs_n(8) == 0u) {                                /* drawn: GEN's windows, HOME with the creature box */
            go_page(GR_GEN);
            gen.win = (uint8_t)vs_n(GEN_NWIN);
            ui.force = 1;
            frame();
            go_home();
            ui.beast_t = BEAST_T;
            ui.force = 1;
            frame();
        }
        if (r % 500u == 0u)
            ui_power_on();
    }
}

static void vs_monkey(uint32_t frames_n)
{
    static const uint32_t BTN[] = {B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO, B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC,
                                   B_OCTDN, B_OCTUP};
    int32_t o[2u * CTL];
    uint32_t f = 0, ev = 0;
    ui_power_on();
    signal(SIGALRM, vs_stuck);
    while (f < frames_n) {
        uint32_t what = vs_n(100), b = BTN[vs_n(sizeof BTN / sizeof BTN[0])];
        ev++;
        vs_ev_now = ev;
        alarm(20);
        if (ui.menu && ui.menu_sel == MI_PANEL && (b == B_OCTUP || what >= 80u)) {
            b = B_OCTDN;                                    /* (MENU > PANEL SETUP: its own loop waits for every button
                                                             * with fm1_ms, which only frame() moves here: not entered) */
            what = 0;
        }
        if (what < 25u) {                                   /* a button tapped */
            press(b);
            f += 2u;
        } else if (what < 28u) {                            /* held (SAVE held: undo; HOME held: SHIFT, the MENU) */
            hold(b);
            f += 53u;
        } else if (what < 33u) {                            /* SHIFT + a button or a knob */
            btn_down(B_HOME);
            frame();
            if (vs_n(2))
                press(b);
            else
                turn(vs_n(NE), (int32_t)vs_n(5) - 2);
            btn_up(B_HOME);
            frame();
            f += 4u;
        } else if (what < 65u) {                            /* a knob, PRESETS and SELECT the most */
            uint32_t e = vs_n(3) == 0u ? EN_PRESET : vs_n(3) == 0u ? EN_SELECT : vs_n(NE);
            int32_t s = (int32_t)vs_n(9) - 4;
            turn(e, s ? s : 1);
            f++;
        } else if (what < 80u) {                            /* a key, sometimes two */
            uint32_t k = vs_n(27);
            key_down(k);
            if (vs_n(3) == 0u)
                key_down(vs_n(27));
            frame();
            fm1_in.notes = 0;
            keyboard_block();
            frame();
            f += 2u;
        } else if (what < 95u) {                            /* GEN: open, its knobs and windows, OCT+ writes, OCT- back */
            uint32_t n = 1u + vs_n(12), i;
            if (vs_n(3) == 0u || ui.home || cur_page()->graph != GR_GEN) {
                go_page(GR_GEN);
                frame();
            }
            for (i = 0; i < n; i++) {
                uint32_t a = vs_n(10);
                if (a < 6u)
                    turn(EN_K1 + vs_n(4), (int32_t)vs_n(11) - 5);
                else if (a < 7u)
                    turn(EN_SELECT, vs_n(2) ? 1 : -1);
                else if (a < 9u)
                    press(B_OCTUP);
                else
                    press(B_OCTDN);
                f += 2u;
            }
        } else {                                            /* time passes */
            uint32_t n = 1u + vs_n(20), i;
            for (i = 0; i < n; i++)
                frame();
            f += n;
        }
        if (vs_n(3) == 0u) {                                /* the audio alongside */
            uint32_t i;
            for (i = 0; i < 6u; i++)
                mix_block(o, CTL);
        }
        vs_st_home += ui.home && !ui.menu;                  /* (where it went) */
        vs_st_world += ui.home && ui.home_scope && !ui.menu;
        vs_st_gen += !ui.home && !ui.menu && cur_page()->graph == GR_GEN;
        vs_st_box += ui.beast_t > 0u;
        vs_st_menu += ui.menu != 0;
        vs_st_play += song.playing != 0;
        vs_st_dialog += ui.confirm != 0;
        if (!ui.home && !ui.menu && !vs_seen_page[ui.page % 64u]) {
            vs_seen_page[ui.page % 64u] = 1;
            vs_st_pages++;
        }
        if (gen.made && gen.dirty == 0u)
            vs_st_writes++;
        if (ui.confirm && vs_n(2))                          /* a dialog: answered either way */
            press(vs_n(2) ? B_OCTUP : B_OCTDN);
        alarm(0);
        if (ev % 20000u == 0u) {
            printf("vallee_stress:   .. %u events, %u frames\n", ev, f);
            fflush(stdout);
        }
        if (ev % 4000u == 0u) {
            vs_check_tracks(ev);
            go_home();
            frame();
        }
    }
    vs_check_tracks(ev);
}

static void vs_creatures(void)
{
    uint32_t e, n, i;
    uint8_t art[XB_ART_N + 8u];
    for (e = 0; e < 32u; e++)                               /* (track numbers past the last too) */
        for (n = 0; n < 256u; n++) {
            uint8_t px[12][12];
            memset(art, 0xEE, sizeof art);
            xe_beast_px(e, n, px);
            xe_beast_art(e, n, art);
            if (art[0] != 12u || art[1] != 12u || art[2] != 3u)
                vs_fail("creature header", e, n, art[0]);
            for (i = XB_ART_N; i < sizeof art; i++)
                if (art[i] != 0xEEu)
                    vs_fail("creature packed past its end", e, n, i);
        }
    for (i = 0; i < 2000u; i++) {                           /* user names: hashed */
        char name[17];
        uint32_t j, len = 1u + vs_n(16);
        for (j = 0; j < len; j++)
            name[j] = (char)(32 + vs_n(95));
        name[len] = 0;
        xe_beast_art(vs_n(NENGINES), xb_hash(name), art);
    }
}

int main(int argc, char **argv)
{
    uint32_t rounds = argc > 1 ? (uint32_t)atoi(argv[1]) : 4000u, mf = argc > 2 ? (uint32_t)atoi(argv[2]) : 20000u;
    if (argc > 3)
        vs_rng ^= (uint64_t)atoi(argv[3]) * 0x2545F4914F6CDD1Dull;
    vs_creatures();
    printf("vallee_stress: creatures: 32 engines x 256 sounds, 2000 names: %s\n", vs_errors ? "FAILED" : "ok");
    vs_own_chords();
    vs_parts_song();
    vs_gen_fuzz(rounds);
    printf("vallee_stress: GEN fuzz: %u writes (random and raw knobs, undo, played, drawn): %s\n", rounds,
           vs_errors ? "FAILED" : "ok");
    vs_monkey(mf);
    printf("vallee_stress: monkey: %u frames of random buttons, knobs, keys, GEN writes, audio: %s\n", mf,
           vs_errors ? "FAILED" : "ok");
    printf("vallee_stress:   events on HOME %u (THE WORLD %u), GEN %u, creature box up %u, MENU %u, playing %u, dialogs %u; %u pages "
           "visited\n", vs_st_home, vs_st_world, vs_st_gen, vs_st_box, vs_st_menu, vs_st_play, vs_st_dialog, vs_st_pages);
    return vs_errors != 0u;
}
