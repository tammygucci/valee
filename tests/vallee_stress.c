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
        gen_write();
        vs_check_tracks(r);
        if (vs_n(4) == 0u) {                                /* OCT-: put back */
            gen_back();
            vs_check_tracks(r);
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
    for (e = 0; e < 32u; e++)                               /* (engine numbers past the last too) */
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
