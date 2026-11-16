/* SPDX-License-Identifier: GPL-3.0-only */
/* (Skiff XENO) THEME XENO: the void between the stars. HOME is a tape machine (after the OP-1's tape, as ORBIT's and
 * KARMA's HOMEs are): two reels whose hubs are turning pentagrams, the tape running from the left reel to the right one
 * as the pattern plays (the left pack shrinks, the right one grows), a head at the middle, and on the head the
 * selected track's alien, twice its size, singing on its notes. Under the deck a lane per track (DESIGN.md rule 10:
 * its number, never the colour alone), each with its engine's alien, its sound's name, and its steps on a strip of
 * tape running past the head: the step playing sits in the head's window, the steps played go by on its left, dimmer.
 * (Vallée) Every sound has its own creature (ui_beast.c): the engine gives the family (ANALOG's greys, FM6's pyramids, VOICE's
 * hooded singers ..), the sound the rest. Aliens blink now and then, sing (their eyes the ACCENT, a ripple beside them) when
 * their track strikes a note, and sleep (eyes shut, dim) while muted. ANIM OFF: no blinking, the reels still.
 * Included by ui_graph.c after ui_quark.c (QK_COS, QK_SIN), ui_crypt.c (ui_sprite, cr_stroke). */

static void sound_name(const track_t *t, char *b);   /* (ui_draw.c) */
/* (Vallée) THEME TEEN (its colours, CUSTOM's over it too): XENO's screens plain, no esoterica: a tape machine with
 * plain reels, no stars, no creatures (on the head, in the lanes, the preset box); HOME on HOME the song's map over a
 * wire shape that the song moves (ui_world.c wl_geo) */
#define UX_TEEN (ux.teen != 0)
#define XE_LANE_H 13                                  /* a lane, 1 between them: the deck gets what is left (39 or more) */
#define XE_TEEN_TOP (Y_GRAPH - Y_LABEL)              /* ((Vallée) TEEN's HOME tall: the cards' row's rows (xe_home) */
#define XE_LANE_TEEN 9                                /* ((Vallée) TEEN: thin lanes, the deck the bigger */
#define XE_HEAD_X 120                                 /* the head: the deck's middle and the lanes' window */
#define XE_TAPE_X0 92                                 /* the lanes' tape: x 92 .. 236 */
#define XE_TAPE_X1 236
#define XE_PITCH 7                                    /* a step on the tape: 5 px, 2 between */
#define XE_CELL_W 5

#include "ui_beast.c"                                 /* (Vallée) a creature for every sound */
/* the track's sound's creature: a factory sound's by its place in its engine's list, any other (a user preset, FM6's
 * own patch, a user sample) by its name */
static void xe_track_art(const track_t *t, uint8_t *art)
{
    uint32_t e = t->eng_req % NENGINES, n = ENGINES[e]->npresets ? t->preset % ENGINES[e]->npresets : 0u;
    char name[20];
    sound_name(t, name);
    n = !ENGINES[e]->npresets || !str_eq(ENGINES[e]->presets[n].name, name) ? xb_hash(name) : n + e * 53u;
    xe_beast_art(trk_index(t), n, art);   /* ((Vallée) of the track's family: ui_beast.c) */
}
static int xe_anim(void) { return !(ui_prefs & PREF_ANIM_OFF); }

/* the alien: 12 x 12, z times that. mood 0 idle, 1 singing (eyes ACCENT), 2 asleep (muted: eyes shut, dim), 3 a blink */
static void xe_alien(int32_t x, int32_t y, const uint8_t *art, uint16_t body, uint16_t dark, int mood, int32_t z)
{
    uint16_t b = mood == 2 ? ux_mix(body, T_BG, 55) : body;
    uint16_t cols[4];
    cols[0] = b;
    cols[1] = mood == 1 ? T_ACCENT : mood >= 2 ? ux_mix(b, dark, 35) : T_TEXT;
    cols[2] = mood >= 2 ? ux_mix(b, dark, 35) : dark;
    cols[3] = ux_mix(b, T_TEXT, 55);
    ui_sprite(x, y, art, 4u, cols, z);
}
/* a blink: a few frames in a hundred, each track at its own moment */
static int xe_blink(uint32_t k) { return xe_anim() && (ui.frame + k * 37u) % 97u < 3u; }

/* circles: a ring (1 px) and a disc, midpoint */
static int32_t xe_isqrt(int32_t v)
{
    int32_t r = 0;
    while ((r + 1) * (r + 1) <= v)
        r++;
    return r;
}
static void xe_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t dy;
    for (dy = -r; dy <= r; dy++) {
        int32_t dx = xe_isqrt(r * r - dy * dy);
        cv_rect(cx - dx, cy + dy, 2 * dx + 1, 1, c);
    }
}
static void xe_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t x = r, y = 0, e = 1 - r;
    while (x >= y) {
        cv_pset(cx + x, cy + y, c); cv_pset(cx - x, cy + y, c); cv_pset(cx + x, cy - y, c); cv_pset(cx - x, cy - y, c);
        cv_pset(cx + y, cy + x, c); cv_pset(cx - y, cy + x, c); cv_pset(cx + y, cy - x, c); cv_pset(cx - y, cy - x, c);
        y++;
        if (e < 0)
            e += 2 * y + 1;
        else {
            x--;
            e += 2 * (y - x) + 1;
        }
    }
}
/* a 1 px line */
static void xe_line(int32_t xa, int32_t ya, int32_t xb, int32_t yb, uint16_t c)
{
    int32_t dx = xb - xa, dy = yb - ya, n = dx < 0 ? -dx : dx, m = dy < 0 ? -dy : dy, k;
    if (m > n)
        n = m;
    if (!n)
        n = 1;
    for (k = 0; k <= n; k++)
        cv_pset(xa + (dx * k + (dx >= 0 ? n / 2 : -n / 2)) / n, ya + (dy * k + (dy >= 0 ? n / 2 : -n / 2)) / n, c);
}
/* a dotted ring: n dots, turned by ph */
static void xe_dots(int32_t cx, int32_t cy, int32_t r, uint32_t n, uint32_t ph, uint16_t c)
{
    uint32_t k;
    for (k = 0; k < n; k++) {
        uint32_t a = ph + k * (0xFFFFFFFFu / n);
        cv_pset(cx + r * QK_COS(a) / 32767, cy + r * QK_SIN(a) / 32767, c);
    }
}

/* a reel: its rim, the tape wound on it (pack px from the middle), the hub ((Vallée) a pentagram in its circle, a
 * sigil) turned by ph, a ring of twelve */
static void xe_reel(int32_t cx, int32_t cy, int32_t r, int32_t pack, uint32_t ph, uint16_t tape)
{
    int32_t hr = r / 2, px[5], py[5];
    uint32_t k;
    xe_disc(cx, cy, r, T_SURF);
    if (pack > hr)
        xe_disc(cx, cy, pack, tape);
    xe_ring(cx, cy, pack > hr ? pack : hr + 1, ux_mix(tape, T_TEXT, 25));
    xe_ring(cx, cy, r, T_MID);
    xe_dots(cx, cy, r - 2, 12u, ph / 3u, T_DIM);           /* (the zodiac's twelve, slower than the hub) */
    xe_disc(cx, cy, hr, T_BG);
    if (UX_TEEN) {                                         /* (TEEN: a reel's own hub, three spokes) */
        xe_ring(cx, cy, hr - 1, T_MID);
        for (k = 0; k < 3u; k++) {
            uint32_t a = ph + k * 1431655765u;
            xe_line(cx + 2 * QK_COS(a) / 32767, cy + 2 * QK_SIN(a) / 32767, cx + (hr - 2) * QK_COS(a) / 32767,
                    cy + (hr - 2) * QK_SIN(a) / 32767, T_TEXT);
        }
        xe_disc(cx, cy, 2, T_THEME);
        return;
    }
    xe_ring(cx, cy, hr - 1, ux_mix(T_BG, T_THEME, 55));    /* (the sigil's circle) */
    for (k = 0; k < 5u; k++) {
        uint32_t a = ph + k * 858993459u;                  /* (a fifth of a turn) */
        px[k] = cx + (hr - 2) * QK_COS(a) / 32767;
        py[k] = cy + (hr - 2) * QK_SIN(a) / 32767;
    }
    for (k = 0; k < 5u; k++)                               /* the pentagram: each point to the one two on */
        xe_line(px[k], py[k], px[(k + 2u) % 5u], py[(k + 2u) % 5u], k & 1u ? T_THEME : ux_mix(T_THEME, T_TEXT, 50));
    cv_pset(cx, cy, T_TEXT);
}

/* the stars behind the deck, the same every time */
static void xe_stars(int32_t w, int32_t h)
{
    uint32_t s = 0x9E3779B9u, k;
    for (k = 0; k < 26u; k++) {
        s = s * 1664525u + 1013904223u;
        cv_pset(4 + (int32_t)(s >> 8) % (w - 8), 2 + (int32_t)(s >> 20) % (h - 6), k % 5u ? T_DIM : T_MID);
    }
}

/* the step track k plays (or will, stopped), its length */
static uint32_t xe_pos(const track_t *t, uint32_t *len)
{
    *len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 16u;
    return t->seq_idx % *len;
}
static int xe_sings(const track_t *t)
{
    uint32_t len, cur = xe_pos(t, &len);
    return song.playing && !t->p[P_MUTE] && step_on(&t->step[cur]);
}

/* (Vallée) TEEN's deck, as an OP-1's tape: on black, two reels drawn in thin lines (the rim, a hub of two rings, three
 * short spokes turning), the tape from under each reel down round a row of rollers to the head (a square, the lanes'
 * window under it); between the reels the counter, the track in its box, PLAY's triangle and the tempo */
static int xe_teen_tall(void) { return ui.home && !ui.home_scope && !ui.layer && large_kind() != LK_TALL; }   /* ((Vallée)
                                                       * XENO too: its HOME tall the same way, no cards on top) */
static void xe_deck_teen(int32_t D)
{
    const track_t *t = TSEL;
    uint32_t len, cur = xe_pos(t, &len), k;
    int32_t r = (D - 13) / 2, cy = r + 2, lcx = 6 + r, rcx = 234 - r, ry = D - 6, b[4], i;
    int32_t rx[5] = {lcx - r / 3, lcx + r * 2 / 3 + 4, XE_HEAD_X, rcx - r * 2 / 3 - 4, rcx + r / 3};
    uint32_t ph = song.playing && xe_anim() ? ui.frame * 0x03000000u : 0x08000000u;
    char d[8];
    cv_rect(0, 0, 240, D, T_BG);
    for (i = 0; i < 2; i++) {                          /* the reels */
        int32_t c = i ? rcx : lcx;
        xe_ring(c, cy, r, T_TEXT);
        xe_ring(c, cy, r / 4 + 2, T_TEXT);
        xe_ring(c, cy, r / 4 - 1 > 1 ? r / 4 - 1 : 1, T_MID);
        cv_pset(c, cy, T_TEXT);
        for (k = 0; k < 3u; k++) {
            uint32_t a = ph + (uint32_t)i * 0x15555555u + k * 1431655765u;
            xe_line(c + r * 45 / 100 * QK_COS(a) / 32767, cy + r * 45 / 100 * QK_SIN(a) / 32767,
                    c + r * 78 / 100 * QK_COS(a) / 32767, cy + r * 78 / 100 * QK_SIN(a) / 32767, T_TEXT);
        }
    }
    /* the tape: off each reel's foot to the rollers and the head, thin */
    xe_line(lcx - r * 7 / 10, cy + r * 7 / 10, rx[0] - 3, ry, T_MID);
    xe_line(rcx + r * 7 / 10, cy + r * 7 / 10, rx[4] + 3, ry, T_MID);
    for (k = 0; k < 4u; k++)
        xe_line(rx[k], ry + 3, rx[k + 1], ry + 3, T_MID);
    for (k = 0; k < 5u; k++) {
        if (k == 2u) {                                 /* the head: a square, the lanes' window under it */
            cv_rect(rx[k] - 4, ry - 4, 9, 1, T_TEXT);
            cv_rect(rx[k] - 4, ry - 4, 1, 8, T_TEXT);
            cv_rect(rx[k] + 4, ry - 4, 1, 8, T_TEXT);
            cv_rect(rx[k] - 4, ry + 3, 9, 1, T_BG);
            cv_rect(XE_HEAD_X - 4, ry + 4, 1, D - ry - 4, T_ACCENT);
            cv_rect(XE_HEAD_X + 4, ry + 4, 1, D - ry - 4, T_ACCENT);
            continue;
        }
        xe_disc(rx[k], ry, 3, T_BG);
        xe_ring(rx[k], ry, 3, T_TEXT);
        if (k == 1u || k == 3u)
            cv_pset(rx[k], ry, T_TEXT);
    }
    {   /* between the reels: the counter, then the track's box, PLAY, the tempo */
        int32_t x0 = lcx + r + 4, x1 = rcx - r - 4, mid = (x0 + x1) / 2,
                yy = xe_teen_tall() && cy - 4 < XE_TEEN_TOP + 3 ? XE_TEEN_TOP + 3 : cy - 4;   /* (tall: under the seam) */
        uint32_t n = cur + 1u, bpm = (uint32_t)song.g[G_BPM];
        d[0] = (char)('0' + n / 100u % 10u);
        d[1] = (char)('0' + n / 10u % 10u);
        d[2] = (char)('0' + n % 10u);
        d[3] = 0;
        text_ink(&AF_M, d, b);
        cv_text(mid - b[2] / 2 - b[0], 2, &AF_M, d, T_TEXT);
        cv_rect(x0 + 2, yy, 11, 1, T_TEXT);            /* the track in a box */
        cv_rect(x0 + 2, yy + 12, 11, 1, T_TEXT);
        cv_rect(x0 + 2, yy, 1, 13, T_TEXT);
        cv_rect(x0 + 12, yy, 1, 13, T_TEXT);
        d[0] = (char)('1' + song.sel % NTRK);
        d[1] = 0;
        text_ink(&AF_S, d, b);
        cv_text(x0 + 8 - b[2] / 2 - b[0], yy + CAP_IN(S, 13), &AF_S, d, t->p[P_MUTE] ? T_DIM : T_TRK(song.sel % NTRK));
        for (i = 0; i < 11; i++) {                     /* PLAY: an outline, filled while it plays */
            int32_t h = i < 6 ? i : 10 - i;
            if (song.playing)
                cv_rect(mid - 4, yy + 1 + i, h + 1, 1, T_TEXT);
            else {
                cv_pset(mid - 4, yy + 1 + i, T_TEXT);
                cv_pset(mid - 4 + h, yy + 1 + i, T_TEXT);
            }
        }
        fmt_int(d, (int32_t)bpm);
        text_ink(&AF_M, d, b);
        cv_text(x1 - 2 - b[2] - b[0], yy + CAP_IN(M, 13), &AF_M, d, T_THEME);
    }
}

/* the deck: D px tall at the canvas's top */
static void xe_deck(int32_t D)
{
    if (UX_TEEN) {
        xe_deck_teen(D);
        return;
    }
    const track_t *t = TSEL;
    uint32_t len, cur = xe_pos(t, &len), k;
    int32_t r = D / 2 - 3 > 27 ? 27 : D / 2 - 3, cy = D / 2 - 1, ty = D - 4, lcx = 4 + r, rcx = 236 - r;   /* ((Vallée)
                                                       * tall: the reels no wider, the meters and counter keep room) */
    int32_t span = r - r / 2 - 3, prog = song.playing ? (int32_t)(cur * 1000u / len) : 0;
    int32_t lp = r / 2 + 2 + span * (1000 - prog) / 1000, rp = r / 2 + 2 + span * prog / 1000;
    uint32_t ph = song.playing && xe_anim() ? ui.frame * 0x03000000u : 0x08000000u;
    uint16_t tape = ux_mix(T_THEME, T_BG, 78);
    int sing = xe_sings(t), muted = t->p[P_MUTE] != 0;
    char d[8];
    cv_rect(0, 0, 240, D, T_BG);
    cv_rect(3, 0, 234, D - 1, ux_mix(T_BG, T_SURF, 45));
    if (!UX_TEEN)
        xe_stars(240, D);
    /* the tape: from under the left pack round a roller, along past the head, round a roller up to the right pack */
    cr_stroke(lcx, cy + lp, lcx + r / 2 + 6, ty, tape);
    cr_stroke(rcx, cy + rp, rcx - r / 2 - 6, ty, tape);
    cv_rect(lcx + r / 2 + 6, ty, rcx - lcx - r - 12, 2, ux_mix(tape, T_TEXT, 15));
    xe_reel(lcx, cy, r, lp, ph, tape);
    xe_reel(rcx, cy, r, rp, ph + 0x15555555u, tape);
    xe_disc(lcx + r / 2 + 6, ty, 2, T_MID);
    xe_disc(rcx - r / 2 - 6, ty, 2, T_MID);
    /* the head and its window (the lanes' window under it) */
    cv_rect(XE_HEAD_X - 6, ty - 5, 13, 5, T_RAISE);
    cv_rect(XE_HEAD_X - 4, ty - 4, 1, D - (ty - 4), T_ACCENT);
    cv_rect(XE_HEAD_X + 4, ty - 4, 1, D - (ty - 4), T_ACCENT);
    /* on the head the selected track's alien, twice its size: a halo while it sings (TEEN: the head alone) */
    if (!UX_TEEN) {
        int32_t ax = XE_HEAD_X - 12, ay = ty - 6 - 24 - (sing ? 1 : 0);
        if (ay < 1)
            ay = 1;
        if (sing && !ui_ph_line) {                   /* ((Skiff #27) PLAYHEAD LINE: no halo, no ripples) */
            xe_dots(XE_HEAD_X, ay + 12, 15, 20u, ui.frame * 0x01000000u, ux_mix(T_ACCENT, T_BG, 30));
            cv_rect(ax - 6, ay + 10, 2, 1, T_ACCENT);
            cv_rect(ax - 8, ay + 13, 3, 1, ux_mix(T_ACCENT, T_BG, 40));
            cv_rect(ax + 28, ay + 10, 2, 1, T_ACCENT);
            cv_rect(ax + 29, ay + 13, 3, 1, ux_mix(T_ACCENT, T_BG, 40));
        }
        uint8_t art[XB_ART_N];
        xe_track_art(t, art);
        xe_alien(ax, ay, art, muted ? T_DIM : T_TRK((song.sel % NTRK) % NTRK), T_BG, muted ? 2 : sing ? 1 : xe_blink(song.sel) ? 3 : 0, 2);
    }
    /* left of the head: a meter a track, lit while its step strikes; right: the tape counter (the step, 3 digits) */
    {
        int32_t x0 = lcx + r + 6, x1 = XE_HEAD_X - 18, bw = (x1 - x0 - 5 * 3) / 6, bh = ty - 10 > 2 * r ? 2 * r : ty - 10,
                by = ty - 10 > 2 * r ? cy - r : 5;     /* ((Vallée) tall: as tall as the reels, beside them) */
        if (bw > 6)
            bw = 6;
        for (k = 0; k < NTRK && bw > 1; k++) {
            const track_t *u = &trk[k];
            int lit = xe_sings(u);
            int32_t bx = x0 + (int32_t)k * (bw + 3), lv = lit ? bh : bh / 5;
            cv_rect(bx, by, bw, bh, T_LINE);
            cv_rect(bx, by + bh - lv, bw, lv, u->p[P_MUTE] ? T_DIM : lit ? T_TRK(k) : ux_mix(T_TRK(k), T_BG, 55));
        }
    }
    {
        int32_t x0 = XE_HEAD_X + 18, b[4];
        uint32_t n = cur + 1u;
        d[0] = (char)('0' + n / 100u % 10u);
        d[1] = (char)('0' + n / 10u % 10u);
        d[2] = (char)('0' + n % 10u);
        d[3] = 0;
        text_ink(&AF_M, d, b);
        cv_text_on(x0 + (rcx - r - 6 - x0 - b[2]) / 2 - b[0], xe_teen_tall() ? XE_TEEN_TOP + CAP_IN(M, ty - 2 - XE_TEEN_TOP) :
                   CAP_IN(M, ty - 2), &AF_M, d, song.playing ? T_THEME : T_MID, ux_mix(T_BG, T_SURF, 45));   /* ((Vallée)
                                                       * tall: under the seam, the cards' row's) */
    }
}

/* ((Vallée) TEEN's lanes, 9 px: a name in a tiny face, 3 x 5 (A..Z 0..9 - . + /, any other a ?), 1 between; to x + w) */
static void xe_tiny(int32_t x, int32_t y, const char *s, uint16_t c, int32_t w)
{
    static const char ORD[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.+/?";
    static const uint16_t T[41] = {11245u,27566u,14627u,27502u,31143u,31140u,14699u,23533u,29847u,4714u,23469u,18727u,24557u,27501u,11114u,27556u,11123u,27565u,14478u,29842u,23407u,23402u,23549u,23213u,23186u,29351u,31599u,11415u,25255u,25230u,23497u,31118u,14831u,29330u,31727u,31694u,448u,2u,1488u,4772u,29314u};
    int32_t x1 = x + w;
    for (; *s && x + 3 <= x1; s++, x += 4) {
        char ch = *s >= 'a' && *s <= 'z' ? (char)(*s - 32) : *s;
        uint32_t g, i;
        if (ch == ' ')
            continue;
        for (g = 0; ORD[g] && ORD[g] != ch; g++)
            ;
        if (g > 40u)
            g = 40u;
        for (i = 0; i < 15u; i++)
            if ((T[g] >> (14u - i)) & 1u)
                cv_pset(x + (int32_t)(i % 3u), y + (int32_t)(i / 3u), c);
    }
}

/* the lanes under the deck, from y; lh tall, 1 between */
static void xe_lanes(int32_t y, int32_t lh)
{
    uint32_t k;
    int32_t i;
    for (k = 0; k < NTRK; k++, y += lh + 1) {
        const track_t *t = &trk[k];
        const step_t *s = t->step;
        uint32_t len, cur = xe_pos(t, &len);
        int sel = k == song.sel, muted = t->p[P_MUTE] != 0, sing = xe_sings(t);
        uint16_t col = muted ? T_DIM : T_TRK(k), lane = sel ? ux_mix(col, T_BG, 72) : T_SURF;
        uint16_t strip = ux_mix(T_BG, lane, 35);
        char name[20], d[2] = {(char)('1' + k), 0};
        int32_t ch = lh - 6 > 3 ? lh - 6 : 3, cy = y + (lh - ch) / 2;
        cv_rect(3, y, 234, lh, lane);
        if (sel)
            cv_rect(3, y, 2, lh, col);
        if (UX_TEEN && lh >= 15)                      /* (TEEN tall: M) */
            cv_text_on(6, y + CAP_IN(M, lh), &AF_M, d, sel ? T_TEXT : col, lane);
        else if (UX_TEEN)                             /* (TEEN: the tiny face) */
            xe_tiny(8, y + (lh - 5) / 2, d, sel ? T_TEXT : col, 4);
        else
            cv_text_on(8, y + CAP_IN(S, lh), &AF_S, d, sel ? T_TEXT : col, lane);
        if (midi_mark(k))                             /* (MIDI IN playing it: a mark after the number) */
            cv_rect(15, y + lh / 2 - 1, 2, 2, T_THEME);
        if (!UX_TEEN) {                               /* (TEEN: no creatures, the name from 18) */
            uint8_t art[XB_ART_N];
            xe_track_art(t, art);
            xe_alien(18, y + (lh - 12) / 2, art, muted ? T_DIM : T_TRK((k) % NTRK), strip, muted ? 2 : sing ? 1 : xe_blink(k) ? 3 : 0, 1);
        }
        sound_name(t, name);
        if (UX_TEEN && lh >= 15)
            cv_free_text(19, y + CAP_IN(M, lh), &AF_M, name[0] ? name : "--", muted ? T_DIM : sel ? T_TEXT : T_MID, lane,
                         XE_TAPE_X0 - 4 - 19);
        else if (UX_TEEN)
            xe_tiny(16, y + (lh - 5) / 2, name[0] ? name : "--", muted ? T_DIM : sel ? T_TEXT : T_MID, XE_TAPE_X0 - 4 - 16);
        else
            cv_free_text(33, y + CAP_IN(S, lh), &AF_S, name[0] ? name : "--", muted ? T_DIM : sel ? T_TEXT : T_MID, lane,
                         XE_TAPE_X0 - 4 - 33);
        /* the tape: its strip, edged */
        cv_rect(XE_TAPE_X0, y + 1, XE_TAPE_X1 - XE_TAPE_X0, lh - 2, strip);
        cv_rect(XE_TAPE_X0, y + 1, XE_TAPE_X1 - XE_TAPE_X0, 1, ux_mix(strip, T_TEXT, 10));
        for (i = -4; i <= 16; i++) {                  /* the step playing in the window, the ones played left of it */
            int32_t x = XE_HEAD_X - XE_CELL_W / 2 + i * XE_PITCH;
            uint32_t idx = (cur + len * 4u + (uint32_t)i) % len;
            const step_t *st = &s[idx];
            uint16_t on = i < 0 ? ux_mix(col, strip, 50) : col;
            if (x < XE_TAPE_X0 + 1 || x + XE_CELL_W > XE_TAPE_X1 - 1)
                continue;
            if (idx % 16u == 0u)                      /* a bar's splice */
                cv_rect(x - 1, y + 1, 1, lh - 2, ux_mix(strip, T_TEXT, 30));
            else if (idx % 4u == 0u)                  /* a beat's tick */
                cv_rect(x - 1, y + lh - 3, 1, 2, ux_mix(strip, T_TEXT, 25));
            if (step_on(st) || st->hit)
                cv_rect(x, cy, XE_CELL_W, ch, on);
            else if (step_tie(st))
                cv_rect(x - 2, cy + ch / 4, XE_CELL_W + 2, ch / 2 > 1 ? ch / 2 : 1, on);
            else
                cv_rect(x + XE_CELL_W / 2, y + lh / 2, 1, 1, i < 0 ? T_LINE : T_DIM);
        }
        cv_rect(XE_HEAD_X - 4, y, 1, lh, T_ACCENT);   /* the head's window */
        cv_rect(XE_HEAD_X + 4, y, 1, lh, T_ACCENT);
    }
}

/* ((Vallée) TEEN's HOME tall, and XENO's: no cards, the deck and the lanes from the cards' row down (XE_TEEN_TOP more rows), drawn
 * in two pieces, the top in the cards' row (ui_draw.c draw_columns), the rest in the panel; a knob turned brings the
 * cards back while it is (ui.hot_t). The lanes then 15 px, their names in M) */
static int xe_teen_cards(void) { return xe_teen_tall() && !ui.hot_t; }   /* (the deck's top where the cards were) */
static void xe_home_in(int32_t gh);
static void xe_teen_top(void)                         /* (the cards' row's piece) */
{
    cv_begin(240, XE_TEEN_TOP, T_BG);
    xe_home_in((int32_t)graph_h() + XE_TEEN_TOP);
    cv_blit(0, Y_LABEL);
}
/* HOME, THEME XENO: the deck over the lanes (ui_home.c graph_tracks) */
static void xe_home(void)
{
    if (xe_teen_tall()) {                             /* (TEEN: the panel's piece, under the top's) */
        cv_oy = -XE_TEEN_TOP;
        xe_home_in((int32_t)graph_h() + XE_TEEN_TOP);
        cv_oy = 0;
        return;
    }
    xe_home_in((int32_t)graph_h());
}
static void xe_home_in(int32_t gh)
{
    int32_t lh = xe_teen_tall() ? 15 : UX_TEEN ? XE_LANE_TEEN : XE_LANE_H, lanes = (int32_t)NTRK * (lh + 1) - 1, D;   /* (tall,
                                                       * either theme: 15) */
    if (gh - lanes < 36) {                            /* (no room for the deck: the lanes alone, as tall as fit) */
        lh = (gh - ((int32_t)NTRK - 1)) / (int32_t)NTRK;
        cv_rect(0, 0, 240, gh, T_BG);
        xe_lanes((gh - ((int32_t)NTRK * (lh + 1) - 1)) / 2, lh);
        return;
    }
    if (NTRK <= 4u)                                  /* (four tracks: taller lanes) */
        lh = (gh - 40 - ((int32_t)NTRK - 1)) / (int32_t)NTRK, lanes = (int32_t)NTRK * (lh + 1) - 1;
    D = gh - lanes;
    xe_deck(D);
    xe_lanes(D, lh);
}

/* the splash: a saucer, its beam, a grey in it, on the card between the bands (72 rows, as QUARK's) */
static void xe_splash(int32_t w, int32_t pad)
{
    int32_t y, cx = w / 2;
    xe_stars(w, 72);
    for (y = 30; y < 72; y++) {                       /* the beam: widening, fading to the card */
        int32_t hw = 6 + (y - 30) * 2 / 3;
        cv_rect(cx - hw, y, 2 * hw, 1, ux_mix(T_ACCENT, T_RAISE, 55 + (y - 30)));
    }
    xe_alien(cx - 36 / 2, -3, SPR_XE_SAUCER, T_THEME, T_RAISE, 0, 3);
    xe_alien(cx - 12, 44, SPR_XE_GREY, T_TRK(1), T_RAISE, 1, 2);
    cv_rect(0, 0, pad, 72, T_RAISE);              /* (kept off the margins, as the text is) */
    cv_rect(w - pad, 0, pad, 72, T_RAISE);
}

/* (Vallée) PRESETS turned on HOME (any theme): for BEAST_T frames a black box over the panel, its border the track's
 * colour, the sound's creature as large as the panel lets it (6 times), its name under it and its engine's. Drawn by
 * draw_graph (ui_graph.c) after the page; ui_draw.c counts ui.beast_t down, PRESETS again starts it over */
#define BEAST_T 80u                                   /* (~1.3 s after the last detent) */
static int beast_box_fits(void) { return ui.home && !ui.home_scope && !ui.layer && large_kind() != LK_TALL; }
static uint32_t beast_sig(void)
{
    const track_t *t = TSEL;
    char name[20];
    sound_name(t, name);
    return xb_hash(name) * 131u + t->eng_req * 7u + song.sel + 1u + (t->p[P_MUTE] ? 17u : 0u);
}
/* ((Vallée) TEEN: no creature; the sound's name big (L, or M if it is too wide), the engine under it, on black, a thin
 * line in the track's colour round it) */
static void beast_box_teen(void)
{
    const track_t *t = TSEL;
    int32_t gh = (int32_t)graph_h(), bw, bh, bx, by, b[4], nh;
    uint16_t edge = t->p[P_MUTE] ? T_DIM : T_TRK(song.sel);
    char name[20];
    const char *eng = ENGINES[t->eng_req % NENGINES]->name;
    const aafont_t *f = &AF_L;
    sound_name(t, name);
    if (!name[0])
        str_cpy(name, "--", sizeof name);
    if (text_w(f, name) > 212)
        f = &AF_M;
    nh = f == &AF_L ? 28 : 18;
    bw = text_w(f, name) + 28;
    if (bw < 120)
        bw = 120;
    if (bw > 228)
        bw = 228;
    bh = 12 + nh + 4 + 12 + 10;
    bx = 120 - bw / 2;
    by = (gh - bh) / 2;
    GFX_HOOK_OVERLAY(bx, by, bx + bw, by + bh);
    cv_rect(bx, by, bw, bh, edge);
    cv_rect(bx + 1, by + 1, bw - 2, bh - 2, T_BG);
    text_ink(f, name, b);
    cv_text_fit(120 - b[2] / 2 - b[0], by + 12 + (f == &AF_L ? CAP_IN(L, nh) : CAP_IN(M, nh)), f, name,
                t->p[P_MUTE] ? T_DIM : T_TEXT, T_BG, bw - 12);
    text_ink(&AF_S, eng, b);
    cv_text_on(120 - b[2] / 2 - b[0], by + 12 + nh + 4 + CAP_IN(S, 12), &AF_S, eng, T_THEME, T_BG);
}
static void beast_box_draw(void)
{
    if (UX_TEEN) {
        beast_box_teen();
        return;
    }
    const track_t *t = TSEL;
    int32_t gh = (int32_t)graph_h(), z = (gh - 2 * 10 - 18 - 12) / 12, bw, bh, bx, by, cw, b[4];
    uint8_t art[XB_ART_N];
    uint16_t edge = t->p[P_MUTE] ? T_DIM : T_TRK(song.sel), ink = 0x0000;
    char name[20];
    const char *eng = ENGINES[t->eng_req % NENGINES]->name;
    if (z > 6)
        z = 6;
    if (z < 2)
        return;
    sound_name(t, name);
    if (!name[0])
        str_cpy(name, "--", sizeof name);
    cw = text_w(&AF_M, name);
    if (text_w(&AF_S, eng) > cw)
        cw = text_w(&AF_S, eng);
    bw = (12 * z > cw ? 12 * z : cw) + 28;
    if (bw > 228)
        bw = 228;
    bh = 10 + 12 * z + 6 + 16 + 12 + 6;
    bx = 120 - bw / 2;
    by = (gh - bh) / 2;
    GFX_HOOK_OVERLAY(bx, by, bx + bw, by + bh);       /* (the lanes under it: hidden) */
    cv_rect(bx, by, bw, bh, edge);                    /* the border, 2 px, the track's colour */
    cv_rect(bx + 2, by + 2, bw - 4, bh - 4, ink);     /* black */
    xe_track_art(t, art);
    xe_alien(120 - 6 * z, by + 10, art, t->p[P_MUTE] ? T_DIM : T_TRK((song.sel % NTRK) % NTRK), ink, t->p[P_MUTE] ? 2 : 0, z);
    GFX_HOOK_ALIGN(bx, by + 10 + 12 * z + 6, bx + bw, by + 10 + 12 * z + 22, AL_H, "creature box name");
    text_ink(&AF_M, name, b);
    cv_text_fit(120 - b[2] / 2 - b[0], by + 10 + 12 * z + 6 + CAP_IN(M, 16), &AF_M, name, T_TEXT, ink, bw - 12);
    text_ink(&AF_S, eng, b);
    cv_text_on(120 - b[2] / 2 - b[0], by + 10 + 12 * z + 22 + CAP_IN(S, 12), &AF_S, eng, T_MID, ink);
}
