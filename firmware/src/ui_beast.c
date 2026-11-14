/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) A creature for every sound: 12 x 12, put together from parts as an identikit is. The engine gives the
 * head (the family: ANALOG's greys, FM6's eyes on the pyramid, VOICE's hooded singers, DRUM's skulls ..) and a mark of
 * its own on the face; the sound gives what is on the head (antennae, horns, a halo, a wizard's hat, a crown, a flame,
 * ears, eyes on stalks), the eyes (black almonds, two glowing, one great eye, a third eye, many, a visor's band) and
 * what is below (a robe, tentacles, legs, wings, a ghost's tail, arms raised, a beard, an orb it floats on). A
 * factory sound's parts come from its place in its engine's list, so no two sounds share a creature (tests/ui_test.c);
 * a user preset's from its name. Left halves of 6, mirrored: ' ' nothing (or, over a part, left as it is), '#' the
 * body, 'o' an eye (lit), '.' dark, '+' a glint. Plain C: xe_beast_px fills the pixels, xe_beast_art packs them as
 * gen_sprites.py packs (3 bits, keys "#o.+"). Included by ui_xeno.c. */

enum { XB_DOME, XB_TALL, XB_HOOD, XB_PYRAMID, XB_SKULL, XB_INSECT, XB_MASK, XB_SQUID, XB_HUMAN, XB_NHEAD };
static const char XB_HEAD[XB_NHEAD][6][7] = {   /* rows 2 .. 7 */
    {"  ####", " #####", "######", "######", " #####", "  ####"},   /* a grey's dome */
    {" #####", "######", "######", " #####", "  ####", "   ###"},   /* a tall cranium, a narrow chin */
    {"  ####", " ##...", "##....", "##....", "##....", "###..."},   /* a hood, the face in its dark */
    {"    ##", "   ###", "  ####", " #####", "######", "######"},   /* the pyramid */
    {" #####", "######", "######", "######", " #####", "  #.#."},   /* a skull, its teeth */
    {"   ###", " #####", "######", "######", "  ####", " #  ##"},   /* an insect, its mandibles */
    {"  ####", " #####", "#.....", "#.....", " #####", "  ####"},   /* a helmet, its visor */
    {"   ###", "  ####", " #####", "######", "######", " #####"},   /* a mantle */
    {"  ####", " #####", " #####", "######", " #####", "  ####"},   /* a face: a seer, a monk */
};
enum { XB_NEYE = 6 };
static const char XB_EYES[XB_NEYE][3][7] = {   /* rows 3 .. 5, over the head */
    {"      ", " ...  ", "  .o  "},                 /* black almonds, a glint */
    {"      ", "  oo  ", "      "},                 /* two glowing */
    {"      ", "    ..", "    .o"},                 /* one great eye */
    {"     o", "  o   ", "      "},                 /* a third eye over two */
    {"   o o", "  o   ", " o  o "},                 /* many */
    {"      ", "  oooo", "      "},                 /* a band of light */
};
enum { XB_NCROWN = 9 };
static const char XB_CROWN[XB_NCROWN][3][7] = {   /* rows 0 .. 2, over the head */
    {"      ", "      ", "      "},                 /* bare */
    {"  +   ", "   #  ", "      "},                 /* antennae */
    {"#     ", "##    ", " ##   "},                 /* horns */
    {"  ++++", " +    ", "      "},                 /* a halo */
    {"     #", "    ##", "   ###"},                 /* a wizard's hat */
    {" +  + ", " #####", "      "},                 /* a crown */
    {"     +", "    ++", "     +"},                 /* a flame */
    {"#     ", "##    ", "      "},                 /* long ears */
    {"   o  ", "    # ", "      "},                 /* eyes on stalks */
};
enum { XB_NLOW = 8 };
static const char XB_LOW[XB_NLOW][4][7] = {   /* rows 8 .. 11 */
    {"    ##", "  ####", " #####", "######"},       /* a robe */
    {"  ####", " # # #", " # # #", "#  #  "},       /* tentacles */
    {"   ###", "  #  #", " #   #", "#    #"},       /* legs */
    {"#   ##", "##  ##", " ## ##", "   ###"},       /* wings */
    {"  ####", "  ####", "   ###", "  # # "},       /* a ghost's tail */
    {"#  ###", "## ###", "  ####", "  ####"},       /* arms raised */
    {"   +++", "   +++", "  ##++", " #####"},       /* a beard */
    {"    ##", "   +++", "  ++++", "   +++"},       /* an orb under it */
};
/* the engines' heads and their marks (ENGI_* order; one not listed: the grey's), each pair its own */
static const uint8_t XB_FAM[][2] = {
    {XB_DOME, 0},  {XB_DOME, 1},   {XB_INSECT, 0}, {XB_MASK, 0},    {XB_HUMAN, 0}, {XB_HOOD, 0}, {XB_TALL, 0},
    {XB_HOOD, 1},  {XB_SQUID, 0},  {XB_SQUID, 1},  {XB_SKULL, 0},   {XB_HOOD, 2},  {XB_PYRAMID, 0},
    {XB_PYRAMID, 1}, {XB_INSECT, 1},
};
#define XB_NFAM (sizeof XB_FAM / sizeof XB_FAM[0])

static void xb_put(uint8_t px[12][12], int32_t y, const char *half)
{
    int32_t i;
    for (i = 0; i < 6; i++) {
        char c = half[i];
        uint8_t v = c == '#' ? 1u : c == 'o' ? 2u : c == '.' ? 3u : c == '+' ? 4u : 0u;
        if (v) {
            px[y][i] = v;
            px[y][11 - i] = v;
        }
    }
}
/* the creature of engine e's sound n (n: its place in the list; a user preset's: 64 + its name's hash) */
static void xe_beast_px(uint32_t e, uint32_t n, uint8_t px[12][12])
{
    const uint8_t *f = XB_FAM[e < XB_NFAM ? e : 0u];
    uint32_t pair = (n * 29u + e * 11u) % (XB_NCROWN * XB_NLOW), eyes = (n * 5u + e) % XB_NEYE, j, i;
    for (j = 0; j < 12u; j++)
        for (i = 0; i < 12u; i++)
            px[j][i] = 0;
    for (j = 0; j < 6u; j++)
        xb_put(px, 2 + (int32_t)j, XB_HEAD[f[0]][j]);
    if (f[1])                                       /* the family's mark: row 7 striped (1) or row 6 glinting (2) */
        for (i = 0; i < 12u; i++)
            if (px[f[1] == 1u ? 7 : 6][i] == 1u && (f[1] == 2u || i % 2u == 0u))
                px[f[1] == 1u ? 7 : 6][i] = 4u;
    for (j = 0; j < 3u; j++)
        xb_put(px, 3 + (int32_t)j, XB_EYES[eyes][j]);
    for (j = 0; j < 3u; j++)
        xb_put(px, (int32_t)j, XB_CROWN[pair % XB_NCROWN][j]);
    for (j = 0; j < 4u; j++)
        xb_put(px, 8 + (int32_t)j, XB_LOW[pair / XB_NCROWN][j]);
}
/* packed as a sprite (ui_sprite): w, h, 3 bits a pixel, a spare byte */
#define XB_ART_N (3u + (12u * 12u * 3u + 7u) / 8u + 1u)
static void xe_beast_art(uint32_t e, uint32_t n, uint8_t *out)
{
    uint8_t px[12][12];
    uint32_t acc = 0, nb = 0, o = 3, j, i;
    xe_beast_px(e, n, px);
    out[0] = 12;
    out[1] = 12;
    out[2] = 3;
    for (j = 0; j < 12u; j++)
        for (i = 0; i < 12u; i++) {
            acc = acc << 3 | px[j][i];
            nb += 3u;
            if (nb >= 8u) {
                out[o++] = (uint8_t)(acc >> (nb - 8u));
                nb -= 8u;
            }
        }
    if (nb)
        out[o++] = (uint8_t)(acc << (8u - nb));
    out[o] = 0;
}
/* a name's number for a sound that is not a factory one */
static uint32_t xb_hash(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return 64u + h % 4093u;
}
