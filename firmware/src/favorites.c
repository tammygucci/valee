/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
#define FELUCCA_FAVORITES 1
/* Stable engine/preset references; fixed capacity independent of optional engines. */
typedef struct {
    uint8_t factory[16][32]; /* engine 0..15, preset 0..255 */
    uint32_t user, filter;
} favorites_t;
static favorites_t favorites;
/* ((Vallée, the SPAMPY sound) each engine's marks in a slice of 8 bytes (64 sounds) in the rows that only ever held marks
 * (0, 2..13: row 1 holds AUTO and MIDI LEARN, row 14 THEME CUSTOM's colours, row 15 Skiff's settings: settings_persist.c),
 * four slices a row from row 0 (rows 0 2 3 4 5 6), in the order of SPAMPY's numbers; the reserved numbers none */
#define FAV_NONE 0xFFu
static const uint8_t FAV_SLOT[NENGINES] = {0, 1, 2, 3, 4, 5, 6, 7, FAV_NONE, 8, FAV_NONE, 9, FAV_NONE, 10, 11, 12, 13,
                                           FAV_NONE, 14, 15, 16};
static const uint8_t FAV_ROW[5] = {0, 2, 3, 4, 5};
_Static_assert(NENGINES == 21, "FAV_SLOT: a place per engine");
static uint8_t *fav_byte(uint32_t engine, uint32_t preset)   /* 0: no mark for it */
{
    uint32_t n;
    if (engine >= NENGINES || (n = FAV_SLOT[engine]) == FAV_NONE || preset >= 64u)
        return 0;
    return &favorites.factory[FAV_ROW[n / 4u]][n % 4u * 8u + preset / 8u];
}
static int favorite_has(uint32_t engine, uint32_t preset)
{
    const uint8_t *b;
    if (engine == NENGINES)
        return preset < UP_SLOTS && ((favorites.user >> preset) & 1u);
    b = fav_byte(engine, preset);
    return b && ((*b >> (preset % 8u)) & 1u);
}
static int favorite_set(uint32_t engine, uint32_t preset, int on)
{
    uint8_t *b;
    if (engine > NENGINES || (engine == NENGINES && preset >= UP_SLOTS))
        return 0;
    if (engine < NENGINES && !(b = fav_byte(engine, preset)))
        return 0;
    if (favorite_has(engine, preset) == !!on) return 0;
    if (engine == NENGINES) {
        if (on) favorites.user |= 1u << preset;
        else favorites.user &= ~(1u << preset);
    } else {
        b = fav_byte(engine, preset);
        if (on) *b |= (uint8_t)(1u << (preset % 8u));
        else *b &= (uint8_t)~(1u << (preset % 8u));
    }
    return 1;
}
/* ((Vallée, the SPAMPY sound) the marks of before name sounds that are gone: rows 0 and 2..13 cleared once (the
 * settings in rows 1, 14 and 15 kept), FAV_ERA in row 7 (marks no more) says so. After the settings load */
#define FAV_ERA_BYTE (favorites.factory[7][0])
#define FAV_ERA 0xA5u
static void favorites_era(void)
{
    uint32_t r;
    if (FAV_ERA_BYTE == FAV_ERA)
        return;
    for (r = 0; r < 14u; r++)
        if (r != 1u)
            memset(favorites.factory[r], 0, sizeof favorites.factory[r]);
    FAV_ERA_BYTE = FAV_ERA;
}
