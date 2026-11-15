/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) THE SPELL: HOME tapped on HOME, in place of the scope (its own place the cards' too: the song's map).
 *   - At the top (wl_strip, where HOME's cards are): the song as SEQ > SONG has it, every entry a block as long as its
 *     bars, the same music the same colour (an entry whose tracks all play what an earlier one played: its colour, its
 *     name), named as GEN named them (INTRO VERSE CHORUS BRIDGE OUTRO: vgen.c gen_sec_name) or A B C .. by first
 *     appearance; the playhead running over it, the part it is in lit, its name and the bar. SONG off or no lists:
 *     one LOOP block, the playhead the selected track's.
 *   - Below (wl_world): THE ALCHEMIST (the intro's head) conjures with the six tracks, each its own element:
 *       1 green aliens  2 blue cryptids  3 yellow psi orbs  4 pink crystals  5 purple sigils  6 orange mythic herbs.
 *     A track playing (a note on its step, not muted) sends one of its element from its jar (the shelf at the foot) in
 *     an arc into the circle: at most one a track a beat, eight in the air. Each one taken in becomes part of what the
 *     spell summons: it settles into the conjured image (a 12 x 12 mirrored figure, 3x: each element a cell in its
 *     colour, placed by the spell's running hash where the figure grows, beside what is there), and once a track has
 *     given enough of itself its trait shows (aliens: its eyes; cryptids: horns; orbs: a halo; crystals: a crown;
 *     sigils: a rune ring; herbs: vines). The hash starts from the six sounds (engine, preset, user sound) and takes in
 *     every element in turn with its note and when it came: another song, other sounds, another order, another image;
 *     the same song played again from the top, the same one. Each PLAY begins a new spell; stopped, the image stays. ANIM OFF: nothing flies, the elements go straight in.
 * State ~0.2 KB. Drawn by graph_scope (ui_graph.c, the panel's height) and draw_columns (ui_draw.c, the cards' row). */

#define WL_NF 8u                                       /* elements in the air at once */
#define WL_FT 18u                                      /* a flight's frames */
#define WL_GN 12u                                      /* the figure: rows, and columns (mirrored: 6 kept) */
#define WL_GH 6u
#define WL_TRAIT 6u                                    /* a track's elements before its trait shows */
#define WL_RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
static const uint16_t WL_COL[NTRK] = {WL_RGB(90, 255, 110), WL_RGB(80, 160, 255), WL_RGB(255, 230, 70),
                                      WL_RGB(255, 110, 200), WL_RGB(180, 110, 255), WL_RGB(255, 150, 50)};
static const char *const WL_ELEM[NTRK] = {"ALIENS", "CRYPTIDS", "PSI ORBS", "CRYSTALS", "SIGILS", "HERBS"};
static struct {
    uint16_t idx[NTRK];                                /* each track's step at the last frame (a strike: a new one, on) */
    uint8_t was_playing;
    uint32_t run;                                      /* song.tick when last drawn (less now: PLAY again) */
    uint32_t seed;                                     /* the spell's running hash */
    uint16_t cnt[NTRK];                                /* each track's elements taken in */
    uint32_t total;
    uint16_t last_q[NTRK];                             /* (one a track a beat: the quarter of its pattern it last gave in, + 1) */
    struct { uint8_t k, t, note; } fly[WL_NF];         /* t 0: none */
    uint8_t flash, flash_k;                            /* an element taken in: the circle flares */
    uint8_t cell[WL_GN][WL_GH];                        /* the figure: 0 empty, else the track + 1 */
    uint8_t order[WL_GN * WL_GH], nmask, shown;        /* its cells (the shape the sounds give) in the order they appear */
    uint8_t allow;                                     /* how many may show by now: as far as the song is (whole at its end) */
    uint8_t got[NTRK];                                 /* each track's cells of it */
} wl;

static uint32_t wl_h(uint32_t v) { return (v ^ (v >> 15)) * 2654435761u; }
static int32_t wl_abs_i(int32_t v) { return v < 0 ? -v : v; }
/* a dotted ellipse (n dots) */
static void wl_oval_i(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint32_t n, uint16_t c)
{
    uint32_t k;
    for (k = 0; k < n; k++) {
        uint32_t a = k * (0xFFFFFFFFu / n);
        cv_pset(cx + rx * QK_COS(a) / 32767, cy + ry * QK_SIN(a) / 32767, c);
    }
}
/* the alchemist's place this frame (the head's sprite's top left, 2x: 44 x 52); its third eye at +22, +14 */
static void wl_head_at(uint32_t now, int anim, int32_t *hx, int32_t *hy)
{
    *hx = 120 - 22 + (anim ? QK_SIN(now * 262144u) * 3 / 32767 : 0);
    *hy = 1 + (anim ? QK_SIN(now * 458752u) * 2 / 32767 : 0);
}

/* the elements, 9 x 9 */
static void wl_elem(int32_t x, int32_t y, uint32_t k, uint16_t c)
{
    static const char *const E[NTRK] = {
        "..#####...#######.##..#..###...#...##########.#######...#.#.#.....###.......#....",   /* an alien */
        "...###.....#####....#.#.#....#####...#######.#.#####.##.#####.#..##.##....##.##..",   /* a cryptid */
        "#...#...#.#.....#....###.....#####..#.#####.#..#####.....###....#.....#.#...#...#",   /* a psi orb */
        "....#.......###.....##.##...##.#.##..#..#..#..##.#.##...##.##.....###.......#....",   /* a crystal */
        "#...#...#.#..#..#...#.#.#.....###...#########...###.....#.#.#...#..#..#.#...#...#",   /* a sigil */
        "....#......#.#.#.....###....#..#..#...#.#.#.....###.......#.......#.#.....#...#..",   /* a herb */
    };
    const char *s = E[k % NTRK];
    uint32_t i;
    for (i = 0; i < 81u && s[i]; i++)
        if (s[i] == '#')
            cv_pset(x + (int32_t)(i % 9u), y + (int32_t)(i / 9u), c);
}

/* ------------------------------------------------------------------ the song's map --- */
/* the song's parts on the longest list (bank.c song_at): their starts and lengths (48ths of a beat x 4: 192 a bar),
 * a group each (the same music: the same group), where the playhead is; 0 parts: no song (the LOOP) */
#define WL_NSEC NENT
static uint32_t wl_song(uint32_t *at0, uint32_t *len, uint8_t *grp, uint8_t *ent, uint32_t *pos, uint32_t *total)
{
    uint32_t kb, e, n = 0, ng = 0, sig[WL_NSEC], j, k;
    *total = song_at(&kb, pos);
    if (!*total || !lists.song)
        return 0;
    for (e = 0; e < NENT; e++) {
        uint32_t l = ent_len48(kb, e) * ent_reps(kb, e), s = 0x811C9DC5u;
        if (!ent_on(kb, e) || !l)
            continue;
        for (k = 0; k < NTRK; k++)
            s = (s ^ lists.ent[k][e][0]) * 16777619u;
        at0[n] = ent_at48(kb, e);
        len[n] = l;
        ent[n] = (uint8_t)e;
        for (j = 0; j < n && sig[j] != s; j++)
            ;
        grp[n] = j < n ? grp[j] : (uint8_t)ng++;
        sig[n] = s;
        n++;
    }
    return n;
}
/* a part's name: GEN SONG's layout (eight parts, five musics: A B C B C D C E) by GEN's names, else A B C .. */
static int wl_gen_layout(const uint8_t *grp, uint32_t n)
{
    static const uint8_t L[8] = {0, 1, 2, 1, 2, 3, 2, 4};
    uint32_t i;
    if (n != 8u)
        return 0;
    for (i = 0; i < 8u; i++)
        if (grp[i] != L[i])
            return 0;
    return 1;
}
static void wl_sec_name(char *b, uint32_t i, uint32_t g, int gen)
{
    static const char *const GN[8] = {"INTRO", "VERSE", "CHORUS", "VERSE", "CHORUS", "BRIDGE", "CHORUS", "OUTRO"};
    if (gen && i < 8u)
        str_cpy(b, GN[i], 8);
    else {
        b[0] = (char)('A' + g % 26u);
        b[1] = 0;
    }
}
static const uint16_t WL_SEC[8] = {WL_RGB(64, 200, 190), WL_RGB(240, 110, 90), WL_RGB(240, 200, 70),
                                   WL_RGB(150, 120, 255), WL_RGB(90, 170, 255), WL_RGB(150, 230, 90),
                                   WL_RGB(255, 130, 200), WL_RGB(220, 180, 140)};
static uint32_t wl_strip_sig;
static int wl_on(void) { return ui.home && ui.home_scope && large_kind() != LK_TALL && !ux.quark && !ux.crypt && !ux.skiff; }
/* the cards' row (y 28, 240 x 46): the map */
static void wl_strip(void)
{
    uint32_t at0[WL_NSEC], len[WL_NSEC], pos, total, n, i, cur = 0, sig, bar, bars;
    uint8_t grp[WL_NSEC], ent[WL_NSEC];
    char nm[24], b[20];
    const int32_t X0 = 6, W = 228, Y = 20, H = 16;
    int gen;
    n = wl_song(at0, len, grp, ent, &pos, &total);
    if (!n) {                                          /* the LOOP: the selected track's pattern */
        uint32_t l = TSEL->p[P_SLEN] > 0 ? (uint32_t)TSEL->p[P_SLEN] : 16u;
        total = l;
        pos = song.playing ? TSEL->seq_idx % l : 0u;
    }
    for (i = 0; i < n; i++)
        if (pos >= at0[i] && pos < at0[i] + len[i])
            cur = i;
    sig = (pos * 228u / (total ? total : 1u)) * 31u + n * 977u + cur * 7u + (uint32_t)song.playing * 5u + total;
    for (i = 0; i < n; i++)
        sig = sig * 31u + at0[i] + grp[i] * 13u + ent[i];
    if (!ui.force && sig == wl_strip_sig)
        return;
    wl_strip_sig = sig;
    cv_begin(240, 46, T_BG);
    cv_rrect(3, 0, 234, 46, 5, T_SURF, T_BG);
    cv_bg = T_SURF;
    gen = wl_gen_layout(grp, n);
    if (n) {
        wl_sec_name(nm, cur, grp[cur], gen);
        bar = pos / 192u + 1u;
        bars = (total + 191u) / 192u;
    } else {
        str_cpy(nm, "LOOP", sizeof nm);
        bar = 1;
        bars = 1;
    }
    cv_text_on(8, 3, &AF_S, nm, n ? WL_SEC[grp[cur] % 8u] : T_THEME, T_SURF);
    str_cpy(b, "BAR ", sizeof b);
    fmt_int(b + 4, (int32_t)bar);
    str_cpy(b + str_len(b), " / ", sizeof b - str_len(b));
    fmt_int(b + str_len(b), (int32_t)bars);
    cv_text_r(232, 3, &AF_S, b, T_MID, T_SURF);
    if (!n) {
        cv_rect(X0, Y, W, H, ux_mix(T_SURF, T_THEME, 45));
    } else {
        for (i = 0; i < n; i++) {                       /* the parts: as long as they are, the same music one colour */
            int32_t x0 = X0 + (int32_t)(at0[i] * (uint32_t)W / total), x1 = X0 + (int32_t)((at0[i] + len[i]) * (uint32_t)W / total);
            uint16_t c = WL_SEC[grp[i] % 8u];
            cv_rect(x0, Y, x1 - x0 - 1, H, i == cur ? c : ux_mix(T_SURF, c, 55));
            if (x1 - x0 > 14) {                         /* its name, where it fits; else its first letter */
                char s[8];
                wl_sec_name(s, i, grp[i], gen);
                if (text_w(&AF_S, s) > x1 - x0 - 4)
                    s[1] = 0;
                if (text_w(&AF_S, s) <= x1 - x0 - 3)
                    cv_text_on(x0 + 2, Y + 2, &AF_S, s, i == cur ? T_BG : ux_mix(T_SURF, T_TEXT, 70), i == cur ? c : ux_mix(T_SURF, c, 55));
            }
        }
    }
    {   /* the playhead */
        int32_t x = X0 + (int32_t)((uint64_t)pos * (uint32_t)W / (total ? total : 1u));
        cv_rect(x, Y - 3, 2, H + 6, T_TEXT);
        cv_rect(X0, Y + H + 3, x - X0, 2, T_THEME);
    }
    cv_blit(0, Y_LABEL);
}

/* ------------------------------------------------------------------ the spell --- */
static void wl_begin(void)                             /* a new spell: from the six sounds */
{
    uint32_t k, s = 0x5EED0A1Cu;
    for (k = 0; k < NTRK; k++)
        s = wl_h(s ^ (trk[k].eng_req * 131u + trk[k].preset * 7u + trk[k].user * 977u + k * 0x9E37u));
    wl.seed = s;
    {   /* the shape the six sounds give: a body's width row by row (a walk), a hole or two, arms; its cells shown from the
         * heart outward, in the order the spell's hash breaks ties */
        uint8_t m[WL_GN][WL_GH], d[WL_GN * WL_GH];
        uint32_t r, c, h = wl_h(s ^ 0xA5A5u), n = 0, i, j;
        int32_t w = 2 + (int32_t)(h % 3u);
        memset(m, 0, sizeof m);
        for (r = 0; r < WL_GN; r++) {
            h = wl_h(h + r);
            w += (int32_t)(h % 3u) - 1;
            w = w < 1 ? 1 : w > (int32_t)WL_GH ? (int32_t)WL_GH : w;
            if (r == 0u || r == WL_GN - 1u)
                w = w > 3 ? 3 : w;
            for (c = WL_GH - (uint32_t)w; c < WL_GH; c++)
                m[r][c] = 1;
            if ((h >> 20) % 4u == 0u && w < (int32_t)WL_GH - 1)   /* an arm */
                m[r][WL_GH - (uint32_t)w - 2u] = m[r][WL_GH - (uint32_t)w - 1u] = 1;
        }
        for (i = 0; i < 2u; i++) {                      /* a hole or two (eyes, mouths) */
            h = wl_h(h + 99u + i);
            r = 2u + h % 7u;
            c = WL_GH - 2u - (h >> 8) % 2u;
            m[r][c] = 0;
        }
        for (r = 0; r < WL_GN; r++)
            for (c = 0; c < WL_GH; c++)
                if (m[r][c]) {
                    int32_t dr = (int32_t)r - 5, dc = (int32_t)WL_GH - 1 - (int32_t)c;
                    wl.order[n] = (uint8_t)(r * WL_GH + c);
                    d[n] = (uint8_t)((uint32_t)(dr * dr + dc * dc * 2) * 4u + (wl_h(s + r * 31u + c) & 3u));
                    n++;
                }
        for (i = 1; i < n; i++)                         /* (by distance: an insertion sort, 72 at most) */
            for (j = i; j > 0 && d[j - 1] > d[j]; j--) {
                uint8_t t = d[j]; d[j] = d[j - 1]; d[j - 1] = t;
                t = wl.order[j]; wl.order[j] = wl.order[j - 1]; wl.order[j - 1] = t;
            }
        wl.nmask = (uint8_t)n;
        wl.shown = 0;
    }
    wl.total = 0;
    memset(wl.cnt, 0, sizeof wl.cnt);
    memset(wl.got, 0, sizeof wl.got);
    memset(wl.cell, 0, sizeof wl.cell);
    memset(wl.fly, 0, sizeof wl.fly);
    memset(wl.last_q, 0, sizeof wl.last_q);
    memset(wl.idx, 0xFF, sizeof wl.idx);              /* (the first step counts too) */
}
/* track k's element taken in (its note): the hash takes it; the figure's next cell appears in its colour (once the
 * figure is whole, the elements feed its glow) */
static void wl_add(uint32_t k, uint32_t note)
{
    wl.total++;
    wl.cnt[k]++;
    wl.seed = wl_h(wl.seed ^ ((k + 1u) * 0x9E3779B9u) ^ (note * 131u) ^ (wl.total * 7919u));
    while (wl.shown < wl.nmask && wl.shown < wl.allow) {   /* (the cells due now: each to the track furthest behind
                                                          * its share of the elements, so the figure is the song's mix) */
        uint32_t at = wl.order[wl.shown++], j, best = k;
        int32_t bd = -0x7FFFFFFF;
        for (j = 0; j < NTRK; j++) {
            int32_t d = (int32_t)(wl.cnt[j] * wl.shown) - (int32_t)(wl.got[j] * wl.total);
            if (wl.cnt[j] && d > bd) {
                bd = d;
                best = j;
            }
        }
        wl.got[best]++;
        wl.cell[at / WL_GH][at % WL_GH] = (uint8_t)(best + 1u);
    }
    wl.flash = 6;
    wl.flash_k = (uint8_t)k;
}
/* the jars on the shelf: their places */
static int32_t wl_jar_x(uint32_t k) { return 10 + (int32_t)k * 38; }

/* the figure the spell summons, 4x (48 x 48), its middle at cx, its top at y: each cell its element's colour, lighter
 * up (lit from above), a dark rim round it; then the traits of the tracks that gave enough of themselves */
#define WL_S 4
static void wl_figure(int32_t cx, int32_t y, uint32_t now, int anim, uint16_t rim)
{
    uint32_t r, c, k, side;
    int32_t x0 = cx - (int32_t)WL_GH * WL_S;
    for (side = 0; side < 2u; side++)                   /* the rim first: each cell one wider all round */
        for (r = 0; r < WL_GN; r++)
            for (c = 0; c < WL_GH; c++)
                if (wl.cell[r][c]) {
                    int32_t x = side ? cx + (int32_t)(WL_GH - 1u - c) * WL_S : x0 + (int32_t)c * WL_S;
                    cv_rect(x - 1, y + (int32_t)r * WL_S - 1, WL_S + 2, WL_S + 2, rim);
                }
    for (r = 0; r < WL_GN; r++)
        for (c = 0; c < WL_GH; c++)
            if (wl.cell[r][c]) {
                uint16_t col = WL_COL[(wl.cell[r][c] - 1u) % NTRK];
                uint16_t lit = ux_mix(col, WL_RGB(255, 255, 255), 30 - (int32_t)r * 4);
                uint16_t sh = ux_mix(col, rim, 35);
                for (side = 0; side < 2u; side++) {
                    int32_t x = side ? cx + (int32_t)(WL_GH - 1u - c) * WL_S : x0 + (int32_t)c * WL_S, yy = y + (int32_t)r * WL_S;
                    cv_rect(x, yy, WL_S, WL_S, r < 4u ? lit : col);
                    cv_rect(x, yy + WL_S - 1, WL_S, 1, sh);
                }
            }
    for (k = 0; k < NTRK; k++) {
        uint16_t col = WL_COL[k];
        if (wl.cnt[k] < WL_TRAIT)
            continue;
        switch (k) {
        case 0:                                        /* aliens: two great black almond eyes, a glint */
            cv_rect(cx - 11, y + 13, 7, 4, rim);
            cv_rect(cx + 4, y + 13, 7, 4, rim);
            cv_rect(cx - 10, y + 12, 5, 1, rim);
            cv_rect(cx + 5, y + 12, 5, 1, rim);
            cv_pset(cx - 7, y + 14, col);
            cv_pset(cx + 7, y + 14, col);
            break;
        case 1:                                        /* cryptids: horns */
            xe_line(cx - 12, y + 2, cx - 20, y - 10, col);
            xe_line(cx - 11, y + 2, cx - 19, y - 10, col);
            xe_line(cx + 11, y + 2, cx + 19, y - 10, col);
            xe_line(cx + 10, y + 2, cx + 18, y - 10, col);
            break;
        case 2:                                        /* psi orbs: a halo */
            wl_oval_i(cx, y - 6, 15, 3, 40u, col);
            break;
        case 3:                                        /* crystals: a crown of shards */
            for (c = 0; c < 5u; c++)
                xe_line(cx - 12 + (int32_t)c * 6, y - 1, cx - 10 + (int32_t)c * 6, y - 6 - (int32_t)(c % 2u) * 4, col);
            break;
        case 4:                                        /* sigils: a rune ring turning */
            xe_dots(cx, y + 24, 33, 14u, anim ? now * 2097152u : 0u, col);
            break;
        default:                                       /* herbs: vines from its foot */
            xe_line(cx - 8, y + 47, cx - 22, y + 38, col);
            xe_line(cx + 7, y + 47, cx + 21, y + 38, col);
            xe_line(cx - 22, y + 38, cx - 24, y + 32, col);
            xe_line(cx + 21, y + 38, cx + 23, y + 32, col);
            break;
        }
    }
}

static void wl_world(void)
{
    const int32_t H = (int32_t)cv_h, cx = 120, cyc = H - 27;       /* (the circle's middle) */
    const int anim = xe_anim();
    const uint32_t now = anim ? fm1_ms : 0u;
    const uint16_t sky = WL_RGB(14, 10, 28), floor = WL_RGB(24, 18, 40);
    int32_t lvl = 0, k, i, hx, hy;
    scope_levels();
    for (k = 0; k < (int32_t)NTRK; k++)
        if (scd.lv[k] > lvl)
            lvl = scd.lv[k];
    if (song.playing && (!wl.was_playing || song.tick < wl.run))   /* PLAY (stopped is the top), or the run begun
                                                                         * again since the last drawing: a new spell */
        wl_begin();
    wl.run = song.tick;
    wl.was_playing = song.playing;
    if (!wl.seed)
        wl_begin();
    {   /* the figure whole as the song ends: so much of it by now (a loop: a cell for every two elements) */
        uint32_t kb, pos, tot = song_at(&kb, &pos);
        wl.allow = (uint8_t)(tot && lists.song ? 2u + (uint32_t)wl.nmask * (pos + 192u) / tot : wl.total / 2u + 2u);
        if (wl.allow > wl.nmask)
            wl.allow = wl.nmask;
    }
    /* the strikes: one element a track a beat */
    for (k = 0; k < (int32_t)NTRK; k++) {
        const track_t *t = &trk[k];
        uint32_t len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 16u, cur = t->seq_idx % len;
        if (song.playing && cur != wl.idx[k] && !t->p[P_MUTE] && step_on(&t->step[cur]) &&
            ENGINES[t->eng_req % NENGINES]->npresets && wl.last_q[k] != cur / 4u + 1u) {
            uint32_t note = t->step[cur].n ? t->step[cur].note[0] : 60u;
            wl.last_q[k] = (uint16_t)(cur / 4u + 1u);
            if (!anim) {
                wl_add((uint32_t)k, note);
            } else {
                for (i = 0; i < (int32_t)WL_NF && wl.fly[i].t; i++)
                    ;
                if (i < (int32_t)WL_NF) {
                    wl.fly[i].k = (uint8_t)k;
                    wl.fly[i].t = 1;
                    wl.fly[i].note = (uint8_t)note;
                }
            }
        }
        wl.idx[k] = (uint16_t)cur;
    }
    wl_head_at(now, anim, &hx, &hy);
    /* the chamber: night, its stars, the floor */
    cv_rrect(3, 0, 234, H, 5, sky, T_BG);
    for (i = 0; i < 40; i++) {
        uint32_t h = wl_h((uint32_t)i * 7919u + 17u), tw = wl_h((uint32_t)i + (now / 180u) * 131u) % 1000u;
        if ((int32_t)tw < 400 + lvl / 2)
            cv_pset(6 + (int32_t)(h % 228u), 2 + (int32_t)(h >> 12) % (H / 2), tw < 120u ? WL_RGB(230, 230, 250) : WL_RGB(110, 100, 150));
    }
    cv_rect(4, cyc - 12, 232, H - cyc + 11, floor);
    /* the circle: a pentagram in its ring on the floor, turning; it flares as an element is taken in */
    {
        uint16_t rc = wl.flash ? ux_mix(WL_COL[wl.flash_k], floor, 60 - (int32_t)wl.flash * 8) : WL_RGB(120, 90, 200);
        uint32_t a0 = anim ? now * 262144u : 0u, j;
        int32_t px[5], py[5];
        wl_oval_i(cx, cyc, 58, 9, 72u, rc);
        wl_oval_i(cx, cyc, 50, 7, 60u, ux_mix(rc, floor, 40));
        for (j = 0; j < 5u; j++) {
            uint32_t a = a0 + j * 858993459u;
            px[j] = cx + 50 * QK_COS(a) / 32767;
            py[j] = cyc + 7 * QK_SIN(a) / 32767;
        }
        for (j = 0; j < 5u; j++)
            xe_line(px[j], py[j], px[(j + 2u) % 5u], py[(j + 2u) % 5u], ux_mix(rc, floor, 30));
    }
    /* what it summons: the figure over the circle, its glow */
    if (wl.total) {
        int32_t fy = cyc - 50 + (anim ? QK_SIN(now * 196608u) * 2 / 32767 : 0), g = (int32_t)(wl.total - wl.shown);
        if (wl.shown >= wl.nmask)                       /* (whole: the elements after feed its glow) */
            xe_dots(cx, fy + 24, 30 + (g > 60 ? 12 : g / 5), 32u, anim ? now * 1048576u : 0u,
                    ux_mix(WL_COL[wl.flash_k], sky, 55));
        wl_figure(cx, fy, now, anim, WL_RGB(20, 12, 36));
    }
    /* the alchemist: the head, its hands' beam from its third eye down into the circle as an element goes in */
    {
        uint16_t cols[4];
        cols[0] = WL_RGB(170, 160, 200);
        cols[1] = WL_RGB(130, 120, 165);
        cols[2] = WL_RGB(50, 40, 80);
        cols[3] = wl.flash ? WL_COL[wl.flash_k] : WL_RGB(200, 120, 255);
        ui_sprite(hx, hy, SPR_VL_HEAD, 4u, cols, 2);
        if (wl.flash) {
            xe_line(hx + 22, hy + 14, cx - 6, cyc - 4, ux_mix(WL_COL[wl.flash_k], sky, 40));
            xe_line(hx + 22, hy + 14, cx + 6, cyc - 4, ux_mix(WL_COL[wl.flash_k], sky, 40));
            if (anim)
                wl.flash--;
        }
        if (lvl > 650)                                  /* (the incantation: its mouth lit when it is loud) */
            cv_rect(hx + 14, hy + 34, 12, 2, WL_RGB(200, 120, 255));
    }
    /* the shelf: the six jars, their elements, how many went in */
    for (k = 0; k < (int32_t)NTRK; k++) {
        const track_t *t = &trk[k];
        int32_t x = wl_jar_x((uint32_t)k), y = H - 13;
        int on = ENGINES[t->eng_req % NENGINES]->npresets && !t->p[P_MUTE];
        char b[6];
        wl_elem(x, y, (uint32_t)k, on ? WL_COL[k] : ux_mix(WL_COL[k], sky, 75));
        fmt_int(b, wl.cnt[k] > 999u ? 999 : (int32_t)wl.cnt[k]);
        cv_text_on(x + 11, y - 1, &AF_S, b, on ? WL_RGB(210, 200, 230) : WL_RGB(90, 80, 110), sky);
    }
    /* the elements in the air: an arc from the jar up and into the circle */
    for (i = 0; i < (int32_t)WL_NF; i++) {
        int32_t x0, y0, x1 = cx - 4, y1 = cyc - 10, mx, my, u, x, y;
        if (!wl.fly[i].t)
            continue;
        x0 = wl_jar_x(wl.fly[i].k);
        y0 = H - 14;
        mx = (x0 + x1) / 2;
        my = cyc - 70;
        u = (int32_t)wl.fly[i].t * 1000 / (int32_t)WL_FT;  /* (0 .. 1000: a quadratic curve) */
        x = (x0 * (1000 - u) / 1000 * (1000 - u) + 2 * mx * (1000 - u) / 1000 * u + x1 * u / 1000 * u) / 1000;
        y = (y0 * (1000 - u) / 1000 * (1000 - u) + 2 * my * (1000 - u) / 1000 * u + y1 * u / 1000 * u) / 1000;
        wl_elem(x, y, wl.fly[i].k, WL_COL[wl.fly[i].k]);
        if (anim && ++wl.fly[i].t > WL_FT) {
            wl_add(wl.fly[i].k, wl.fly[i].note);
            wl.fly[i].t = 0;
        }
    }
}
