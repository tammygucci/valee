/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) what SLOOP's engines (sloop/) expect of SPAMPY's core.h that Vallée names otherwise */
#define NPOLY NVOICE                     /* SPAMPY's voices a part; Vallée's: NVOICE */
#define P_FTYPE P_AFLT                   /* ANALOG's filter TYPE (LP BP HP): Vallée's FILTER slot (params.c) */
static void *eng_part(const track_t *t);
/* SPAMPY's per-part arena: Vallée's shared engine area (engines.c eng_part; an engine switch zeroes it) */
static inline void *eng_arena_of(const track_t *t, uint32_t eng) { (void)eng; return eng_part(t); }
