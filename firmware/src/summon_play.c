/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) SUMMON's being plays its own part (eng_summon.c, ui_summon.c). A key on SUMMON's EDIT page starts it
 * (sm_play_begin, asked by the UI through sm.rq): from then, while the song plays, its track plays by itself in the song's key (the first track with
 * a SCALE: its ROOT and SCALE, as GEN writes them; none: the key pressed, minor), from the octave of the key pressed,
 * and grows with the ceremony (sm.prog): a drone on the root first, then its fifth, a third, slow notes of a line
 * over them, then a quicker one and sparks high up. Stopped, it lets go and waits; the next PLAY goes on. The key
 * pressed also seeds its dice, so each key gives another part. Notes go out as a played key's do (trk_note_on), from
 * the sequencer's block (seq.c), on the first core, before the mix. Included by seq.c. */

static uint32_t sm_rand(void)
{
    sm.rng ^= sm.rng << 13;
    sm.rng ^= sm.rng >> 17;
    sm.rng ^= sm.rng << 5;
    return sm.rng;
}
static void sm_let_go(void)
{
    uint32_t i;
    for (i = 0; i < 6u; i++)
        if (sm.hl[i]) {
            trk_note_off(&trk[sm.ptrk % NTRK], sm.hn[i]);
            sm.hl[i] = 0;
        }
}
static void sm_play_begin(uint32_t note, uint32_t tk)
{
    uint32_t i;
    sm_let_go();
    sm.ptrk = (uint8_t)(tk % NTRK);
    sm.key = (uint8_t)note;
    sm.root = (uint8_t)(note % 12u);
    sm.mask = SCALE_MASK[2];                            /* (no song key: the key pressed, minor) */
    for (i = 0; i < NTRK; i++)
        if (trk[i].p[P_SCALE] > 0 && i != sm.ptrk) {     /* (GEN's song, or one in a key: its ROOT and SCALE) */
            sm.root = (uint8_t)(trk[i].p[P_ROOT] % 12);
            sm.mask = (uint16_t)scale_mask(&trk[i]);
            break;
        }
    sm.base = (uint8_t)clamp(36 + (int32_t)((note + 12u - sm.root) / 12u) * 12 - 12 + sm.root, 36, 60);
    sm.rng = (sm.ent ^ note * 2654435761u) | 1u;
    sm.acc = 0;
    sm.step = 0;
}
/* scale degree d (0 the root; may go below 0) from the base */
static uint32_t sm_deg(int32_t d)
{
    int32_t oct = 0, k = 0, n;
    uint32_t deg[12], nd = 0, i;
    for (i = 0; i < 12u; i++)
        if ((sm.mask >> i) & 1u)
            deg[nd++] = i;
    if (!nd)
        return sm.base;
    while (d < 0) {
        d += (int32_t)nd;
        oct--;
    }
    k = d % (int32_t)nd;
    oct += d / (int32_t)nd;
    n = (int32_t)sm.base + oct * 12 + (int32_t)deg[k];
    return (uint32_t)clamp(n, 24, 108);
}
static void sm_hit(uint32_t note, uint32_t vel, uint32_t len)
{
    track_t *t = &trk[sm.ptrk % NTRK];
    uint32_t i, j = 0;
    for (i = 0; i < 6u; i++) {                          /* (the same note again: let it go first) */
        if (sm.hl[i] && sm.hn[i] == note) {
            trk_note_off(t, note);
            sm.hl[i] = 0;
        }
        if (!sm.hl[i] || sm.hl[i] < sm.hl[j])
            j = i;
    }
    for (i = 0; i < 6u && sm.hl[i]; i++)
        ;
    if (i == 6u) {                                      /* (all held: the nearest its end goes) */
        trk_note_off(t, sm.hn[j]);
        i = j;
    }
    sm.hn[i] = (uint8_t)note;
    sm.hl[i] = (uint8_t)(len < 255u ? len : 255u);
    trk_note_on(t, note, vel);
}
/* every sequencer block: n samples */
static void sm_play(uint32_t n)
{
    track_t *t;
    uint32_t q, i, s, g, v, r, rq = sm.rq;
    if (rq) {                                           /* (the UI's ask, here where the notes are played) */
        if (rq == 1u)
            sm_play_begin(sm.rq_note, sm.rq_trk);
        else
            sm_let_go();
        sm.rq = 0;
    }
    if (!sm.on)
        return;
    t = &trk[sm.ptrk % NTRK];
    if (t->eng_req % NENGINES != ENGI_SUMMON) {         /* (another sound on its track: the part ends) */
        sm_let_go();
        sm.on = 0;
        return;
    }
    if (!song.playing) {
        sm_let_go();
        sm.acc = 0;
        return;
    }
    q = beat_samples() / 4u;                            /* a 16th */
    if (sm.acc > n) {
        sm.acc -= n;
        return;
    }
    sm.acc = q - (n - sm.acc) % (q ? q : 1u);
    for (i = 0; i < 6u; i++)                            /* (what it holds: a 16th nearer its end) */
        if (sm.hl[i] && !--sm.hl[i])
            trk_note_off(t, sm.hn[i]);
    s = sm.step++;
    g = sm.cer ? sm.prog * 64u / (SM_CANDLES * SM_STEP_MS) : 64u;   /* how grown: 0..64 */
    v = 60u + g;                                        /* (louder as it grows) */
    r = sm_rand();
    if (s % 32u == 0u) {                                /* every two bars: the root, held */
        sm_hit(sm_deg(0), v, 31u);
        if (g >= 10u)
            sm_hit(sm_deg(4), v - 10u, 31u);            /* its fifth (the scale's 5th degree) */
    }
    if (g >= 22u && s % 32u == 8u)                      /* a third over them, later in the bar */
        sm_hit(sm_deg(2 + 7 * (int32_t)(r & 1u)), v - 15u, 22u);
    if (g >= 32u && s % 4u == 0u && r % 64u < 6u + (g - 32u)) {   /* a slow line, more notes as it grows */
        static int32_t at = 7;
        at = clamp(at + (int32_t)((r >> 8) % 5u) - 2, 4, 13);
        sm_hit(sm_deg(at), v - 5u, 3u + (r >> 12) % 6u);
    }
    if (g >= 48u && s % 2u == 1u && r % 64u < (g - 46u))  /* sparks high up, at the end */
        sm_hit(sm_deg(14 + (int32_t)((r >> 16) % 5u)), v - 25u, 1u + (r >> 20) % 2u);
}
