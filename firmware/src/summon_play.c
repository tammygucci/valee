/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) SUMMON's being plays its own part (eng_summon.c, ui_summon.c). A key on SUMMON's EDIT page starts it
 * (sm_play_begin, asked by the UI through sm.rq): from then, while the song plays, its track plays by itself, from
 * the octave of the key pressed, and only ever the notes of the chord the song is playing that moment: the chord of
 * the other tracks (follow.c fol_src: each track's latest step or held keys; the one with the most notes, a kit never,
 * its own never), its root found as chord follow finds it. When the chord changes, what it holds that is not in the new
 * one lets go and the new root sounds. Before the song has played a chord: the key's triad (the first track with a
 * SCALE: its ROOT and SCALE; none: the key pressed, minor). It grows with the ceremony (sm.prog): the chord's root
 * first, then its fifth (or the chord's next note), another of its notes, a slow line on its notes, sparks high up. Stopped, it lets go and waits; the next PLAY goes on. The key
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
    sm.hpc = 0;
    sm.line = 2;
    for (i = 0; i < 12u; i++)                           /* (the song it forms over: from now) */
        sm.hist[i] = 0;
    sm.hn_lo = sm.hn_hi = 0;
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
/* the k-th note of the chord from note lo up (k from 0) */
static uint32_t sm_ct(uint32_t lo, uint32_t k)
{
    uint32_t n = lo;
    for (;; n++)
        if (((sm.hpc >> (n % 12u)) & 1u) && !k--)
            return n < 108u ? n : n - 12u * ((n - 96u) / 12u + 1u);
}
/* the chord's root at or above lo */
static uint32_t sm_rt(uint32_t lo) { return lo + (sm.hroot + 12u - lo % 12u) % 12u; }
/* what the other tracks sound this moment (their pitch classes; kits never) */
static uint32_t sm_others(void)
{
    uint32_t k, i, pc = 0;
    for (k = 0; k < NTRK; k++)
        if (k != sm.ptrk && !chord_kit(&trk[k]))
            for (i = 0; i < NVOICE; i++) {
                const voice_t *v = &trk[k].v[i];
                if (v->active && v->gate && v->stage && v->stage < 3u)
                    pc |= 1u << (v->note % 12u);
            }
    return pc;
}
/* note rubs against them: a semitone or a tritone from one of theirs it is not itself */
static int sm_rubs(uint32_t note, uint32_t o)
{
    uint32_t p = note % 12u, near = (1u << ((p + 1u) % 12u)) | (1u << ((p + 11u) % 12u)) | (1u << ((p + 6u) % 12u));
    return !((o >> p) & 1u) && (o & near);
}
static void sm_hit(uint32_t note, uint32_t vel, uint32_t len)
{
    track_t *t = &trk[sm.ptrk % NTRK];
    uint32_t i, j = 0, o = sm_others(), k;
    for (k = 0; k < 3u && sm_rubs(note, o); k++)        /* (it would rub: the chord's next note up, or none) */
        note = sm_ct(note + 1u, 0);
    if (sm_rubs(note, o))
        return;
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
/* the song's chord this moment: the other tracks' (the fullest), else the one before, else the key's triad */
static void sm_harmony(void)
{
    uint32_t i, best = NTRK, bn = 1, pc;
    for (i = 0; i < NTRK; i++) {
        uint32_t n;
        if (i == sm.ptrk || chord_kit(&trk[i]) || !fol_src[i].pc)
            continue;
        n = (uint32_t)__builtin_popcount(fol_src[i].pc);
        if (n > bn) {
            bn = n;
            best = i;
        }
    }
    if (best < NTRK) {
        pc = fol_src[best].pc;
        sm.hroot = (uint8_t)fol_root(pc, fol_src[best].lo);
    } else if (sm.hpc)
        return;
    else {                                              /* (no chord yet: the key's triad, its 1st 3rd 5th) */
        pc = 0;
        for (i = 0; i < 5u; i += 2u)
            pc |= 1u << (sm_deg((int32_t)i) % 12u);
        sm.hroot = sm.root;
    }
    {   /* (only the chord's steady core: its root, its third, its fifth; never a 7th, 9th or tritone of it) */
        uint32_t r = sm.hroot, core = 1u << r, m3 = 1u << ((r + 3u) % 12u), M3 = 1u << ((r + 4u) % 12u), p5 = 1u << ((r + 7u) % 12u);
        core |= pc & M3 ? M3 : pc & m3;
        core |= pc & p5;
        pc = core;
    }
    if (pc != sm.hpc) {                                 /* (a new chord: what is not in it lets go) */
        track_t *t = &trk[sm.ptrk % NTRK];
        for (i = 0; i < 6u; i++)
            if (sm.hl[i] && !((pc >> (sm.hn[i] % 12u)) & 1u)) {
                trk_note_off(t, sm.hn[i]);
                sm.hl[i] = 0;
            }
        sm.hpc = (uint16_t)pc;
        sm.step = sm.step / 32u * 32u + 32u;            /* (its root now: the two bars start again) */
    }
}
/* every sequencer block: n samples */
static __attribute__((noinline)) void sm_play(uint32_t n)   /* (out of events_block: its budget, tests/target_budget.txt) */
{
    track_t *t;
    uint32_t q, i, s, g, v, r, rq = sm.rq, nn, root, fresh;
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
    if (sm.hl[0] | sm.hl[1] | sm.hl[2] | sm.hl[3] | sm.hl[4] | sm.hl[5]) {   /* (every block: what it holds that now
                                                         * rubs against the others, a passing note, a new chord: let go) */
        uint32_t o = sm_others();
        for (i = 0; i < 6u; i++)
            if (sm.hl[i] && sm_rubs(sm.hn[i], o)) {
                trk_note_off(t, sm.hn[i]);
                sm.hl[i] = 0;
            }
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
    sm_harmony();
    if (sm.cer) {                                       /* (while it forms: what the song sounds, for its shape) */
        uint32_t k, o = 0;
        for (k = 0; k < NTRK; k++)
            if (k != sm.ptrk && !chord_kit(&trk[k]))
                for (i = 0; i < NVOICE; i++) {
                    const voice_t *w = &trk[k].v[i];
                    if (w->active && w->gate && w->stage && w->stage < 3u) {
                        o |= 1u << (w->note % 12u);
                        if (w->note < 60u)
                            sm.hn_lo++;
                        else
                            sm.hn_hi++;
                    }
                }
        for (i = 0; i < 12u; i++)
            if (((o >> i) & 1u) && sm.hist[i] < 65535u)
                sm.hist[i]++;
    }
    s = sm.step++;
    g = sm.cer ? sm.prog * 64u / (SM_CANDLES * SM_STEP_MS) : 64u;   /* how grown: 0..64 */
    v = 60u + g;                                        /* (louder as it grows) */
    r = sm_rand();
    nn = (uint32_t)__builtin_popcount(sm.hpc);
    root = sm_rt(sm.base);
    fresh = s % 32u == 0u;
    if (fresh) {                                        /* every two bars, or a new chord: its root, held */
        sm_hit(root, v, 31u);
        if (g >= 10u && nn > 1u)                        /* its fifth if the chord has one, else its next note */
            sm_hit((sm.hpc >> ((sm.hroot + 7u) % 12u)) & 1u ? root + 7u : sm_ct(root + 1u, 0), v - 10u, 31u);
    }
    if (g >= 22u && s % 32u == 8u && nn > 1u)           /* another of its notes, an octave up, later in the bar */
        sm_hit(sm_ct(root + 12u, 1u + (r & 1u) % (nn > 2u ? 2u : 1u)), v - 15u, 22u);
    if (g >= 32u && s % 4u == 0u && r % 64u < 6u + (g - 32u)) {   /* a slow line on its notes, more as it grows */
        sm.line = (uint8_t)clamp((int32_t)sm.line + (int32_t)((r >> 8) % 5u) - 2, 0, (int32_t)nn * 2 - 1);
        sm_hit(sm_ct(root + 12u, sm.line), v - 5u, 3u + (r >> 12) % 6u);
    }
    if (g >= 48u && s % 2u == 1u && r % 64u < (g - 46u))  /* sparks high up, at the end */
        sm_hit(sm_ct(root + 24u, (r >> 16) % (nn ? nn : 1u)), v - 25u, 1u + (r >> 20) % 2u);
}
