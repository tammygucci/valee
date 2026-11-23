/* ((Vallée) GEN's own chords, a part's (vgen.c: CHORDS, the fifth window; project.c: saved with the project, SKF4 after
 * the tracks). Here, not in vgen.c, so project.c builds without GEN (the host tests that hold it alone) */
#ifndef GEN_CHORDS_H
#define GEN_CHORDS_H
#define GC_NPART 6u                                     /* LOOP, INTRO, VERSE, CHORUS, BRIDGE, OUTRO (vgen.c GP_*) */
/* r: 1..7 a white key's, the mode's degree (C I, D II .. B VII); 8..19 a black key's, its root 8 + semitones above the
 * key; 0 none, the part ends before it. q: 0 the mode's own chord there (AUTO), else Q + 1. on: the part plays them
 * (else GEN's own; they are kept) */
typedef struct { uint8_t on, r[4], q[4]; } gch_part_t;
static gch_part_t gch[GC_NPART];
#endif
