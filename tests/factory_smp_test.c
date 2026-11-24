/* SPDX-License-Identifier: GPL-3.0-only */
/* (Vallée) The factory sample block in the sample area (eng_sample.c smp_fact_scan, editor.c SMP_* slot 127), built as
 * the FM-1 has it: no user slots, the block read from a flash image (host_samples: 0xA0000..0xDBFFF). Checks that
 * only a whole block of this build plays: erased, damaged, partly written (a power cut), another build's, or written
 * out of order, every factory set is missing and SAMPLE plays the sine, never the bytes; that the installer's
 * BEGIN / WRITE / END lands a block that plays; that BEGIN takes the sets away before it erases. */
#define EDITOR_TEST_FACTORY 1
#define EDITOR_TEST_NO_MAIN 1
#define SMP_FACT_IMG() ((const uint8_t *)&host_samples[0][0])
#include "editor_test.c"

static int sample_plays(void)                      /* a SAMPLE note on set 0: 1 its sample, 0 the sine */
{
    voice_t v;
    track_t *t = &trk[0];
    memset(&v, 0, sizeof v);
    t->p[P_E0] = 0;
    v.note = 60;
    sample_note_on(t, &v);
    return v.s[6] != 2;
}
static uint8_t *area(void) { return &host_samples[0][0]; }
static void put_block(void) { memcpy(area(), SMP_FACT_BLOB, sizeof SMP_FACT_BLOB); }

static uint32_t f_begin(void) { uint8_t a[1] = {0x7F}; request(ED_SMP_BEGIN, a, 1); return host_wire[6]; }
static uint32_t f_write(uint32_t off, const uint8_t *d, uint32_t len)
{
    uint8_t a[400];
    a[0] = 0x7F; a[1] = off & 127u; a[2] = off >> 7 & 127u; a[3] = off >> 14 & 127u;
    request(ED_SMP_WRITE, a, pack7(d, len, a + 4) + 4u);
    return host_wire[9];
}
static uint32_t f_end(const uint8_t *h)
{
    uint8_t a[40];
    a[0] = 0x7F;
    request(ED_SMP_END, a, pack7(h, 16u, a + 1) + 1u);
    return host_wire[6];
}
static uint32_t f_data(const uint8_t *blob, uint32_t n)   /* the data, as the installer sends it: rc of the first refusal */
{
    uint32_t off, rc;
    for (off = SMP_FACT_HDR; off < n; off += 256u)
        if ((rc = f_write(off, blob + off, n - off < 256u ? n - off : 256u)) != 0u)
            return rc;
    return 0;
}

int main(void)
{
    int bad = 0;
    uint8_t h[16], bl[sizeof SMP_FACT_BLOB];
    uint32_t n = sizeof SMP_FACT_BLOB;
    reset();
    bad += check("factory: the FM-1 build has no user slot (SMP_SHOWN 0, USR1 refused)", SMP_SHOWN == 0u && !SMP_USER_HAS(0));

    memset(area(), 0xFF, 0x3C000u); smp_fact_scan();
    bad += check("factory: an erased sample area: every set missing, SAMPLE plays the sine", !smp_fact_ok && !sample_plays() &&
                 smp_set_missing(0) && snd_missing(&trk[0], &n) == 0);
    n = sizeof SMP_FACT_BLOB;
    put_block(); smp_fact_scan();
    bad += check("factory: this build's block: in place, SAMPLE plays its sample", smp_fact_ok && sample_plays());
    area()[SMP_FACT_HDR + n / 2u] ^= 1u; smp_fact_scan();
    bad += check("factory: one bit off in the data: missing (the sine)", !smp_fact_ok && !sample_plays());
    put_block(); area()[9] ^= 1u; smp_fact_scan();
    bad += check("factory: another length in the header: missing", !smp_fact_ok);
    put_block(); area()[12] ^= 1u; smp_fact_scan();
    bad += check("factory: another CRC in the header (another build's block): missing", !smp_fact_ok);
    put_block(); area()[0] = 'X'; smp_fact_scan();
    bad += check("factory: no magic: missing", !smp_fact_ok);

    /* the installer's way */
    memset(area(), 0xFF, 0x3C000u); smp_fact_scan(); reset(); host_erases = host_writes = 0;
    bad += check("factory: WRITE before BEGIN refused (4), nothing written", f_write(SMP_FACT_HDR, SMP_FACT_BLOB, 256) == 4u &&
                 !host_writes);
    put_block(); smp_fact_scan(); reset();
    bad += check("factory: BEGIN on a good block: the sets go missing first, the header's sector erased", f_begin() == 0u &&
                 !smp_fact_ok && !sample_plays() && area()[0] == 0xFFu && area()[SMP_FACT_HDR] == 0xFFu);
    bad += check("factory: a WRITE inside the header, unaligned, or past the area: refused (1)",
                 f_write(0, SMP_FACT_BLOB, 256) == 1u && f_write(SMP_FACT_HDR + 1u, SMP_FACT_BLOB, 16) == 1u &&
                 f_write(0x3C000u - 128u, SMP_FACT_BLOB, 256) == 1u);
    /* a power cut halfway: no END */
    bad += check("factory: half the data, then the power goes: no block (missing) after the restart",
                 f_data(SMP_FACT_BLOB, n / 2u) == 0u && (smp_fact_scan(), !smp_fact_ok) && !sample_plays());
    /* the whole data, a wrong header */
    reset();
    bad += check("factory: the whole data after a new BEGIN", f_begin() == 0u && f_data(SMP_FACT_BLOB, n) == 0u);
    memcpy(h, SMP_FACT_BLOB, 16); h[12] ^= 1u;
    bad += check("factory: END with another build's header: refused (2), no header written, missing",
                 f_end(h) == 2u && area()[0] == 0xFFu && !smp_fact_ok);
    memcpy(h, SMP_FACT_BLOB, 16);
    bad += check("factory: END with this build's header: written last, the block in place, SAMPLE plays",
                 f_end(h) == 0u && !memcmp(area(), SMP_FACT_BLOB, n) && smp_fact_ok && sample_plays());
    bad += check("factory: END again without BEGIN: refused (4)", f_end(h) == 4u && smp_fact_ok);
    /* damaged data on the way: END finds it */
    memcpy(bl, SMP_FACT_BLOB, n); bl[n - 3u] ^= 0x10u; reset();
    bad += check("factory: data damaged on the way: END refuses it (3), no header, missing",
                 f_begin() == 0u && f_data(bl, n) == 0u && f_end(h) == 3u && area()[0] == 0xFFu && !smp_fact_ok);
    bad += check("factory: after that refusal, WRITE without a new BEGIN refused (4)", f_write(SMP_FACT_HDR, SMP_FACT_BLOB, 256) == 4u);
    bad += check("factory: BEGIN, the data again, END: in place", f_begin() == 0u && f_data(SMP_FACT_BLOB, n) == 0u && f_end(h) == 0u &&
                 smp_fact_ok && sample_plays());
    /* INFO tells the installer */
    request(ED_INFO, h, 0);
    bad += check("factory: INFO 56 01 says in place", host_wire[host_wire_n - 7] == 1u);
    memset(area(), 0xFF, 0x3C000u); smp_fact_scan(); request(ED_INFO, h, 0);
    bad += check("factory: INFO 56 01 says missing on an erased area", host_wire[host_wire_n - 7] == 0u);
    /* a restore of an old backup's user samples never lands on the block */
    put_block(); smp_fact_scan(); reset();
    {
        uint8_t a[1] = {0};
        uint32_t k;
        request(ED_SMP_BEGIN, a, 1);
        bad += check("factory: SMP_BEGIN of USR1 (an old backup): refused, the block kept", host_wire[6] != 0u &&
                     !memcmp(area(), SMP_FACT_BLOB, n) && (smp_fact_scan(), smp_fact_ok));
        for (k = 0; k < 3u; k++) {
            a[0] = (uint8_t)k;
            request(ED_SMP_ERASE, a, 1);
            bad += check("factory: SMP_ERASE of a user slot: refused, the block kept", host_wire[6] != 0u &&
                         !memcmp(area(), SMP_FACT_BLOB, n));
        }
    }
    printf("%s\n", bad ? "FACTORY SAMPLE TEST FAILED" : "factory sample test passed");
    return bad != 0;
}
