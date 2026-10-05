#ifndef VITA_AOE_DEFLATE_WRITER_H
#define VITA_AOE_DEFLATE_WRITER_H

#include <stddef.h>
#include <stdint.h>

/* The compressed stream writer of patched Rise of Rome 1.0a (the
   writing side of 0x0046ba40 / 0x0046b6b0). The original links its own
   deflate encoder (0x005223e0..0x00524fc0, state magic 0x12345678, done
   0xabcd1234, one global current state 0x007d32c8). Its output is
   standard raw deflate (no zlib header): a shipped 1.11 .scx body
   inflates completely with a -15 window. The port replaces the encoder
   with zlib's raw deflate, so compressed bytes differ from the original
   while the inflated data is the same.

   Original encoder, for reference (all reached only through 0x00524ef0
   and 0x005223e0):
   - 0x00524ef0 init (tuning clamped to 1..1500, block type flags, output
     buffer and size, the output callback; 4 KB input steps); 0x00524fc0
     clears the three hash heads/chains to 0xffff and the bit buffer.
   - 0x005223e0 write(input, size, finish): loads the state into globals,
     runs 0x00522570, stores them back; 2 for a bad state.
   - 0x00522570 the loop: 0x00524dc0 takes up to 4 KB into the 32 KB
     window (0x00524e90 zero-fills the rest when finishing); 0x00522930 /
     0x00522980 drop and rebuild hash chains for the step (3-byte hash
     <<5 ^); 0x005226d0 copies (the window slide); 0x00523ea0 /
     0x00524da0 match up to a limit through 0x00523ee0, which picks the
     lazy (0x005245b0) or greedy (0x00524980) matcher, both using the
     longest-match search 0x005243b0; finishing flushes the last block
     (0x00522a70), aligns (0x00522790) and hands the output on
     (0x00522730).
   - 0x00524560 a 32-bit word of literal/match flags done; at 0x3000
     symbols a block is emitted by 0x00522af0: header bits (0x005227c0,
     LSB-first bit writer), stored (0x00523d30), fixed (0x00523330 with
     the fixed tables of 0x005230a0: 144 x 8, 112 x 9, 24 x 7, 8 x 8 and
     32 x 5 bits) or dynamic (0x00523b60, headers 0x00523b90: HLIT - 257,
     HDIST - 1, the code-length codes) chosen by size, symbols by
     0x00522e50.
   - Huffman building: 0x00523350 counts, 0x005237a0 the code-length
     codes (run symbols 16 by 0x00523a80, 17/18 by 0x00523ae0;
     0x00523a60 copies); 0x00523540 builds a length-limited tree with
     the heap sift 0x00523660; 0x005236f0 limits lengths (bit-length
     counts at 0x007d32e0); 0x005231b0 counts lengths and 0x00523240
     assigns canonical codes (bit-reversed). */
typedef struct AoeDeflateWriterRor AoeDeflateWriterRor;

/* Output sink: returns 0 on success (as 0x00522730's callback). */
typedef int (*AoeDeflateSinkRor)(void *context, const uint8_t *data,
                                 size_t size);

AoeDeflateWriterRor *aoe_deflate_writer_new_ror(int level,
                                                AoeDeflateSinkRor sink,
                                                void *context);
/* write(..., finish): 0 success, 2 failure (as 0x005223e0). With finish
   the stream is closed; no more writes are accepted. */
int aoe_deflate_writer_write_ror(AoeDeflateWriterRor *w, const void *data,
                                 size_t size, int finish);
void aoe_deflate_writer_free_ror(AoeDeflateWriterRor *w);

#endif
