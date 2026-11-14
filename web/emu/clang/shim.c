/* SPDX-License-Identifier: GPL-3.0-only */
/* (Skiff XENO) the four C library calls the browser emulator's live code makes, for a plain clang --target=wasm32
 * build (no Emscripten): web/emu/build_clang.sh */
#include <stddef.h>
void *memcpy(void *d, const void *s, size_t n)
{
    unsigned char *a = d; const unsigned char *b = s;
    while (n--) *a++ = *b++;
    return d;
}
void *memset(void *d, int c, size_t n)
{
    unsigned char *a = d;
    while (n--) *a++ = (unsigned char)c;
    return d;
}
void *memmove(void *d, const void *s, size_t n)
{
    unsigned char *a = d; const unsigned char *b = s;
    if (a < b)
        while (n--) *a++ = *b++;
    else
        while (n--) a[n] = b[n];
    return d;
}
int memcmp(const void *x, const void *y, size_t n)
{
    const unsigned char *a = x, *b = y;
    for (; n; n--, a++, b++)
        if (*a != *b) return *a < *b ? -1 : 1;
    return 0;
}
/* (what Emscripten's STANDALONE_WASM reactor exports and worklet.js calls first: no constructors here) */
__attribute__((export_name("_initialize"))) void wasm_initialize(void) {}
