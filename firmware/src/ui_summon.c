/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) SUMMON's EDIT pages (eng_summon.c): SUMMONER on THEME XENO, AURA on TEEN. a key there begins a ceremony
 * (sm_start); sm_tick, every UI frame, lights its candles (SM_CANDLES, a step each SM_STEP_MS of its track playing, sooner the busier the
 * room is played) and at the last puts the being's number into the track's SIGIL and SEAL; a demon's or an angel's
 * arrival is said in the header on any page.
 *   SUMMONER: the eight signals as strands across the top, the circle on the floor with its five points and its
 *   candles, the being built up from its feet as they light (its eyes open at the last; a demon's horns and wings, an
 *   angel's halo and wings of light, red or gold flames, once the last but one is lit); its name over it.
 *   AURA: a ring a signal round a point, one more as each step passes; a demon a red mark pointing down in red rings,
 *   an angel a gold sun and its rays; the signals as eight small bars.
 * ANIM OFF: no strands moving, no flicker; the being appears whole at the end. Included by ui_graph.c. */
static int sm_page(void)
{
    const page_t *pg = cur_page();
    return !ui.home && pg->scope == SC_ENGINE && ENGINES[TSEL->eng_req % NENGINES] == &ENG_SUMMON;
}
static void sm_start(uint32_t note)
{
    uint32_t h = (sm.ent ^ fm1_ticks()) + note * 0x9E3779B9u, k;
    for (k = 0; k < SM_NSIG; k++)
        h = (h ^ sm.sig[k]) * 16777619u;
    h ^= h >> 13;
    sm.next = (uint16_t)(1u + h % 16383u);
    sm.trk = song.sel;
    sm.prog = 0;
    sm.lit = 0;
    sm.t0 = fm1_ms;
    sm.cer = 1;
    sm.rq_note = (uint8_t)note;                         /* (its part, from this key: summon_play.c, in the audio's block) */
    sm.rq_trk = sm.trk;
    sm.on = 1;
    sm.rq = 1;
    sm_touch(1);
    ui.force = 1;
}
/* a key on its page: it begins; going (its track, playing its part): it fades (the notes let go; a ceremony not yet
   done ends without a being, one done keeps it) */
static void sm_key(uint32_t note)
{
    if (sm.on && sm.trk == song.sel) {
        sm.on = 0;
        sm.rq = 2;
        if (sm.cer) {
            sm.cer = 0;
            ui_message("IT FADED");
        } else
            ui_message("IT RESTS");
        ui.force = 1;
        return;
    }
    sm_start(note);
}
static void sm_tick(void)
{
    uint32_t now = fm1_ms, dt = now - sm.t0;
    track_t *t = &trk[sm.trk % NTRK];
    sm_room(now - sm.rt);                               /* (the signals move here, between mixes: eng_summon.c) */
    sm.rt = now;
    if (!sm.cer)
        return;
    sm.t0 = now;
    if (dt > 200u)
        dt = 200u;
    if (song.playing && sm.on)                          /* ((Vallée) it forms over a song: while the song plays, */
        sm.prog += dt + dt * (uint32_t)sm.near / 65536u;   /* up to half again as fast played to) */
    sm.lit = (uint8_t)(sm.prog / SM_STEP_MS);
    if (t->eng_req % NENGINES != ENGI_SUMMON) {        /* (the track's sound changed meanwhile: no ceremony) */
        sm.cer = 0;
        return;
    }
    if (sm.lit >= SM_CANDLES) {
        int32_t kind = sm_kind(sm.next);
        sm.lit = SM_CANDLES;
        sm.cer = 0;
        t->p[P_E6] = (int16_t)(sm.next >> 7);
        t->p[P_E7] = (int16_t)(sm.next & 127u);
        if (kind == 1)
            ui_message("A DEMON ANSWERED");
        else if (kind == 2)
            ui_message("AN ANGEL ANSWERED");
        else {
            char nm[24];
            sm_name(sm.next, nm);
            str_cpy(nm + str_len(nm), " CAME", sizeof nm - str_len(nm));
            ui_message(nm);
        }
        if (t == TSEL)
            sync_reload = 1;
        ui.force = 1;
    }
}
/* the canvas changes: the strands and flames move, a ceremony runs (draw_graph's signature) */
static uint32_t sm_sig(void)
{
    return (sm.cer ? sm.prog * 64u / SM_STEP_MS + 1u : 0u) * 7919u + sm_sigil(TSEL) * 31u + (xe_anim() ? ui.frame / 2u : 0u);
}

static void sm_ellipse(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t c)
{
    uint32_t k;
    int32_t px = cx + rx, py = cy;
    for (k = 1; k <= 40u; k++) {
        uint32_t a = k * 107374182u;                    /* (a fortieth of a turn) */
        int32_t x = cx + rx * QK_COS(a) / 32767, y = cy + ry * QK_SIN(a) / 32767;
        xe_line(px, py, x, y, c);
        px = x;
        py = y;
    }
}
static void sm_tri(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t c)
{
    int32_t y, ymin = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2), ymax = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    for (y = ymin; y <= ymax; y++) {                    /* a scanline: where the three edges cross it */
        int32_t xs[3], n = 0, k, lo = 9999, hi = -9999;
        const int32_t E[3][4] = {{x0, y0, x1, y1}, {x1, y1, x2, y2}, {x2, y2, x0, y0}};
        for (k = 0; k < 3; k++) {
            const int32_t *e = E[k];
            if ((y >= e[1] && y <= e[3]) || (y >= e[3] && y <= e[1]))
                xs[n++] = e[1] == e[3] ? e[0] : e[0] + (e[2] - e[0]) * (y - e[1]) / (e[3] - e[1]);
        }
        for (k = 0; k < n; k++) {
            lo = xs[k] < lo ? xs[k] : lo;
            hi = xs[k] > hi ? xs[k] : hi;
        }
        if (n)
            cv_rect(lo, y, hi - lo + 1, 1, c);
    }
}
static const uint16_t SM_HUE[8] = {WL_RGB(120, 230, 255), WL_RGB(255, 120, 210), WL_RGB(140, 255, 120),
                                   WL_RGB(255, 200, 90), WL_RGB(190, 150, 255), WL_RGB(255, 140, 90),
                                   WL_RGB(110, 200, 255), WL_RGB(150, 255, 200)};
#define SM_RED WL_RGB(255, 50, 60)
#define SM_GOLD WL_RGB(255, 205, 80)

/* what the panel shows: the number (the ceremony's, or the track's), how far (0..SM_CANDLES x 16), its kind */
static uint32_t sm_show(const track_t *t, uint32_t *step)
{
    if (sm.cer && sm.trk == trk_index(t)) {
        *step = sm.prog * 16u / SM_STEP_MS;
        return sm.next;
    }
    *step = SM_CANDLES * 16u;
    return sm_sigil(t);
}

static void sm_summoner(const track_t *t)
{
    const int32_t W = 240, H = (int32_t)cv_h, cx = 120, fy = H - 13, anim = xe_anim();
    uint32_t step, sig = sm_show(t, &step), k, lit = step / 16u, done = step >= SM_CANDLES * 16u, strong = 0;
    int32_t kind = sm_kind(sig), reveal = sig && (lit + 1u >= SM_CANDLES), x, y;
    uint16_t sky = WL_RGB(14, 10, 28), ink = kind == 1 && reveal ? SM_RED : kind == 2 && reveal ? SM_GOLD :
                                                 sig ? SM_HUE[(sig >> 3) % 8u] : T_MID;
    char nm[24];
    cv_rect(0, 0, W, H, sky);
    cv_bg = sky;
    xe_stars(W, H);
    for (k = 0; k < SM_NSIG; k++) {                     /* the eight signals: strands, each its own wobble */
        int32_t amp = 1 + sm.sig[k] / 300, py = 0;
        uint32_t fr = (2u << 24) + k * (9u << 20), ph = (anim ? ui.frame * (0x01400000u + k * 0x00300000u) : 0u) + k * 0x2F000000u;
        uint16_t c = ux_mix(sig ? ink : T_THEME, sky, 25 + 8 * k);
        strong += sm.sig[k] > 600u;
        for (x = 6; x <= W - 8; x += 4) {
            int32_t yy = 5 + (int32_t)k * 3 + amp * QK_SIN(ph + (uint32_t)x * fr) / 32767;
            if (x > 6)
                xe_line(x - 4, py, x, yy, c);
            py = yy;
        }
    }
    sm_ellipse(cx, fy, 64, 9, ux_mix(ink, sky, 65));   /* the circle on the floor, its five points */
    sm_ellipse(cx, fy, 54, 6, ux_mix(ink, sky, 40));
    for (k = 0; k < 5u; k++) {
        uint32_t a = 3221225472u + k * 1717986918u, b = a + 1717986918u;   /* (two fifths on: a star of five) */
        xe_line(cx + 54 * QK_COS(a) / 32767, fy + 6 * QK_SIN(a) / 32767, cx + 54 * QK_COS(b) / 32767,
                fy + 6 * QK_SIN(b) / 32767, ux_mix(ink, sky, 50));
    }
    if (sig) {                                          /* the being, from its feet up as the candles light */
        uint8_t px[16][16];
        int32_t z = H >= 110 ? 4 : 3, bx = cx - 8 * z, by = fy - 4 - 16 * z;
        uint32_t rows = !anim && !done ? 0u : done ? 16u : step / SM_CANDLES;   /* (feet first, across all the candles) */
        uint16_t body = kind == 1 && reveal ? WL_RGB(200, 30, 40) : kind == 2 && reveal ? WL_RGB(255, 240, 200) : ink;
        uint16_t line = ux_mix(body, sky, 45), eye = kind == 1 && reveal ? WL_RGB(255, 230, 60) : T_TEXT;
        if (reveal && kind == 1) {                      /* a demon: wings of skin, horns */
            for (k = 0; k < 2u; k++) {
                int32_t s = k ? 1 : -1;
                sm_tri(cx + s * 3 * z, by + 6 * z, cx + s * 16 * z, by - z, cx + s * 14 * z, by + 9 * z, WL_RGB(90, 10, 20));
                sm_tri(cx + s * 3 * z, by + 6 * z, cx + s * 14 * z, by + 9 * z, cx + s * 6 * z, by + 11 * z, WL_RGB(70, 8, 16));
                sm_tri(cx + s * 2 * z, by + 2 * z, cx + s * 5 * z, by - 3 * z, cx + s * 4 * z, by + 2 * z, WL_RGB(220, 220, 200));
            }
        } else if (reveal && kind == 2) {               /* an angel: a halo, wings of light */
            sm_ellipse(cx, by - 2 * z, 4 * z, z + 1, SM_GOLD);
            for (k = 0; k < 10u; k++) {
                int32_t s = k & 1u ? 1 : -1, i = (int32_t)(k / 2u);
                xe_line(cx + s * 6 * z, by + (5 + i) * z, cx + s * (15 - i * 2) * z, by + (i * 2) * z, ux_mix(SM_GOLD, sky, 90 - i * 14));
            }
        }
        sm_being(sig, 16, px);
        for (y = 0; y < 16; y++) {
            if ((uint32_t)(16 - y) > rows)
                continue;
            for (x = 0; x < 16; x++) {
                uint32_t v = px[y][x];
                if (!v)
                    continue;
                cv_rect(bx + x * z - 1, by + y * z - 1, z + 2, z + 2, line);
            }
        }
        for (y = 0; y < 16; y++) {
            if ((uint32_t)(16 - y) > rows)
                continue;
            for (x = 0; x < 16; x++) {
                uint32_t v = px[y][x];
                if (v)
                    cv_rect(bx + x * z, by + y * z, z, z, v == 2u && done ? eye : ux_mix(body, line, 100 - y * 2));
            }
        }
    } else {
        int32_t b[4];
        text_ink(&AF_M, "A KEY: SUMMON", b);
        cv_text_on(cx - b[2] / 2 - b[0], H / 2 - 8, &AF_M, "A KEY: SUMMON", T_MID, sky);
    }
    for (k = 0; k < SM_CANDLES; k++) {                  /* the candles round it, lit one by one */
        int32_t x0 = cx - 70 + (int32_t)k * 28, on = sig && k < lit;
        uint16_t fl = kind == 1 && reveal ? SM_RED : kind == 2 && reveal ? SM_GOLD : WL_RGB(255, 180, 70);
        cv_rect(x0, fy - 3, 2, 8, WL_RGB(225, 220, 200));
        if (on) {
            int32_t f = anim ? (int32_t)((ui.frame / 3u + k * 5u) % 3u) : 1;
            cv_rect(x0, fy - 6 - (f == 2), 2, 2 + (f == 1), fl);
            cv_pset(x0 + (f == 0), fy - 7 - (f == 2), WL_RGB(255, 240, 200));
        }
    }
    if (sig && done) {                                  /* its name, over it */
        int32_t b[4];
        sm_name(sig, nm);
        text_ink(&AF_M, nm, b);
        cv_text_on(cx - b[2] / 2 - b[0], 31, &AF_M, nm, ink, sky);
    }
    if (reveal && kind > 0) {
        cv_text_on(8, 48, &AF_M, kind == 1 ? "A DEMON" : "AN ANGEL", kind == 1 ? SM_RED : SM_GOLD, sky);
        cv_text_on(8, 65, &AF_S, "1 IN 50", ux_mix(kind == 1 ? SM_RED : SM_GOLD, sky, 60), sky);
    }
    {
        char b[12] = "SIGNALS ";
        b[8] = (char)('0' + strong);
        b[9] = 0;
        cv_text_r(W - 8, 31, &AF_S, b, T_MID, sky);
    }
}

static void sm_aura(const track_t *t)
{
    const int32_t W = 240, H = (int32_t)cv_h, cx = 120, cy = H / 2 + 6, anim = xe_anim();
    uint32_t step, sig = sm_show(t, &step), k, done = step >= SM_CANDLES * 16u;
    int32_t kind = sm_kind(sig), reveal = sig && (step / 16u + 1u >= SM_CANDLES);
    uint32_t rings = !sig ? 0u : done || !anim ? SM_NSIG : step * SM_NSIG / (SM_CANDLES * 16u);
    uint16_t c = kind == 1 ? SM_RED : SM_GOLD;
    cv_rect(0, 0, W, H, T_BG);
    cv_bg = T_BG;
    if (reveal && kind == 2) {                          /* an angel: a gold sun, its rays */
        for (k = 0; k < 24u; k++) {
            uint32_t a = k * 178956971u + (anim ? ui.frame * 0x00400000u : 0u);
            xe_line(cx + 22 * QK_COS(a) / 32767, cy + 22 * QK_SIN(a) / 32767, cx + 56 * QK_COS(a) / 32767,
                    cy + 56 * QK_SIN(a) / 32767, k & 1u ? SM_GOLD : ux_mix(SM_GOLD, T_BG, 45));
        }
        xe_disc(cx, cy, 17, SM_GOLD);
        xe_ring(cx, cy, 28, T_TEXT);
    } else if (reveal && kind == 1) {                   /* a demon: red rings, a mark pointing down */
        for (k = 0; k < 6u; k++)
            xe_ring(cx, cy, 10 + (int32_t)k * 8, ux_mix(SM_RED, T_BG, 90 - (int32_t)k * 13));
        sm_tri(cx - 18, cy - 13, cx + 18, cy - 13, cx, cy + 18, SM_RED);
    } else {                                            /* a ring a signal, sized and set off by it */
        for (k = 0; k < rings; k++) {
            int32_t r = 54 - (int32_t)k * 6 + (int32_t)(sm.sig[k] >> 8) - 2;
            int32_t dx = ((int32_t)((sig >> k) & 7u) - 3), dy = ((int32_t)((sig >> (k + 3u)) & 7u) - 3);
            xe_ring(cx + dx, cy + dy, r, T_TRK(k % NTRK));
            if (k & 1u)
                xe_ring(cx + dx, cy + dy, r - 1, T_TRK(k % NTRK));
        }
        if (sig)
            xe_disc(cx, cy, 3, T_TEXT);
        else {
            int32_t b[4];
            text_ink(&AF_M, "A KEY: AURA", b);
            cv_text_on(cx - b[2] / 2 - b[0], cy - 8, &AF_M, "A KEY: AURA", T_MID, T_BG);
        }
    }
    if (reveal && kind > 0) {
        int32_t b[4];
        const char *s = kind == 1 ? "A DEMON" : "AN ANGEL";
        text_ink(&AF_M, s, b);
        cv_text_on(cx - b[2] / 2 - b[0], 2, &AF_M, s, c, T_BG);
    } else if (sig && done) {
        char nm[24];
        int32_t b[4];
        sm_name(sig, nm);
        text_ink(&AF_M, nm, b);
        cv_text_on(cx - b[2] / 2 - b[0], 2, &AF_M, nm, T_TEXT, T_BG);
    }
    for (k = 0; k < SM_NSIG; k++) {                     /* the signals: eight small bars */
        int32_t h = 2 + (int32_t)sm.sig[k] / 70;
        cv_rect(8 + (int32_t)k * 5, H - 4 - h, 3, h, reveal && kind > 0 ? c : T_TRK(k % NTRK));
    }
}
static void graph_summon(const track_t *t)
{
    cv_oy = 0;
    if (UX_TEEN)
        sm_aura(t);
    else
        sm_summoner(t);
}
