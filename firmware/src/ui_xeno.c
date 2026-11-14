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
#define XE_LANE_H 13                                  /* a lane, 1 between them: the deck gets what is left (39 or more) */
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
    if (!ENGINES[e]->npresets || !str_eq(ENGINES[e]->presets[n].name, name))
        n = xb_hash(name);
    xe_beast_art(e, n, art);
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

/* the deck: D px tall at the canvas's top */
static void xe_deck(int32_t D)
{
    const track_t *t = TSEL;
    uint32_t len, cur = xe_pos(t, &len), k;
    int32_t r = D / 2 - 3, cy = D / 2 - 1, ty = D - 4, lcx = 4 + r, rcx = 236 - r;
    int32_t span = r - r / 2 - 3, prog = song.playing ? (int32_t)(cur * 1000u / len) : 0;
    int32_t lp = r / 2 + 2 + span * (1000 - prog) / 1000, rp = r / 2 + 2 + span * prog / 1000;
    uint32_t ph = song.playing && xe_anim() ? ui.frame * 0x03000000u : 0x08000000u;
    uint16_t tape = ux_mix(T_THEME, T_BG, 78), sel = T_TRK(song.sel);
    int sing = xe_sings(t), muted = t->p[P_MUTE] != 0;
    char d[8];
    cv_rect(0, 0, 240, D, T_BG);
    cv_rect(3, 0, 234, D - 1, ux_mix(T_BG, T_SURF, 45));
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
    /* on the head the selected track's alien, twice its size: a halo while it sings */
    {
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
        xe_alien(ax, ay, art, muted ? T_DIM : sel, T_BG, muted ? 2 : sing ? 1 : xe_blink(song.sel) ? 3 : 0, 2);
    }
    /* left of the head: a meter a track, lit while its step strikes; right: the tape counter (the step, 3 digits) */
    {
        int32_t x0 = lcx + r + 6, x1 = XE_HEAD_X - 18, bw = (x1 - x0 - 5 * 3) / 6, bh = ty - 10, by = 5;
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
        cv_text_on(x0 + (rcx - r - 6 - x0 - b[2]) / 2 - b[0], CAP_IN(M, ty - 2), &AF_M, d, song.playing ? T_THEME : T_MID,
                   ux_mix(T_BG, T_SURF, 45));
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
        cv_text_on(8, y + CAP_IN(S, lh), &AF_S, d, sel ? T_TEXT : col, lane);
        if (midi_mark(k))                             /* (MIDI IN playing it: a mark after the number) */
            cv_rect(15, y + lh / 2 - 1, 2, 2, T_THEME);
        uint8_t art[XB_ART_N];
        xe_track_art(t, art);
        xe_alien(18, y + (lh - 12) / 2, art, col, strip, muted ? 2 : sing ? 1 : xe_blink(k) ? 3 : 0, 1);
        sound_name(t, name);
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

/* HOME, THEME XENO: the deck over the lanes (ui_home.c graph_tracks) */
static void xe_home(void)
{
    int32_t gh = (int32_t)graph_h(), lh = XE_LANE_H, lanes = (int32_t)NTRK * (XE_LANE_H + 1) - 1, D;
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
static void beast_box_draw(void)
{
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
    xe_alien(120 - 6 * z, by + 10, art, edge, ink, t->p[P_MUTE] ? 2 : 0, z);
    GFX_HOOK_ALIGN(bx, by + 10 + 12 * z + 6, bx + bw, by + 10 + 12 * z + 22, AL_H, "creature box name");
    text_ink(&AF_M, name, b);
    cv_text_fit(120 - b[2] / 2 - b[0], by + 10 + 12 * z + 6 + CAP_IN(M, 16), &AF_M, name, T_TEXT, ink, bw - 12);
    text_ink(&AF_S, eng, b);
    cv_text_on(120 - b[2] / 2 - b[0], by + 10 + 12 * z + 22 + CAP_IN(S, 12), &AF_S, eng, T_MID, ink);
}
