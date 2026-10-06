#ifndef VITA_AOE_INFLATE_H
#define VITA_AOE_INFLATE_H

#include <stddef.h>
#include <stdint.h>

/* The stream decoder patched Rise of Rome 1.0a links for its compressed
   files (the DAT, scenarios, the player file): 0x00525b0b, reached only
   from the stream read 0x0046b800, with its work size 0x00525b05
   (0xad90 bytes, zeroed by the caller). The executable contains no zlib;
   the format is raw deflate (stored, fixed and dynamic blocks, no header
   or check value).

   The original is hand-written assembly (0x005252b0..0x00525bb7, tables
   0x00525060..0x00525290) that suspends by copying its own stack into
   the work area (0x0052544d) and resumes by copying it back
   (0x0052549d). This translation keeps the same states, bit reader,
   window, code tables and results and holds the resume points
   explicitly:

   - work area: a 0x8000-byte window at +0 (with slack for a match that
     runs past it), write position +0x8200, the end of the previous fill
     +0x8204, the limit +0x8208 (window +0x8000); the call's output
     pointer and space +0x820c / +0x8210, input +0x8220 / +0x8224; the
     final-block bit +0xad84; the state +0xa958 (0 start, 1 out of input,
     2 output full).
   - bits (0x0052554c / 0x00525574 / 0x00525564 / 0x005254fc): LSB first,
     edx holding the rest of the current byte (ch bits, 1..8) and the
     next whole byte above it; a byte is fetched each time the current
     one is used up. With no input left the original suspends (state 1)
     unless the call's last argument is set, when it reads 0xff; 0x0046b800
     always sets it, so state 1 is unreachable in the game and this
     translation always reads 0xff.
   - blocks (0x005259c0 through the table at 0x005252a0): final bit,
     2-bit type; 0 stored (0x00525953: to a byte boundary, LEN, NLEN which
     must be its complement, the bytes, then a fresh two-byte look-ahead),
     1 fixed (0x00525904: 144 x 8, 112 x 9, 24 x 7, 8 x 8; 32 x 5),
     2 dynamic (0x005257c9: HLIT + 257 at most 286, HDIST + 1 at most 30,
     HCLEN + 4 three-bit code-length lengths in the order at 0x00525060,
     then the lengths with 16 / 17 / 18 repeats; a repeat past the total
     fails; a 16 first repeats 0), 3 fails.
   - codes (0x005252b0): canonical; the code must be complete unless it
     has at most one length; codes of up to 8 bits fill a 256-entry table
     on the next 8 bits, longer ones hang a binary tree off it (entries are
     symbol x 4, tree nodes negative). An empty slot decodes as symbol 0.
   - symbols (0x0052571c): literals into the window, 256 ends the block,
     257..285 a length (bases 0x00525128, extra bits 0x005250ac, more than
     258 fails) then a distance (0x00525624; bases 0x0052521c, extra bits
     0x005251a4). A distance reaching before the window start continues
     from the end of the previous fill (+0x8204), so before the first
     flush it reads the zeroed window. The match copies byte by byte.
   - flush (0x0052566d): when the position reaches the limit, and at the
     end, the window goes to the output; when the output space runs out
     (even exactly) the call returns 0 in state 2 and the next call
     continues there.
   - results (0x0052541e, 0x00525ab4..0x00525bc4): 0 suspended, -1 the
     final block decoded and flushed, -2 failure (flushed first). The
     input and output counts are updated to what the call consumed and
     produced; on failure the input count is left as passed.

   Port choices: length symbols 286 / 287 and distance symbols 30 / 31
   fail (the original reads their extra bits and bases past its tables);
   after -1 or -2 further calls return the same without output (the
   original would replay its saved stack or restart). */

typedef struct AoeInflateRor AoeInflateRor;

/* A zeroed decoder in state 0 (the caller of 0x00525b0b allocates
   0x00525b05 bytes cleared). */
AoeInflateRor *aoe_inflate_new_ror(void);
void aoe_inflate_free_ror(AoeInflateRor *z);

/* 0x00525b0b: decode from `src` (`*src_len` bytes) into `dst`
   (`*dst_len` bytes of space). On return the counts are the bytes
   consumed and produced. Returns 0, -1 or -2 as above. */
int aoe_inflate_ror(AoeInflateRor *z, const uint8_t *src, uint32_t *src_len, uint8_t *dst,
                    uint32_t *dst_len);

/* Port helper for whole files (the loop of 0x0046b800): decodes all of
   `src` into a new buffer of at most `max_size` bytes. 1 on a complete
   stream; 0 on failure, a truncated stream (more than the two look-ahead
   bytes read past the end) or more output than `max_size`. */
int aoe_inflate_buffer_ror(const uint8_t *src, size_t size, size_t max_size, uint8_t **out,
                           size_t *out_size);

#endif
