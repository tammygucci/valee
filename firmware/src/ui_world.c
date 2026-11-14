/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) THE WORLD: HOME tapped on HOME, in place of the scope. An alien land at dusk where the creatures of the six
 * tracks' sounds (ui_beast.c) dance for the head from the intro (SPR_VL_HEAD), huge and ghostly in the sky. Everything
 * moves to the song, and the song draws on it:
 *   - the creatures dance: each its own move, a new one every bar or two, chosen from the song (hop, sway, shuffle,
 *     orbit, bow to the head, leap), on the beat; a jump on each note its track strikes; asleep while muted;
 *   - the head: bobbing; its eyes following whoever just struck; its third eye flashing on the beat and beaming down
 *     on whoever strikes; now and then it summons one, lifting it up on the beam for a while before it drifts back;
 *     its mouth glowing when the song is loud (it sings along); behind it a ring of dots turning, pulsing on the beat;
 *   - the art the music makes: each note sends a sigil up from its creature (sun, moon, the eye in the pyramid, a
 *     star, an eye, an ankh, a cross, a spiral: by the note's pitch class), which drifts to the head's third eye and
 *     is taken in (the eye flares); and leaves, every other note or so, a star in the sky (across by pitch, up by where
 *     in the bar), joined to that track's last: the song's constellations, the oldest fading, the last 32 kept;
 *   - the land: stars twinkling (more the louder it is), dunes and spires, a ground grid running toward you with the
 *     playhead, a ripple on each beat.
 * ANIM OFF: still. Drawn by graph_scope (ui_graph.c) at the panel's full height, on HOME; its state here, ~0.4 KB. */

#define WL_NP 24u                                      /* sigils at once */
#define WL_NS 32u                                      /* stars kept */
#define WL_SUMMON 40u                                  /* a summoning's frames */
static struct {
    uint16_t idx[NTRK];                                /* each track's step at the last frame (a strike: a new one, on) */
    uint8_t init;
    uint8_t nsig, nstar;
    uint32_t beat_ms, beats;                           /* when the last beat fell; beats since the world opened */
    uint8_t look_k;                                    /* who struck last (the eyes follow) */
    uint8_t beam_k, beam_t;                            /* the third eye's beam: to which track, frames left */
    uint8_t summon_k, summon_t;                        /* who is lifted, frames left */
    uint32_t summon_beat;                              /* the beat the last one began on */
    uint8_t glow;                                      /* the third eye flaring: a sigil taken in */
    uint8_t jump[NTRK];                                /* each creature's jump (a strike), falling */
    struct { int16_t x, y; uint8_t k, g, age; } sig[WL_NP];
    struct { uint8_t x, y, k, prev; } star[WL_NS];     /* (prev: the index of the track's star before, 255: none) */
    uint8_t last[NTRK];                                /* each track's newest star (255: none) */
} wl;

/* the sigils, 5 x 5 */
static void wl_draw_glyph(int32_t x, int32_t y, uint32_t g, uint16_t c)
{
    static const char *const G[8] = {
        ".###.#...##.#.##...#.###.",   /* the sun */
        ".##..#....#....#.....##..",   /* the moon */
        "..#...#.#..#.#.#...######",   /* the eye in the pyramid */
        "#.#.#.###.#####.###.#.#.#",   /* a star */
        ".....###..#.#.#.###......",   /* an eye */
        ".###.#...#.###.#####..#..",   /* an ankh */
        "..#..#####..#....#....#..",   /* a cross */
        "####....#.##.#.#..#.####.",   /* a spiral */
    };
    const char *s = G[g & 7u];
    uint32_t i;
    for (i = 0; i < 25u; i++)
        if (s[i] == '#')
            cv_pset(x + (int32_t)(i % 5u), y + (int32_t)(i / 5u), c);
}
static uint32_t wl_h(uint32_t v) { return (v ^ (v >> 15)) * 2654435761u; }
static int32_t wl_abs(int32_t v) { return v < 0 ? -v : v; }

/* a dotted ellipse (n dots) */
static void wl_oval(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint32_t n, uint16_t c)
{
    uint32_t k;
    for (k = 0; k < n; k++) {
        uint32_t a = k * (0xFFFFFFFFu / n);
        cv_pset(cx + rx * QK_COS(a) / 32767, cy + ry * QK_SIN(a) / 32767, c);
    }
}

/* where track k's creature stands (its feet's middle), dancing aside */
static void wl_spot(uint32_t k, int32_t H, int32_t *x, int32_t *y)
{
    static const uint8_t X[NTRK] = {24, 62, 100, 140, 178, 216};
    static const int8_t Y[NTRK] = {-6, -14, -4, -4, -14, -6};
    *x = X[k];
    *y = H - 8 + Y[k];
}

/* the head's place this frame (its sprite's top left, 2x: 44 x 52); its third eye's middle at +22, +14 */
static void wl_head_at(uint32_t now, int anim, int32_t *hx, int32_t *hy)
{
    *hx = 120 - 22 + (anim ? QK_SIN(now * 262144u) * 3 / 32767 : 0);
    *hy = 2 + (anim ? QK_SIN(now * 0x00070000u) * 2 / 32767 : 0);
}

/* a strike of track k (its step just begun, a note on it): a sigil, maybe a star, the beam, the eyes, a jump */
static void wl_strike(uint32_t k, const step_t *s, int32_t H, int32_t hz, uint32_t now)
{
    uint32_t note = s->n ? s->note[0] : 60u, r = wl_h(now * 31u + k * 977u + note), i;
    int32_t fx, fy;
    wl_spot(k, H, &fx, &fy);
    i = wl.nsig++ % WL_NP;                             /* (the oldest goes) */
    wl.sig[i].x = (int16_t)(fx - 2 + (int32_t)(r % 9u) - 4);
    wl.sig[i].y = (int16_t)(fy - 30);
    wl.sig[i].k = (uint8_t)k;
    wl.sig[i].g = (uint8_t)(note % 12u * 2u / 3u + (r >> 29)) & 7u;
    wl.sig[i].age = 0;
    if (r >> 31) {                                      /* every other note or so: a star */
        uint32_t len, pos;
        int32_t sx = 8 + ((int32_t)note - 30) * 224 / 66, sy;
        len = trk[k].p[P_SLEN] > 0 ? (uint32_t)trk[k].p[P_SLEN] : 16u;
        pos = trk[k].seq_idx % len;
        sy = 4 + (int32_t)((pos % 16u) * (uint32_t)(hz - 34) / 16u) + (int32_t)(r >> 8 & 3u);
        if (sx < 6) sx = 6;
        if (sx > 233) sx = 233;
        i = wl.nstar++ % WL_NS;
        for (pos = 0; pos < WL_NS; pos++)               /* (a star replaced: nothing joins to it now) */
            if (wl.star[pos].prev == i)
                wl.star[pos].prev = 255u;
        for (pos = 0; pos < NTRK; pos++)
            if (wl.last[pos] == i)
                wl.last[pos] = 255u;
        wl.star[i].x = (uint8_t)sx;
        wl.star[i].y = (uint8_t)sy;
        wl.star[i].k = (uint8_t)k;
        wl.star[i].prev = wl.last[k];
        wl.last[k] = (uint8_t)i;
    }
    wl.beam_k = (uint8_t)k;
    wl.beam_t = 3;
    wl.look_k = (uint8_t)k;
    wl.jump[k] = 8;
}

/* track k's creature's dance this frame: its offset from its spot. p: 0 .. 999 through the beat */
static void wl_dance(uint32_t k, uint32_t p, int32_t *dx, int32_t *dy)
{
    uint32_t move = wl_h(wl.beats / 4u / (1u + k % 2u) * 7919u + k * 131u + trk[k].eng_req) % 6u;
    uint32_t half = p * 0x7FFFu / 1000u * 65536u;     /* (0 .. pi through the beat) */
    int32_t up = QK_SIN(half) * 5 / 32767, odd = (int32_t)(wl.beats & 1u);
    *dx = 0;
    *dy = 0;
    switch (move) {
    case 0: *dy = -up; break;                                           /* hop */
    case 1: *dx = (odd ? 1 : -1) * (5 - (int32_t)p * 10 / 1000); break; /* sway, beat to beat */
    case 2: *dx = (wl.beats % 4u < 2u ? -4 : 4); *dy = p < 250u ? -2 : 0; break;   /* shuffle */
    case 3: {                                                           /* orbit: once round in two beats */
        uint32_t a = ((wl.beats & 1u) * 1000u + p) * (0xFFFFFFFFu / 2000u);
        *dx = QK_COS(a) * 6 / 32767;
        *dy = QK_SIN(a) * 3 / 32767;
        break;
    }
    case 4: *dy = 3 - (int32_t)p * 3 / 1000; break;                     /* bow to the head on the beat */
    default: *dy = -up * 2; break;                                      /* leap */
    }
}

static void wl_world(void)
{
    const int32_t H = (int32_t)cv_h, hz = H * 58 / 100, cx = 120;
    const int anim = xe_anim();
    const uint32_t now = anim ? fm1_ms : 0u;
    uint16_t sky = ux_mix(T_BG, T_SURF, 60), ground = ux_mix(T_BG, T_THEME, 10);
    int32_t lvl = 0, k, i, beat, hx, hy;
    uint32_t bpos, len0, p = 0, beat_len;
    /* the levels, the strikes, the beat */
    scope_levels();
    for (k = 0; k < (int32_t)NTRK; k++)
        if (scd.lv[k] > lvl)
            lvl = scd.lv[k];
    len0 = trk[0].p[P_SLEN] > 0 ? (uint32_t)trk[0].p[P_SLEN] : 16u;
    bpos = trk[0].seq_idx % len0;
    if (!wl.init) {
        memset(wl.last, 255, sizeof wl.last);
        for (i = 0; i < (int32_t)WL_NS; i++)
            wl.star[i].prev = 255u;
        wl.init = 1;
    }
    wl_head_at(now, anim, &hx, &hy);
    for (k = 0; k < (int32_t)NTRK; k++) {
        const track_t *t = &trk[k];
        uint32_t len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 16u, cur = t->seq_idx % len;
        if (song.playing && anim && cur != wl.idx[k]) {
            if (!t->p[P_MUTE] && step_on(&t->step[cur]) && ENGINES[t->eng_req % NENGINES]->npresets)
                wl_strike((uint32_t)k, &t->step[cur], H, hz, now);
            if (k == 0 && cur % 4u == 0u) {             /* a beat */
                wl.beat_ms = now;
                wl.beats++;
                if (wl.beats % 4u == 0u && !wl.summon_t) {   /* a bar: one summoned, at even odds (two bars at most) */
                    uint32_t r = wl_h(wl.beats * 2246822519u + bpos), c = (r >> 8) % NTRK;
                    if ((r % 2u == 0u || wl.beats - wl.summon_beat >= 8u) && !trk[c].p[P_MUTE] &&
                        ENGINES[trk[c].eng_req % NENGINES]->npresets) {
                        wl.summon_k = (uint8_t)c;
                        wl.summon_t = WL_SUMMON;
                        wl.summon_beat = wl.beats;
                    }
                }
            }
        }
        wl.idx[k] = (uint16_t)cur;
    }
    beat_len = 60000u / (uint32_t)(song.g[G_BPM] > 0 ? song.g[G_BPM] : 120);
    {   /* (0 .. 1000: just after the beat; 0 when it has passed) */
        uint32_t since = now - wl.beat_ms;
        beat = song.playing && anim && since < 400u ? (int32_t)(400u - since) * 1000 / 400 : 0;
        p = song.playing && anim ? (since % beat_len) * 1000u / beat_len : 0u;
    }
    /* the sky: darker up, the theme's glow at the horizon */
    cv_rrect(3, 0, 234, H, 5, sky, T_BG);
    for (i = 0; i < 6; i++)
        cv_rect(4, hz - 18 + i * 3, 232, 3, ux_mix(sky, T_THEME, 3 + i * 3));
    for (i = 0; i < 46; i++) {                          /* the stars: twinkling; with the song, more of them lit */
        uint32_t h = wl_h((uint32_t)i * 7919u + 17u), tw = wl_h((uint32_t)i + (now / 160u) * 131u) % 1000u;
        int32_t x = 6 + (int32_t)(h % 228u), y = 2 + (int32_t)(h >> 12) % (hz - 20);
        if ((int32_t)tw < 350 + lvl / 2)
            cv_pset(x, y, tw < 120u ? T_TEXT : (h >> 30) ? T_MID : T_DIM);
    }
    for (i = 0; i < (int32_t)WL_NS && i < (int32_t)wl.nstar; i++) {   /* the song's constellations */
        uint32_t age = (wl.nstar - 1u + WL_NS - (uint32_t)i) % WL_NS;   /* (0: the newest) */
        int32_t j = wl.star[i].prev;
        uint16_t c = ux_mix(T_TRK(wl.star[i].k), sky, (int32_t)(25u + age * 2u));
        if (j < (int32_t)WL_NS)
            xe_line(wl.star[i].x, wl.star[i].y, wl.star[j].x, wl.star[j].y, ux_mix(c, sky, 55));
        cv_rect(wl.star[i].x - 1, wl.star[i].y, 3, 1, c);
        cv_rect(wl.star[i].x, wl.star[i].y - 1, 1, 3, c);
        if (age < 3u)
            cv_pset(wl.star[i].x, wl.star[i].y, T_TEXT);
    }
    /* the head: a ring of dots turning behind it, pulsing on the beat; its eyes following, its third eye lit on the
     * beat and when it takes a sigil in, its mouth glowing when it is loud */
    {
        uint16_t cols[4];
        int32_t ex = 0;
        xe_dots(hx + 22, hy + 24, 30 + beat * 3 / 1000, 36u, now * 0x00100000u, ux_mix(T_THEME, sky, 55 - beat / 40));
        cols[0] = ux_mix(sky, T_TEXT, 30);
        cols[1] = ux_mix(sky, T_TEXT, 24);
        cols[2] = ux_mix(sky, T_BG, 40);
        cols[3] = beat > 300 || wl.glow ? T_ACCENT : ux_mix(sky, T_ACCENT, 45);
        ui_sprite(hx, hy, SPR_VL_HEAD, 4u, cols, 2);
        if (song.playing && anim) {                     /* the eyes: on whoever struck last (or is lifted) */
            int32_t tx, ty;
            wl_spot(wl.summon_t ? wl.summon_k : wl.look_k, H, &tx, &ty);
            ex = (tx - (hx + 22)) / 25;
            ex = ex < -2 ? -2 : ex > 2 ? 2 : ex;
        }
        cv_rect(hx + 12 + ex, hy + 20, 2, 2, ux_mix(T_TEXT, T_ACCENT, lvl * 60 / 1000));
        cv_rect(hx + 26 + ex, hy + 20, 2, 2, ux_mix(T_TEXT, T_ACCENT, lvl * 60 / 1000));
        if (lvl > 650)                                  /* singing along */
            cv_rect(hx + 14, hy + 34, 12, 2, ux_mix(T_ACCENT, sky, 100 - (lvl - 650) * 100 / 350));
        if (wl.glow) {
            xe_ring(hx + 22, hy + 14, 3 + (int32_t)wl.glow / 2, ux_mix(T_ACCENT, sky, 70 - (int32_t)wl.glow * 8));
            if (anim)
                wl.glow--;
        }
    }
    /* the dunes and spires on the horizon */
    for (i = 4; i < 236; i++) {
        uint32_t h = wl_h((uint32_t)i / 9u + 5u), sp = wl_h((uint32_t)i / 9u + 77u) % 5u == 0u;
        int32_t m = (i % 9) < 5 ? i % 9 : 8 - i % 9, s = 1 + (int32_t)(h % 4u) + (sp ? m * (int32_t)(3u + h % 4u) : m / 2);
        cv_rect(i, hz - s, 1, s, ux_mix(T_BG, sky, 35));
    }
    /* the ground: its grid running toward you with the playhead; the beat's ripple */
    cv_rect(4, hz, 232, H - hz - 1, ground);
    {
        uint32_t ph = song.playing ? (uint32_t)(bpos % 4u) * 250u : 0u;
        for (i = 0; i < 6; i++) {                       /* across: closer, further apart */
            int32_t d = (i * 1000 + (int32_t)ph) / 6, y = hz + 1 + (H - hz) * d / 1000 * d / 1000;
            if (y > hz && y < H - 1)
                cv_rect(4, y, 232, 1, ux_mix(ground, T_THEME, 18 + d / 50));
        }
        for (i = -7; i <= 7; i++)                       /* toward the horizon */
            xe_line(cx + i * 6, hz + 1, cx + i * 40, H - 2, ux_mix(ground, T_THEME, 16));
    }
    if (beat)
        wl_oval(cx, hz + 22, 20 + (1000 - beat) * 90 / 1000, 4 + (1000 - beat) * 14 / 1000, 64u,
                ux_mix(T_ACCENT, ground, 100 - beat / 12));
    /* the creatures dancing */
    for (k = 0; k < (int32_t)NTRK; k++) {
        const track_t *t = &trk[k];
        uint8_t art[XB_ART_N];
        int32_t fx, fy, dx = 0, dy = 0, lift = 0;
        int sing = xe_sings(t), muted = t->p[P_MUTE] != 0;
        if (!ENGINES[t->eng_req % NENGINES]->npresets)
            continue;
        wl_spot((uint32_t)k, H, &fx, &fy);
        if (anim && !muted) {
            if (song.playing)
                wl_dance((uint32_t)k, p, &dx, &dy);
            else
                dy = QK_SIN(now * 327680u + (uint32_t)k * 0x2A000000u) / 32767;   /* (stopped: breathing) */
            dy -= (int32_t)wl.jump[k] / 2 + scd.lv[k] * 2 / 1000;
            if (wl.jump[k])
                wl.jump[k] = (uint8_t)(wl.jump[k] > 2u ? wl.jump[k] - 2u : 0u);
            if (wl.summon_t && (uint32_t)k == wl.summon_k) {   /* lifted up on the beam, then let down */
                uint32_t a = (WL_SUMMON - wl.summon_t) * (0x7FFFFFFFu / WL_SUMMON);   /* (0 .. pi) */
                lift = QK_SIN(a) * 26 / 32767;
            }
        }
        wl_oval(fx + dx, fy, 9 - lift / 6, 2, 28u, ux_mix(ground, T_BG, 50 - lift));
        if (lift > 2) {                                 /* the beam holding it */
            xe_line(hx + 22, hy + 14, fx + dx - 3, fy + dy - lift - 12, ux_mix(T_ACCENT, sky, 45));
            xe_line(hx + 22, hy + 14, fx + dx + 3, fy + dy - lift - 12, ux_mix(T_ACCENT, sky, 45));
            xe_dots(fx + dx, fy + dy - lift - 12, 15, 16u, now * 0x01000000u, ux_mix(T_ACCENT, sky, 30));
        }
        xe_track_art(t, art);
        xe_alien(fx + dx - 12, fy + dy - lift - 24, art, muted ? T_DIM : T_TRK(k), sky,
                 muted ? 2 : sing || lift > 2 ? 1 : xe_blink((uint32_t)k) ? 3 : 0, 2);
        if (muted && anim && (now / 600u + (uint32_t)k) % 3u == 0u)   /* (asleep) */
            wl_draw_glyph(fx + 9, fy - 30, 7u, T_DIM);
    }
    if (wl.summon_t && anim)
        wl.summon_t--;
    if (wl.beam_t && anim) {                            /* the beam: the third eye to whoever struck */
        int32_t fx, fy;
        wl_spot(wl.beam_k, H, &fx, &fy);
        xe_line(hx + 22, hy + 14, fx, fy - 26, ux_mix(T_TRK(wl.beam_k), sky, 35));
        wl.beam_t--;
    }
    /* the sigils: up from the notes, drawn to the third eye, taken in */
    for (i = 0; i < (int32_t)WL_NP && i < (int32_t)wl.nsig; i++) {
        int32_t tx = hx + 20, ty = hy + 12;
        if (wl.sig[i].age >= 40u)
            continue;
        if (anim) {
            int32_t ddx = tx - wl.sig[i].x, ddy = ty - wl.sig[i].y;
            if (wl.sig[i].age > 6u) {                   /* (first up, then toward the eye) */
                wl.sig[i].x = (int16_t)(wl.sig[i].x + (ddx > 1 ? 1 + ddx / 24 : ddx < -1 ? -1 + ddx / 24 : 0));
                wl.sig[i].y = (int16_t)(wl.sig[i].y + (ddy < -1 ? -1 + ddy / 20 : ddy > 1 ? 1 : 0));
            } else
                wl.sig[i].y = (int16_t)(wl.sig[i].y - 2);
            wl.sig[i].age++;
            if (wl_abs(ddx) < 4 && wl_abs(ddy) < 4) {  /* taken in */
                wl.sig[i].age = 40u;
                wl.glow = 6;
                continue;
            }
        }
        if (wl.sig[i].y > 1 && wl.sig[i].x > 4 && wl.sig[i].x < 231)
            wl_draw_glyph(wl.sig[i].x, wl.sig[i].y, wl.sig[i].g,
                          ux_mix(T_TRK(wl.sig[i].k), sky, wl.sig[i].age > 20u ? (int32_t)(wl.sig[i].age - 20u) * 2 : 0));
    }
}
