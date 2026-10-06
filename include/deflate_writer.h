#ifndef VITA_AOE_DEFLATE_WRITER_H
#define VITA_AOE_DEFLATE_WRITER_H

#include <stddef.h>
#include <stdint.h>

/* The stream encoder patched Rise of Rome 1.0a links for its compressed
   files (the writing side of 0x0046ba40 / 0x0046b6b0): 0x005223e0 with
   its state from 0x00524ef0 (work size 0x00525050, 0x3817c bytes zeroed
   by the caller; magic 0x12345678 while open, 0xabcd1234 once finished).
   The executable contains no zlib; the output is raw deflate, read back
   by its decoder (inflate.h).

   0x0046ba40 always initialises it as 0x00524ef0(work, 75, 2, 1, a
   64 KB output buffer, 0x00010000, callback 0x0046ba10): a hash-chain
   search of at most 75 links, each block stored, fixed or dynamic by
   size, and the chain matcher 0x005245b0. This translation is that
   configuration; the paths those arguments never reach (fixed-only and
   dynamic-only blocks, the other matchers 0x00524980 and the loop in
   0x00523ee0) are not translated.

   - Input (0x00524dc0): copied into a 32 KB window in 4 KB steps; a step
     is processed once full, or when finishing (the rest of the step is
     zeroed, 0x00524e90). The step's positions are hashed
     (0x00522980: (b0 << 5 ^ b1) & 0xff, << 5 ^ b2, 13 bits) into heads
     (+0x9102), newer-to-older links (+0xd102) and back links (+0x1d102)
     that let 0x00522930 drop the step about to be overwritten; the last
     two positions of each step are never hashed. The first 0x1102 bytes
     are mirrored after the window when step 0 is processed (0x005226d0)
     so matches run past its end.
   - Matching (0x00523ea0, 0x005245b0, 0x005243b0): from the position up
     to 0xefd bytes into the step (all of it when finishing, 0x00524da0);
     no chain or no match longer than 2 is a literal; the longest of up
     to 75 links (compared two bytes at a time, at most 258; a full 258
     stops the search; ties keep the nearer) is clamped to the bytes
     left, and is a literal when under 3, or when 3 at a distance over
     0x3fff.
   - Symbols: one byte for a literal, three (length - 3, distance) for a
     match, with a 32-bit word of flags (most significant first); at
     0x3000 symbols a block is written (0x00524560 -> 0x00522af0).
   - Blocks (0x00522af0): BFINAL 0, then: under 0x80 symbols, the fixed,
     dynamic and stored sizes are measured and the smallest written
     (stored only when strictly smaller than both, fixed only when
     strictly smaller than dynamic); else under 0x3999 bytes, dynamic or
     stored; else dynamic. Finishing (0x00522a70) writes the last block,
     then an empty fixed block with BFINAL 1, pads to a byte (0x00522790)
     and hands on the output (0x00522730).
   - Codes: lengths by repeatedly merging the two least frequent groups
     (0x00523540, heap 0x00523660 with ties to the higher symbol), limited
     to 15 bits (7 for the code-length code) by 0x005236f0, assigned in
     length-then-symbol order (0x005231b0) as canonical bit-reversed codes
     (0x00523240). The dynamic header (0x005237a0, 0x00523b90) uses 16 for
     repeats of the previous length (3..6), 17 / 18 for zero runs.
   - Bits (0x005227c0): LSB first; the output buffer goes to the sink
     whenever it is full and once at the end, even when empty (the
     game's callback 0x0046ba10 then records a write error, since
     _write returns 0). */
typedef struct AoeDeflateWriterRor AoeDeflateWriterRor;

/* Output sink: returns 0 on success (as 0x00522730's callback). */
typedef int (*AoeDeflateSinkRor)(void *context, const uint8_t *data,
                                 size_t size);

/* 0x00524ef0 with the game's arguments (75, 2, 1, 64 KB). */
AoeDeflateWriterRor *aoe_deflate_writer_new_ror(AoeDeflateSinkRor sink,
                                                void *context);
/* 0x005223e0(work, data, size, finish): 0 when the input is taken and
   more may follow; 1 once finished (and also when the sink failed); 2
   when the writer is not open (including after finishing). */
int aoe_deflate_writer_write_ror(AoeDeflateWriterRor *w, const void *data,
                                 size_t size, int finish);
void aoe_deflate_writer_free_ror(AoeDeflateWriterRor *w);

#endif
