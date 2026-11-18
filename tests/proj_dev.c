/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* (Vallée) A unit for web/test_project.mjs: the firmware's backup objects (editor_backup.c LIST / GET / PUT) over
 * simulated NOR flash, its requests one a line on stdin ("cmd a0 a1 .."), each reply a line on stdout; set up by
 * "setup" (patterns and projects of its own), "load k" (slot k into the tracks: each track's first notes). */
static unsigned char host_samples[3][0x14000];
#define SMP_USER_XIP(k) host_samples[k]
#define main hostsim_main
#include "hostsim.c"
#undef main

static int32_t fm1_enc_take(uint32_t e) { (void)e; return 0; }
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static void lcd_sync(void) {}
static void lcd_power(uint32_t s) { (void)s; }   /* (MENU > SCREEN OFF: lcd.c) */
static void lcd_wake_now(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#define FELUCCA_FLASH 1
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
static void panel_setup(void) {}

static uint8_t nor[0x100000], flash_ok = 1;
static int erase_error;
static uint32_t erases;
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { if (erase_error) return -8; memset(nor + off, 0xFF, 4096); erases++; return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n) { memcpy(nor + off, src, n); return 0; }
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0; }
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"
#include "../firmware/src/bank.c"                 /* (Skiff: the pattern bank, objects 16..31) */

enum { ED_BACKUP_LIST = 65, ED_BACKUP_GET, ED_BACKUP_PUT };
static uint8_t rep[4096];
static uint32_t rep_n;
static void ed_b(uint32_t v) { if (rep_n < sizeof rep) rep[rep_n++] = (uint8_t)(v & 127u); }
static uint32_t ed_unpack7(const uint8_t *a, uint32_t na, uint8_t *out, uint32_t max)
{
    uint32_t n = 0;
    while (na && n < max) {
        uint32_t m = *a++, j;
        na--;
        uint32_t k = na > 7u ? 7u : na;
        if (!k || n + k > max || (m >> k))
            return 0;
        for (j = 0; j < k; j++, na--)
            out[n++] = (uint8_t)(*a++ | ((m >> j) & 1u) << 7);
    }
    return na ? 0u : n;
}
static uint8_t ed_smp_buf[512] __attribute__((aligned(4)));
static void fm1_wdt_feed(void) {}
static int ed_flash_stop(void) { return transport_busy(); }
#include "../firmware/src/editor_backup.c"

static uint32_t call(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    rep_n = 0;
    ed_backup_handle(cmd, a, n);
    return rep_n;
}
static void reset(void)
{
    memset(nor, 0xFF, sizeof nor);
    memset(&song, 0, sizeof song);
    sel_set(0);
    memset(trk, 0, sizeof trk);
    memset(proj_slot, 0, sizeof proj_slot);
    memset(up_bank, 0, sizeof up_bank);
    memset(&persist_saved, 0, sizeof persist_saved);
    memset(&settings, 0, sizeof settings);
    memset(&ui, 0, sizeof ui);
    panel = PANEL_DEFAULT;
    settings_init();
    host_tracks_init();
    fm1_ms = 0;
    ed_bk_valid = ed_bk_put = 0;
    bank_boot();
    lists_clear();
}
/* pattern id: track k's steps notes base, base+1 .. (len of them), packed and put in the pool */
static int pat(uint32_t id, uint32_t k, uint32_t base, uint32_t len)
{
    uint8_t r[BK_REC];
    track_t *t = &trk[k];
    uint32_t i;
    track_defaults_steps(t);
    for (i = 0; i < len; i++)
        t->step[i] = (step_t){{(uint8_t)(base + i)}, 1, ST_NOTE, 0, 100};
    t->p[P_SLEN] = (int16_t)len;
    return bk_pack(r, t) && pool_put(id, r) == 0;
}
/* a project in slot: track k's first entry pattern ids[k] (PAT_NONE: none), its second ids2[k] */
static int proj(uint32_t slot, const char *name, const uint8_t *ids, const uint8_t *ids2)
{
    uint32_t k;
    lists_clear();
    for (k = 0; k < NTRK; k++) {                   /* (the tracks emptied, on no pattern: the save keeps none of them) */
        track_t *t = &trk[k];
        motion_clear(t);
        track_defaults_steps(t);
        t->p[P_SLEN] = 16; t->p[P_SDIV] = 2; t->p[P_SSWING] = 0; t->p[P_SGATE] = 64; t->p[P_DIR] = DIR_FWD;
        trk_pid[k] = 0;
        if (ids[k] != PAT_NONE) { lists.ent[k][0][0] = (uint8_t)(ids[k] + 1u); lists.ent[k][0][1] = 1; }
        if (ids2[k] != PAT_NONE) { lists.ent[k][1][0] = (uint8_t)(ids2[k] + 1u); lists.ent[k][1][1] = 0; }
    }
    return project_save_as(slot, name) == 0;
}
int main(int argc, char **argv)
{
    char line[8192];
    (void)argc; (void)argv;
    reset();
    while (fgets(line, sizeof line, stdin)) {
        uint8_t a[4096];
        uint32_t n = 0, cmd, i;
        char *s = line, *e;
        if (!strncmp(line, "setup", 5)) {          /* "setup a": slot A names 0 1 2 5 (2 twice); "setup b": a pool
                                                    * nearly full, slot B and the music on some of it */
            static const uint8_t N[NTRK] = {PAT_NONE, PAT_NONE, PAT_NONE, PAT_NONE, PAT_NONE, PAT_NONE};
            int ok = 1;
            if (line[6] == 'a') {
                static const uint8_t A1[NTRK] = {0, 1, 2, PAT_NONE, 5, PAT_NONE}, A2[NTRK] = {2, PAT_NONE, PAT_NONE, PAT_NONE, PAT_NONE, PAT_NONE};
                ok &= pat(0, 0, 40, 4) && pat(1, 1, 50, 8) && pat(2, 2, 60, 3) && pat(5, 4, 70, 16) && pat(9, 0, 30, 2);
                ok &= proj(0, "SONG A", A1, A2);
            } else {
                static const uint8_t B1[NTRK] = {3, 4, PAT_NONE, PAT_NONE, PAT_NONE, PAT_NONE};
                uint32_t free = (uint32_t)atoi(line + 8), id;
                for (id = 0; id < NPAT - free; id++)
                    ok &= pat(id, id % NTRK, 20 + id % 40, 1 + id % 7);
                ok &= pat(0, 1, 50, 8);             /* (pattern 0: the same as unit A's 1, shared on a load) */
                ok &= proj(1, "SONG B", B1, N);
            }
            lists_clear();
            printf("%d\n", ok);
        } else if (!strncmp(line, "load", 4)) {    /* slot k into the tracks: each track's first entry's notes */
            uint32_t k = (uint32_t)atoi(line + 5), t;
            project_load(k);
            printf("%s", ui.msg);
            for (t = 0; t < NTRK; t++) {
                uint8_t id = lists.ent[t][0][0];
                printf(" %u:", id);
                if (id && id <= NPAT) {
                    static step_t st[NSTEP]; int16_t v[8]; static motion_store_t ms;
                    if (!pat_decode(&trk[t], id - 1u, st, v, &ms))
                        printf("%u", st[0].note[0]);
                }
            }
            printf("\n");
        } else {
            cmd = (uint32_t)strtoul(s, &e, 10);
            for (s = e; n < sizeof a; s = e) {
                unsigned long v = strtoul(s, &e, 10);
                if (e == s) break;
                a[n++] = (uint8_t)v;
            }
            call(cmd, a, n);
            for (i = 0; i < rep_n; i++) printf(i ? " %u" : "%u", rep[i]);
            printf("\n");
        }
        fflush(stdout);
    }
    return 0;
}
