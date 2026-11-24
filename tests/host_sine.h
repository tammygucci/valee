/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée, the SPAMPY sound) host tests only: a test engine at reserved number 8 (engines.c HOST_TEST_ENGINE), a plain
 * sine at the voice's pitch (computed exactly: libm) through the precise amplitude (dsp.c voice_amp2), as Skiff's
 * ANALOG played one. The way
 * out's checks (tail_test.c) need a source that keeps its shape as it fades; SLOOP's engines and FM6 step their
 * amplitude in Q15 (a faint shape of their own some -115 dBFS down, in SPAMPY too). Never in a firmware build. */
#include <math.h>
static void hs_note_on(struct track *t, voice_t *v)
{
    (void)t;
    v->ph[0] = 0;
}
static void hs_render(struct track *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    uint32_t i, ph = v->ph[0], inc = m->inc;
    (void)t;
    for (i = 0; i < n; i++) {
        out[i] += voice_amp2((int32_t)lrint(65000.0 * sin((double)ph * 1.4629180792671596e-9)), m, i);   /* (an exact
                                                     * sine, host only (libm): the table's is -55 dB; as loud as Skiff's
                                                     * ANALOG voice) */
        ph += inc;
    }
    v->ph[0] = ph;
}
static const engine_t ENG_HOSTTEST = {.name = "SINE", .page_title = {"-", "-"}, .note_on = hs_note_on,
                                      .render = hs_render};
