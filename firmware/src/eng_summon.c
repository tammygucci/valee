/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) SUMMON: SUMMONER on THEME XENO, AURA on TEEN (the screen: ui_summon.c). An ambient engine fed by what the
 * FM-1 picks up of the room: the eight SIGNALS, slow values that wander with the jitter between the audio blocks and
 * the timer, the battery reading's noise, and every key, button and MIDI note that comes in (the idea of SWARM, in
 * JIANT FM's EXT engine: nearby radios as voices; the FM-1 under Vallée has no radio, so the room is its static and
 * your hands). Those touches are its nearness too: played much, the sound goes to NEAR's colour, left alone it
 * drifts back to FAR's.
 *
 * A note sounds up to three partials (VOICES), each an interval over it that the signals and the being pick, each
 * with a second harmonic as bright as NEAR or FAR says, wandering in pitch and level as DRIFT allows; AIR adds
 * breath; all through a gentle low-pass (TONE). The being the ceremony summoned (SIGIL and SEAL: its number,
 * 1..16383, 0 none) chooses the intervals: an ordinary one fifths and octaves and ninths, a demon (1 in 50) tritones,
 * seconds and a sub-octave, an angel (1 in 50) octaves, fifths and thirds high up.
 *
 * The ceremony (sm_start: OCT+ on its EDIT page; sm_tick, every UI frame): the being's number taken from the
 * signals, then SM_CANDLES candles lit one by one, over a song, as its track plays (faster the busier), the being built up as they
 * light; at the last its number goes into the track's SIGIL and SEAL (the sound turns to it, a project keeps it).
 *
 * Voice state: ph[0..2] the partials' phases; s[0] the noise, s[1] s[2] the filter, s[3] the slow phase, s[4..6] the
 * partials' intervals (1/16 st), s[7] the voice's own drift seed. */
#ifdef FM1_TICKS_PER_US                                 /* (the unit, the emulator: the free-running timer; the host's */
static uint32_t fm1_ticks(void);                       /* table dumps have none) */
#define SM_CLOCK() fm1_ticks()
#else
#define SM_CLOCK() 0u
#endif
static uint32_t sm_sig(void);                          /* (ui_summon.c: draw_graph's signature) */
#define SM_NSIG 8u
#define SM_CANDLES 6u
#define SM_STEP_MS 25000u                              /* ((Vallée) a candle: 25 s of the song playing (sooner played to):
                                                          the six over 2.5 minutes, a song */
static struct {
    uint32_t ent, last_t, seen, blocks;
    volatile uint32_t touch;                            /* (the UI and MIDI IN count here: sm_touch) */
    int32_t near;                                       /* Q15: how near the room is (touches rise it, time falls it) */
    uint16_t sig[SM_NSIG], to[SM_NSIG];                 /* 0..1023, and where each is going */
    uint8_t cer, trk, lit;                              /* ceremony: running; its track; candles lit */
    uint16_t next;                                      /* the number being summoned */
    uint32_t prog;                                      /* ms of the ceremony done (to SM_CANDLES x SM_STEP_MS) */
    uint32_t t0, rt;                                    /* (the ceremony's last frame; the room's) */
    /* ((Vallée) the being's part (summon_play.c): a key on its EDIT page starts it; it plays with the song */
    uint8_t on, key, root, base, ptrk;                  /* playing; the key pressed; the song's key; its low note; its track */
    volatile uint8_t rq, rq_note, rq_trk;               /* the UI asks (1 begin from that key on that track, 2 fade); the
                                                           audio's block does it (summon_play.c sm_play) */
    uint16_t mask;                                      /* the song's scale (seq.c SCALE_MASK) */
    uint32_t acc, step, rng;                            /* samples to the next 16th; 16ths played; its dice */
    uint8_t hn[6], hl[6];                               /* the notes it holds, 16ths left of each */
    uint16_t hpc;                                       /* the song's chord now: its pitch classes (bit i: i above C) */
    uint8_t hroot, line;                                /* .. its root (a pitch class); where the line is in its notes */
} sm;
static void sm_touch(uint32_t n)
{
    sm.touch += n;
    sm.ent = (sm.ent ^ SM_CLOCK()) * 2654435761u + n;
    sm.to[sm.ent % SM_NSIG] = (uint16_t)(640u + (sm.ent >> 23) % 384u);   /* (a touch: a signal stronger) */
}
/* its kind: 0 a being, 1 a demon, 2 an angel (1 in 50 each over the numbers), -1 none */
static int32_t sm_kind(uint32_t sig)
{
    uint32_t h;
    if (!sig)
        return -1;
    h = (sig * 2654435761u) >> 16;
    return h % 50u == 7u ? 1 : h % 50u == 31u ? 2 : 0;
}
static uint32_t sm_sigil(const track_t *t) { return ((uint32_t)t->p[P_E6] & 127u) << 7 | ((uint32_t)t->p[P_E7] & 127u); }

/* the room: one step of what the signals do (an audio block's worth, 32 samples). Run from the UI's frame (sm_room),
   never from a track's render: both cores read sig[] and near while they mix, and nothing they render may depend on
   which core rendered which track (tests/dualcore_test.c) */
static void sm_room_step(uint32_t d)
{
    uint32_t k, tc = sm.touch;
    sm.ent = (sm.ent ^ d ^ (d << 11) ^ (uint32_t)song.batt_raw << 19) * 2654435761u + 0x7F4A7C15u;
    sm.ent ^= sm.ent >> 15;
    if (tc != sm.seen) {                                /* touched: nearer */
        sm.near = clamp(sm.near + (int32_t)(tc - sm.seen) * 2500, 0, 32767);
        sm.seen = tc;
    }
    sm.near -= (sm.near >> 9) + 1;                      /* (~6 s back to far) */
    if (sm.near < 0)
        sm.near = 0;
    k = sm.blocks++ % SM_NSIG;                          /* a signal a step: gliding to where the static sends it, */
    if ((sm.ent & 1023u) < 3u + (uint32_t)(sm.near >> 12))   /* somewhere new every second or two (sooner played) */
        sm.to[k] = (uint16_t)(sm.ent >> 22);
    {
        int32_t s = sm.sig[k];
        s += ((int32_t)sm.to[k] - s) / 64;
        sm.sig[k] = (uint16_t)clamp(s, 0, 1023);
    }
}
static void sm_room(uint32_t ms)                        /* (ui_summon.c sm_tick, every frame: ~1378 steps a second) */
{
    uint32_t now = SM_CLOCK(), d = now - sm.last_t, n = (ms > 100u ? 100u : ms) * 11u / 8u;
    sm.last_t = now;
    while (n--)
        sm_room_step(d += 0x9E37u);
}

/* the intervals (semitones) a partial may take: an ordinary being's, a demon's, an angel's */
/* the partials a note may have (semitones): octaves only, so whatever chord the note is in, they are in it too (an
   ordinary being's round the note, a demon's below it, an angel's above) */
static const int8_t SM_IV[3][8] = {{0, 12, -12, 24, 12, 0, -12, 24}, {0, -12, -24, -12, 0, 12, -24, -12},
                                   {12, 24, 36, 12, 24, 0, 36, 24}};
static void summon_note_on(track_t *t, voice_t *v)
{
    uint32_t sig = sm_sigil(t), k;
    int32_t kind = sm_kind(sig);
    const int8_t *iv = SM_IV[kind > 0 ? kind : 0];
    for (k = 0; k < 3u; k++)                            /* (the first on the note itself, an angel's an octave up) */
        v->s[4 + k] = (int32_t)iv[k ? (sm.sig[(k * 3u + v->note) % SM_NSIG] + (sig >> (k * 3u))) % 8u : 0u] * 16;
    v->s[0] = (int32_t)(sm.ent | 1u);
    v->s[3] = (int32_t)(sm.ent >> 8);
    v->s[7] = (int32_t)sm.ent;
}

static void summon_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    int32_t near = sm.near, bright = (p[P_E0] * near + p[P_E1] * (32767 - near)) >> 15;   /* 0..127 */
    int32_t drift = p[P_E2], nv = 1 + clamp(p[P_E3], 0, 2), air = p[P_E5];
    int32_t amp[3], ic1 = v->s[1], ic2 = v->s[2], rng = v->s[0], hb = bright * 200;   /* (the 2nd harmonic, Q15) */
    uint32_t inc[3], ph[3], i, k, slow = (uint32_t)v->s[3];
    tsvf_t f;
    for (k = 0; k < 3u; k++) {                          /* per block: each partial's pitch and level, wandering */
        int32_t sg = (int32_t)sm.sig[(k + (uint32_t)v->s[7]) % SM_NSIG] - 512;
        int32_t ct = sg * drift / 10000;                /* (up to ~ +-6 cents: a chorus, never out of tune) */
        int32_t lfo = sine_i(slow * (k + 1u) + k * 1431655765u);
        inc[k] = cents_inc(m->pitch16 + v->s[4 + k], ct, m->fine);
        ph[k] = v->ph[k];
        amp[k] = (int32_t)k < nv ? (k ? 7000 : 10000) - ((lfo >> 6) * drift >> 7) : 0;
    }
    v->s[3] = (int32_t)(slow + 0x00180000u + (uint32_t)drift * 0x2000u);
    tsvf_coef(&f, clamp((p[P_E4] << 8) + bright * 40 + m->cutoff, 0, 127 << 8), 20);
    for (i = 0; i < n; i++) {
        int32_t y = 0;
        for (k = 0; k < 3u; k++) {
            if (!amp[k])
                continue;
            ph[k] += inc[k];
            y += (sine_i(ph[k]) + ((sine_i(ph[k] << 1) * hb) >> 15)) * amp[k] >> 15;
        }
        if (air)
            y += (((int32_t)noise32(&rng) >> 18) * air) >> 5;
        y = tsvf_lp(&f, y, &ic1, &ic2);
        out[i] += voice_amp(y, m, i);
    }
    for (k = 0; k < 3u; k++)
        v->ph[k] = ph[k];
    v->s[0] = rng;
    v->s[1] = ic1;
    v->s[2] = ic2;
}

/* a being: n x n (12 the lanes', 16 the circle's), mirrored, grown from its number: 1 the body, 2 an eye */
static void sm_being(uint32_t sig, uint32_t n, uint8_t px[16][16])
{
    uint8_t g[16][8], q[16][8];
    uint32_t r = sig * 2654435761u + 0x6C8E9CF5u, h = n / 2u, x, y, pass, nn2 = n * n;
#define SM_RND() (r ^= r << 13, r ^= r >> 17, r ^= r << 5, r)
    for (y = 0; y < n; y++)
        for (x = 0; x < h; x++) {
            int32_t X2 = (int32_t)(2u * x + 1u) - (int32_t)n, Y2 = (int32_t)(2u * y + 1u) - (int32_t)n;
            uint32_t d = ((uint32_t)(X2 * X2) * 1333u + (uint32_t)(Y2 * Y2) * 1562u) / nn2;
            g[y][x] = (uint8_t)(SM_RND() % 100u < (d < 450u ? 90u : d < 900u ? 45u : 8u));
        }
    for (pass = 0; pass < 2u; pass++) {                 /* smoothed into a body */
        for (y = 0; y < n; y++)
            for (x = 0; x < h; x++) {
                uint32_t c = 0;
                int32_t yy, xx;
                for (yy = (int32_t)y - 1; yy <= (int32_t)y + 1; yy++)
                    for (xx = (int32_t)x - 1; xx <= (int32_t)x + 1; xx++)
                        if (yy >= 0 && yy < (int32_t)n && xx >= 0)
                            c += g[yy][xx < (int32_t)h ? xx : (int32_t)h - 1];
                q[y][x] = (uint8_t)(c >= 5u || (c >= 4u && g[y][x]));
            }
        memcpy(g, q, sizeof g);
    }
    for (y = n / 5u; y < n * 4u / 5u; y++)              /* a spine: one piece */
        g[y][h - 1u] = 1;
    x = h - 5u + SM_RND() % 3u;                         /* legs, arms or none, antennae or none */
    for (y = n - 4u; y < n; y++)
        g[y][x] = 1;
    if (SM_RND() % 10u < 6u) {
        y = n * 2u / 5u + SM_RND() % (n / 4u);
        for (x = 0; x < 4u; x++)
            g[y][x] = 1;
        g[y - 1u][0] = 1;
    }
    if (SM_RND() % 2u) {
        x = h - 4u + SM_RND() % 3u;
        for (y = 0; y < 4u; y++)
            g[y][x] = 1;
    }
    for (y = 0; y < n; y++)
        for (x = 0; x < n; x++)
            px[y][x] = g[y][x < h ? x : n - 1u - x];
    {                                                   /* the eyes: one, two, three or four */
        static const uint8_t NE[5] = {1, 2, 2, 3, 4};
        uint32_t ne = NE[SM_RND() % 5u], ey = n * 4u / 15u + SM_RND() % 3u, dx = 2u + SM_RND() % 2u;
        if (ne & 1u)
            px[ey][h - 1u] = px[ey][h] = 2;
        if (ne >= 2u)
            px[ey][h - 1u - dx] = px[ey][h + dx] = 2;
        if (ne == 4u)
            px[ey + 2u][h - 2u] = px[ey + 2u][h + 1u] = 2;
    }
#undef SM_RND
}
/* its name: a few syllables from its number (a demon's and an angel's endings their own) */
static void sm_name(uint32_t sig, char *b)
{
    static const char *const ON[12] = {"VA", "OR", "KA", "ZE", "MU", "I", "SO", "THA", "NE", "QU", "YR", "LO"};
    static const char *const MI[8] = {"SH", "R", "TH", "M", "L", "Z", "V", "KH"};
    static const char *const VO[4] = {"A", "I", "U", "E"};
    static const char *const EN[3][4] = {{"UTH", "AX", "OS", "ION"}, {"ZAR", "GOTH", "MOR", "AZ"},
                                         {"IEL", "AEL", "IRON", "UEL"}};
    uint32_t h = sig * 2246822519u, kind = (uint32_t)(sm_kind(sig) > 0 ? sm_kind(sig) : 0);
    b[0] = 0;
    str_cpy(b, ON[h % 12u], 16);
    str_cpy(b + str_len(b), MI[(h >> 5) % 8u], 16 - str_len(b));
    str_cpy(b + str_len(b), VO[(h >> 9) % 4u], 16 - str_len(b));
    str_cpy(b + str_len(b), EN[kind][(h >> 13) % 4u], 16 - str_len(b));
}

/* {NEAR, FAR, DRIFT, VOICES, TONE, AIR, SIGIL, SEAL} (SIGIL SEAL 0: no being yet: the circle empty) */
static const preset_t SUMMON_PRESETS[] = {
    {"CALLING", {96, 30, 50, 2, 80, 16, 0, 0}, {90, 90, 120, 100}, 0, 0, FX(0, 20, 30, 90), TAG(PT_PAD)},
    {"LOW HOST", {70, 10, 40, 1, 60, 8, 0, 0}, {80, 90, 127, 110}, 0, 0, FX(0, 10, 20, 80), TAG(PT_PAD)},
    {"VIGIL", {110, 60, 70, 2, 96, 30, 0, 0}, {100, 90, 120, 110}, 0, 0, FX(0, 30, 40, 100), TAG(PT_PAD)},
    {"SEANCE", {80, 20, 100, 2, 70, 60, 0, 0}, {110, 90, 110, 120}, 0, 0, FX(0, 20, 50, 100), TAG(PT_FX)},
    {"EMPYREAN", {127, 90, 30, 2, 110, 10, 0, 0}, {100, 90, 127, 120}, 0, 0, FX(0, 40, 30, 110), TAG(PT_PAD)},
};
static const char *const N_SM_VOICES[] = {"1", "2", "3"};
static const engine_t ENG_SUMMON = {
    .name = "SUMMON",
    .page_title = {"CIRCLE", "SIGIL"},
    .edit = {
        {"NEAR", 0, 0, 127, 96, F_PCT, 0},
        {"FAR", 0, 0, 127, 30, F_PCT, 0},
        {"DRIFT", 0, 0, 127, 50, F_PCT, 0},
        {"VOICES", N_SM_VOICES, 0, 2, 2, F_ENUM, 0},
        {"TONE", 0, 0, 127, 80, F_CUTOFF, 0},
        {"AIR", 0, 0, 127, 16, F_PCT, 0},
        {"SIGIL", 0, 0, 127, 0, F_INT, 0},
        {"SEAL", 0, 0, 127, 0, F_INT, 0},
    },
    .presets = SUMMON_PRESETS,
    .npresets = NELEM(SUMMON_PRESETS),
    .note_on = summon_note_on,
    .render = summon_render,
    .block = 0,
    .knob = {P_E0, P_E1, P_E2, P_E4},
    .poly = 4,
    .keep = 0x06,                /* the filter */
};
