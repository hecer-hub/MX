#ifndef MXA_FUNCTIONS_H
#define MXA_FUNCTIONS_H

#define MXA_MAGIC "MXA\0"
#define MXA_MAGIC_LEN 4
#define FILENAME_LEN_BYTE 1
#define FILESIZE_LEN_BYTE 4
#define COMPRESSION_FLAG_BYTE 1

#define COMPRESSION_NONE 0x00
#define COMPRESSION_RLE 0x01
#define COMPRESSION_LZ_LITE 0x02

#define RLE_MARKER 0xFF
#define RLE_MAX_REPEAT 255

#define LZ_LITE_MARKER 0xFD
#define LZ_LITE_WINDOW_SIZE 255
#define LZ_LITE_MIN_MATCH 2
#define LZ_LITE_MAX_MATCH 16

int mxa_pack_cmd(int argc, char *argv[]);
int mxa_unpack_cmd(int argc, char *argv[]);

#endif