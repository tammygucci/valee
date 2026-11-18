/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) SUMMON fits the song: GEN's songs (every style, two seeds), SUMMON on T5 grown full (summon_play.c), the
 * mixer run 12 s each; every block, SUMMON's sounding notes against the other tracks' (kits never). A clash it adds:
 * one of its notes no other track sounds, a semitone or a tritone from one they do. None may happen (a dissonance
 * already in the song, between its own parts, is counted only to compare). On the UI test bench (ui_test.c). */
#define UI_TEST_MAIN ui_test_main
#include "ui_test.c"
static uint32_t pcs_of(uint32_t k, int gated)
{
    uint32_t i, pc = 0;
    for (i = 0; i < NVOICE; i++) {
        const voice_t *v = &trk[k].v[i];
        if (v->active && (!gated || v->gate) && v->stage && v->stage < 3)
            pc |= 1u << (v->note % 12u);
    }
    return pc;
}
static int clash(uint32_t a, uint32_t b)   /* a semitone or a tritone between them */
{
    uint32_t i, j;
    for (i = 0; i < 12; i++) if ((a >> i) & 1u)
        for (j = 0; j < 12; j++) if ((b >> j) & 1u) {
            uint32_t d = (i + 12 - j) % 12;
            if (d == 1 || d == 11 || d == 6) return 1;
        }
    return 0;
}
int main(void)
{
    uint32_t st, sd, b, tot = 0, sb = 0, sclash = 0, base_clash = 0, base_n = 0, out = 0;
    int32_t o[2 * CTL];
    ui_power_on();
    gen_reset();
    for (st = 0; st < GS_N; st++)
        for (sd = 1; sd <= 2; sd++) {
            uint32_t ls = 0, lc = 0, lo = 0, prev_clash = 0;   /* (a note another track starts inside a block: SUMMON
                                                                    lets go at the next, 0.7 ms on; two blocks count) */
            gen.v[GK_STYLE] = (uint8_t)st;
            gen.seed = (uint16_t)(sd * 37);
            gen_write();
            set_engine_of(&trk[4], ENGI_SUMMON);
            trk[4].engine = ENGI_SUMMON;
            for (b = 0; b < NSTEP; b++) trk[4].step[b].n = 0, trk[4].step[b].time = ST_REST, trk[4].step[b].hit = 0;
            sm.trk = 4; sm.rq_note = 57; sm.rq_trk = 4; sm.on = 1; sm.rq = 1; sm.cer = 0;
            seq_start();
            for (b = 0; b < 44100u * 12u / CTL; b++) {
                uint32_t s, others = 0, k, keys = 0, kn = 0;
                mix_block(o, CTL);
                s = pcs_of(4, 1);
                for (k = 0; k < NTRK; k++) {
                    uint32_t p;
                    if (k == 4 || chord_kit(&trk[k])) continue;
                    p = pcs_of(k, 1);
                    others |= p;
                    if ((uint32_t)__builtin_popcount(p) > kn) { kn = __builtin_popcount(p); keys = p; }
                }
                if (!s || !others) continue;
                tot++; ls++;
                {   /* a clash SUMMON adds: one of its notes no other track sounds, against one they do;
                       outside: a note in no other track's sound nor in the chord the fullest track last played */
                    uint32_t ch = 0, cn = 0;
                    for (k = 0; k < NTRK; k++) if (k != 4 && !chord_kit(&trk[k]) && (uint32_t)__builtin_popcount(fol_src[k].pc) > cn) { cn = __builtin_popcount(fol_src[k].pc); ch = fol_src[k].pc; }
                    if (clash(s & ~others, others) && prev_clash++) { sclash++; lc++; if (getenv("SM_VERBOSE")) { printf("  clash st %u sd %u block %u: summon %03x others %03x chord %03x |", st, sd, b, s, others, sm.hpc); for (k = 0; k < NTRK; k++) printf(" T%u %03x", k + 1, pcs_of(k, 1)); printf(" | held"); for (k = 0; k < 6; k++) if (sm.hl[k]) printf(" %u", sm.hn[k]); puts(""); } }
                    if (!clash(s & ~others, others))
                        prev_clash = 0;
                    if (s & ~(others | ch)) { out++; lo++; }
                }
                if (kn >= 2 && others != keys) { base_n++; base_clash += clash(others & ~keys, keys); }
            }
            song.playing = 0;
            sm.on = 0; sm.rq = 2;
            mix_block(o, CTL);
            if (getenv("SM_VERBOSE")) printf("style %2u seed %u: %5u blocks, clash %5.1f %%, outside %5.1f %%\n", st, sd, ls, ls ? 100.0 * lc / ls : 0, ls ? 100.0 * lo / ls : 0);
        }
    printf("ALL: summon clash %.1f %% of %u blocks; notes no other track sounds %.1f %%; song's own (chord vs the rest) %.1f %%\n",
           100.0 * sclash / tot, tot, 100.0 * out / tot, base_n ? 100.0 * base_clash / base_n : 0);
    if (!tot || sclash) {
        printf("summon_fit: FAIL\n");
        return 1;
    }
    printf("summon_fit: SUMMON adds no clash to any song ok\n");
    return 0;
}
