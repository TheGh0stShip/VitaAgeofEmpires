/* Extract a 2048-byte ISO image from a raw MODE1/2352 CD data track.
   The Rise of Rome Rev 1 disc stores each track in a separate BIN file. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

enum { RAW_SECTOR = 2352, DATA_OFFSET = 16, DATA_SECTOR = 2048 };

int main(int argc, char **argv) {
    FILE *source, *destination;
    uint8_t sector[RAW_SECTOR];
    size_t count = 0;
    int result = 1;
    if (argc != 3) {
        fprintf(stderr, "usage: cdtrack_tool MODE1_2352.bin OUTPUT.iso\n");
        return 2;
    }
    source = fopen(argv[1], "rb");
    if (!source) { perror(argv[1]); return 1; }
    destination = fopen(argv[2], "wb");
    if (!destination) { perror(argv[2]); fclose(source); return 1; }
    for (;;) {
        size_t bytes = fread(sector, 1, sizeof(sector), source);
        if (!bytes) {
            if (ferror(source)) perror(argv[1]);
            else if (count > 16) result = 0;
            break;
        }
        if (bytes != sizeof(sector) || sector[0] != 0 || sector[11] != 0 ||
            sector[15] != 1) {
            fprintf(stderr, "invalid MODE1 sector at index %zu\n", count);
            break;
        }
        for (unsigned i = 1; i < 11; ++i)
            if (sector[i] != 0xff) {
                fprintf(stderr, "invalid sync at sector %zu\n", count);
                goto done;
            }
        if (count == 16 && memcmp(sector + DATA_OFFSET + 1, "CD001", 5)) {
            fprintf(stderr, "missing ISO9660 primary volume descriptor\n");
            break;
        }
        if (fwrite(sector + DATA_OFFSET, 1, DATA_SECTOR, destination) != DATA_SECTOR) {
            perror(argv[2]);
            break;
        }
        ++count;
    }
done:
    if (fclose(destination)) { perror(argv[2]); result = 1; }
    if (fclose(source)) { perror(argv[1]); result = 1; }
    if (!result) printf("extracted %zu MODE1 sectors\n", count);
    return result;
}
