#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define EBCDIC_TXT "\xE3\xE7\xE3"
#define EBCDIC_END "\xC5\xD5\xC4"
#define EBCDIC_NUM(x) ((x) + 0xf0)
#define EBCDIC_SPACE ((0x0f * 3) + 1)
#define MAX_REC_SIZE (4096 * 4)

static const unsigned char zero[80] = {0};
int bin2rec(
    FILE *in,
    FILE *out)
{
    uint32_t addr = 0, b_addr, end_addr;
    uint16_t len;

    end_addr = MAX_REC_SIZE;
    len = __bswap_16(56);
    while(!feof(in)) {
        unsigned char tmp[56];

        /* Read from binary */
        fread(&tmp[0], 1, 56, in);

        /* New punchcard line */
        size_t w_len = 0, line_len = 0;

        w_len = 1; /* COLUMN 0,0 */
        fwrite("\x02", 1, w_len, out);
        line_len += w_len;

        w_len = 3; /* COLUMN 1,3 */
        if(addr >= end_addr || feof(in)) {
            fwrite(EBCDIC_END, 1, w_len, out);
            w_len = 80 - line_len; /* COLUMN 4,80 */
            fwrite(&zero, 1, w_len, out);
            line_len += w_len;
            break;
        } else {
            fwrite(EBCDIC_TXT, 1, w_len, out);
        }
        line_len += w_len;

        w_len = 1; /* COLUMN 4,4 */
        fwrite(&zero, 1, w_len, out);
        line_len += w_len;

        w_len = 3; /* COLUMN 5,7 */
        b_addr = __bswap_32(addr);
        fwrite((char *)&b_addr + 1, 1, w_len, out);
        line_len += w_len;

        w_len = 2; /* COLUMN 8,9 */
        fwrite(&zero, 1, w_len, out);
        line_len += w_len;

        w_len = sizeof(uint16_t); /* COLUMN 10,11 */
        fwrite(&len, 1, w_len, out);
        line_len += w_len;

        w_len = 16 - line_len; /* COLUMN 12,15 */
        fwrite(&zero, 1, w_len, out);
        line_len += w_len;

        w_len = 56; /* COLUMN 16,68 */
        fwrite(&tmp[0], 1, w_len, out);
        line_len += w_len;

        w_len = 80 - line_len; /* COLUMN 68,80 */
        fwrite(&zero, 1, w_len, out);
        line_len += w_len;

        addr += 56;
        if(line_len > 80) {
            printf("Line is bigger than 80 (%zu)\n", line_len);
            return -1;
        }
    }
    return 0;
}

int main(
    int argc,
    char **argv)
{
    FILE *inp, *out;
    if(argc <= 2) {
        perror("Usage: bin2txt [in] [out]\n");
        exit(EXIT_FAILURE);
    }
    inp = fopen(argv[1], "rb");
    if(!inp) {
        perror("Cannot open file\n");
        exit(EXIT_FAILURE);
    }
    out = fopen(argv[2], "wb");
    if(!out) {
        perror("Cannot create file\n");
        exit(EXIT_FAILURE);
    }

    bin2rec(inp, out);

    fclose(out);
    fclose(inp);
    return 0;
}
