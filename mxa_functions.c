#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include "mxa_functions.h"

long rle_compress(const unsigned char *in_buffer, size_t in_len, unsigned char *out_buffer, size_t out_len) {
    size_t in_pos = 0;
    size_t out_pos = 0;
    while (in_pos < in_len) {
        unsigned char current_byte = in_buffer[in_pos];
        int repeat_count = 0;
        size_t lookahead_pos = in_pos;
        while (lookahead_pos < in_len && in_buffer[lookahead_pos] == current_byte && repeat_count < RLE_MAX_REPEAT) {
            repeat_count++;
            lookahead_pos++;
        }
        if (current_byte == RLE_MARKER) {
            if (out_pos + 2 > out_len) return -1;
            out_buffer[out_pos++] = RLE_MARKER;
            out_buffer[out_pos++] = 0x00;
            in_pos += repeat_count;
            continue;
        }
        if (repeat_count >= 3) {
            if (out_pos + 3 > out_len) return -1;
            out_buffer[out_pos++] = RLE_MARKER;
            out_buffer[out_pos++] = current_byte;
            out_buffer[out_pos++] = (unsigned char)repeat_count;
        } else {
            for (int i = 0; i < repeat_count; ++i) {
                if (out_pos + 1 > out_len) return -1;
                out_buffer[out_pos++] = in_buffer[in_pos + i];
            }
        }
        in_pos += repeat_count;
    }
    return (long)out_pos;
}

long rle_decompress(const unsigned char *in_buffer, size_t in_len, unsigned char *out_buffer, size_t out_len) {
    size_t in_pos = 0;
    size_t out_pos = 0;
    while (in_pos < in_len) {
        unsigned char current_byte = in_buffer[in_pos++];
        if (current_byte == RLE_MARKER) {
            if (in_pos >= in_len) return -1;
            unsigned char next_byte = in_buffer[in_pos++];
            if (next_byte == 0x00) {
                if (out_pos + 1 > out_len) return -1;
                out_buffer[out_pos++] = RLE_MARKER;
            } else {
                if (in_pos >= in_len) return -1;
                unsigned char repeat_count = in_buffer[in_pos++];
                if (repeat_count == 0) return -1;
                if (out_pos + repeat_count > out_len) return -1;
                for (int i = 0; i < repeat_count; ++i) {
                    out_buffer[out_pos++] = next_byte;
                }
            }
        } else {
            if (out_pos + 1 > out_len) return -1;
            out_buffer[out_pos++] = current_byte;
        }
    }
    return (long)out_pos;
}

long lz_lite_compress(const unsigned char *in_buffer, size_t in_len, unsigned char *out_buffer, size_t out_len) {
    size_t in_pos = 0;
    size_t out_pos = 0;
    while (in_pos < in_len) {
        int best_match_len = 0;
        int best_match_offset = 0;
        size_t search_start = (in_pos > LZ_LITE_WINDOW_SIZE) ? (in_pos - LZ_LITE_WINDOW_SIZE) : 0;
        for (size_t i = search_start; i < in_pos; ++i) {
            int current_match_len = 0;
            if (in_buffer[i] == in_buffer[in_pos]) {
                while (current_match_len < LZ_LITE_MAX_MATCH &&
                       (in_pos + current_match_len < in_len) &&
                       (i + current_match_len < in_pos) &&
                       (in_buffer[in_pos + current_match_len] == in_buffer[i + current_match_len])) {
                    current_match_len++;
                }
            }
            if (current_match_len > best_match_len) {
                best_match_len = current_match_len;
                best_match_offset = in_pos - i;
            }
        }
        if (best_match_len >= LZ_LITE_MIN_MATCH) {
            if (out_pos + 3 > out_len) return -1;
            out_buffer[out_pos++] = LZ_LITE_MARKER;
            out_buffer[out_pos++] = (unsigned char)best_match_offset;
            out_buffer[out_pos++] = (unsigned char)best_match_len;
            in_pos += best_match_len;
        } else {
            unsigned char current_byte = in_buffer[in_pos];
            if (out_pos + 1 > out_len) return -1;
            if (current_byte == LZ_LITE_MARKER) {
                if (out_pos + 2 > out_len) return -1;
                out_buffer[out_pos++] = LZ_LITE_MARKER;
                out_buffer[out_pos++] = 0x00;
            } else {
                out_buffer[out_pos++] = current_byte;
            }
            in_pos++;
        }
    }
    return (long)out_pos;
}

long lz_lite_decompress(const unsigned char *in_buffer, size_t in_len, unsigned char *out_buffer, size_t out_len) {
    size_t in_pos = 0;
    size_t out_pos = 0;
    while (in_pos < in_len) {
        unsigned char current_byte = in_buffer[in_pos++];
        if (current_byte == LZ_LITE_MARKER) {
            if (in_pos >= in_len) return -1;
            unsigned char next_byte = in_buffer[in_pos++];
            if (next_byte == 0x00) {
                if (out_pos + 1 > out_len) return -1;
                out_buffer[out_pos++] = LZ_LITE_MARKER;
            } else {
                if (in_pos >= in_len) return -1;
                unsigned char offset = next_byte;
                unsigned char length = in_buffer[in_pos++];
                if (length == 0) return -1;
                if (out_pos < offset) {
                    fprintf(stderr, "lz_lite_decompress: Invalid offset.\n");
                    return -1;
                }
                if (out_pos + length > out_len) return -1;
                size_t copy_src_pos = out_pos - offset;
                for (int i = 0; i < length; ++i) {
                    out_buffer[out_pos++] = out_buffer[copy_src_pos++];
                }
            }
        } else {
            if (out_pos + 1 > out_len) return -1;
            out_buffer[out_pos++] = current_byte;
        }
    }
    return (long)out_pos;
}

int mxa_pack_cmd(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-n|-m|-d] <archive_name.mxa> <file1> [file2...]\n", argv[0]);
        return 1;
    }
    int arg_offset = 1;
    unsigned char compression_mode = COMPRESSION_NONE;
    if (argv[1][0] == '-') {
        if (strcmp(argv[1], "-n") == 0) {
            compression_mode = COMPRESSION_NONE;
            arg_offset++;
        } else if (strcmp(argv[1], "-m") == 0) {
            compression_mode = COMPRESSION_RLE;
            arg_offset++;
        } else if (strcmp(argv[1], "-d") == 0) {
            compression_mode = COMPRESSION_LZ_LITE;
            arg_offset++;
        } else {
            fprintf(stderr, "%s: Unknown option '%s'\n", argv[0], argv[1]);
            return 1;
        }
    }

    if (argc < arg_offset + 2) {
        fprintf(stderr, "Usage: %s [-n|-m|-d] <archive_name.mxa> <file1> [file2...]\n", argv[0]);
        return 1;
    }
    char *archive_name = argv[arg_offset];
    FILE *archive_fp = fopen(archive_name, "wb");
    if (archive_fp == NULL) {
        perror("mxa_pack: failed to open archive file");
        return 1;
    }
    fwrite(MXA_MAGIC, 1, MXA_MAGIC_LEN, archive_fp);
    for (int i = arg_offset + 1; i < argc; ++i) {
        char *file_path = argv[i];
        char *file_name = basename(file_path);
        struct stat st;
        if (stat(file_path, &st) != 0) {
            perror(file_path);
            fclose(archive_fp);
            return 1;
        }
        if (!S_ISREG(st.st_mode)) {
            fprintf(stderr, "%s: not a regular file, skipping.\n", file_path);
            continue;
        }
        size_t original_file_size = st.st_size;
        size_t temp_name_len = strlen(file_name);
        if (temp_name_len == 0 || temp_name_len > 255) {
            fprintf(stderr, "%s: filename too long or empty, skipping.\n", file_name);
            continue;
        }
        unsigned char name_len = (unsigned char)temp_name_len;

        FILE *input_fp = fopen(file_path, "rb");
        if (input_fp == NULL) {
            perror(file_path);
            fclose(archive_fp);
            return 1;
        }
        unsigned char *file_data = (unsigned char *)malloc(original_file_size);
        if (file_data == NULL) {
            fprintf(stderr, "mxa_pack: Out of memory for file data.\n");
            fclose(input_fp);
            fclose(archive_fp);
            return 1;
        }
        if (fread(file_data, 1, original_file_size, input_fp) != original_file_size) {
            fprintf(stderr, "mxa_pack: Error reading file %s.\n", file_path);
            free(file_data);
            fclose(input_fp);
            fclose(archive_fp);
            return 1;
        }
        fclose(input_fp);
        unsigned char *output_data = NULL;
        long compressed_size_long = original_file_size;
        size_t max_compressed_size_estimate = original_file_size * 2 + 1024;
        
        if (compression_mode == COMPRESSION_RLE) {
            output_data = (unsigned char *)malloc(max_compressed_size_estimate);
            if (output_data == NULL) {
                fprintf(stderr, "mxa_pack: Out of memory for RLE output.\n");
                free(file_data); fclose(archive_fp); return 1;
            }
            compressed_size_long = rle_compress(file_data, original_file_size, output_data, max_compressed_size_estimate);
            if (compressed_size_long == -1) {
                fprintf(stderr, "mxa_pack: RLE compression error for %s.\n", file_name);
                free(file_data); free(output_data); fclose(archive_fp); return 1;
            }
        } else if (compression_mode == COMPRESSION_LZ_LITE) {
            output_data = (unsigned char *)malloc(max_compressed_size_estimate);
            if (output_data == NULL) {
                fprintf(stderr, "mxa_pack: Out of memory for LZ_LITE output.\n");
                free(file_data); fclose(archive_fp); return 1;
            }
            compressed_size_long = lz_lite_compress(file_data, original_file_size, output_data, max_compressed_size_estimate);
            if (compressed_size_long == -1) {
                fprintf(stderr, "mxa_pack: LZ_LITE compression error for %s.\n", file_name);
                free(file_data); free(output_data); fclose(archive_fp); return 1;
            }
        } else {
            output_data = file_data;
        }

        size_t compressed_size = (size_t)compressed_size_long;

        if (output_data == NULL || compressed_size_long == -1) {
            fprintf(stderr, "mxa_pack: Compression/memory error for %s.\n", file_name);
            if (file_data) free(file_data);
            if (output_data != file_data && output_data) free(output_data);
            fclose(archive_fp);
            return 1;
        }

        fwrite(&name_len, 1, FILENAME_LEN_BYTE, archive_fp);
        fwrite(file_name, 1, name_len, archive_fp);
        fwrite(&compressed_size, 1, FILESIZE_LEN_BYTE, archive_fp);
        fwrite(&compression_mode, 1, COMPRESSION_FLAG_BYTE, archive_fp);
        fwrite(output_data, 1, compressed_size, archive_fp);
        
        const char* mode_str;
        switch (compression_mode) {
            case COMPRESSION_NONE: mode_str = "NONE"; break;
            case COMPRESSION_RLE: mode_str = "RLE"; break;
            case COMPRESSION_LZ_LITE: mode_str = "LZ_LITE"; break;
            default: mode_str = "UNKNOWN"; break;
        }
        printf("Packed: %s (original: %zu bytes, compressed: %zu bytes, mode: %s)\n",
               file_name, original_file_size, compressed_size, mode_str);
        if (compression_mode != COMPRESSION_NONE) {
            free(output_data);
        }
        free(file_data);
    }
    unsigned char end_marker = 0x00;
    fwrite(&end_marker, 1, FILENAME_LEN_BYTE, archive_fp);
    fclose(archive_fp);
    printf("Archive '%s' created successfully.\n", archive_name);
    return 0;
}

int mxa_unpack_cmd(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <archive_name.mxa>\n", argv[0]);
        return 1;
    }
    char *archive_name = argv[1];
    FILE *archive_fp = fopen(archive_name, "rb");
    if (archive_fp == NULL) {
        perror("mxa_unpack: failed to open archive file");
        return 1;
    }
    char magic_buffer[MXA_MAGIC_LEN];
    if (fread(magic_buffer, 1, MXA_MAGIC_LEN, archive_fp) != MXA_MAGIC_LEN ||
        memcmp(magic_buffer, MXA_MAGIC, MXA_MAGIC_LEN) != 0) {
        fprintf(stderr, "mxa_unpack: Not a valid .mxa archive: %s\n", archive_name);
        fclose(archive_fp);
        return 1;
    }
    printf("Extracting from archive: %s\n", archive_name);
    unsigned char name_len;
    size_t compressed_size;
    unsigned char compression_mode;
    char file_name[256];
    while (fread(&name_len, 1, FILENAME_LEN_BYTE, archive_fp) == FILENAME_LEN_BYTE && name_len != 0) {
        if (fread(file_name, 1, name_len, archive_fp) != name_len) {
            fprintf(stderr, "mxa_unpack: Error reading filename.\n");
            fclose(archive_fp);
            return 1;
        }
        file_name[name_len] = '\0';
        if (fread(&compressed_size, 1, FILESIZE_LEN_BYTE, archive_fp) != FILESIZE_LEN_BYTE) {
            fprintf(stderr, "mxa_unpack: Error reading compressed size for %s.\n", file_name);
            fclose(archive_fp);
            return 1;
        }
        if (fread(&compression_mode, 1, COMPRESSION_FLAG_BYTE, archive_fp) != COMPRESSION_FLAG_BYTE) {
            fprintf(stderr, "mxa_unpack: Error reading compression flag for %s.\n", file_name);
            fclose(archive_fp);
            return 1;
        }
        unsigned char *input_data = (unsigned char *)malloc(compressed_size);
        if (input_data == NULL) {
            fprintf(stderr, "mxa_unpack: Out of memory for input data.\n");
            fclose(archive_fp);
            return 1;
        }
        if (fread(input_data, 1, compressed_size, archive_fp) != compressed_size) {
            fprintf(stderr, "mxa_unpack: Error reading file data for %s.\n", file_name);
            free(input_data);
            fclose(archive_fp);
            return 1;
        }
        FILE *output_fp = fopen(file_name, "wb");
        if (output_fp == NULL) {
            perror(file_name);
            free(input_data);
            continue;
        }
        unsigned char *output_data = NULL;
        long decompressed_size_long = 0;
        size_t max_decompressed_size_estimate = compressed_size * 4 + 1024;
        
        switch (compression_mode) {
            case COMPRESSION_RLE:
                output_data = (unsigned char *)malloc(max_decompressed_size_estimate);
                if (output_data == NULL) {
                    fprintf(stderr, "mxa_unpack: Out of memory for RLE decomp.\n");
                    free(input_data); fclose(output_fp); fclose(archive_fp); return 1;
                }
                decompressed_size_long = rle_decompress(input_data, compressed_size, output_data, max_decompressed_size_estimate);
                if (decompressed_size_long == -1) {
                    fprintf(stderr, "mxa_unpack: RLE decompression error for %s.\n", file_name);
                    free(input_data); free(output_data); fclose(output_fp); fclose(archive_fp); return 1;
                }
                break;
            case COMPRESSION_LZ_LITE:
                output_data = (unsigned char *)malloc(max_decompressed_size_estimate);
                if (output_data == NULL) {
                    fprintf(stderr, "mxa_unpack: Out of memory for LZ_LITE decomp.\n");
                    free(input_data); fclose(output_fp); fclose(archive_fp); return 1;
                }
                decompressed_size_long = lz_lite_decompress(input_data, compressed_size, output_data, max_decompressed_size_estimate);
                if (decompressed_size_long == -1) {
                    fprintf(stderr, "mxa_unpack: LZ_LITE decompression error for %s.\n", file_name);
                    free(input_data); free(output_data); fclose(output_fp); fclose(archive_fp); return 1;
                }
                break;
            case COMPRESSION_NONE:
                output_data = input_data;
                decompressed_size_long = compressed_size;
                break;
            default:
                fprintf(stderr, "mxa_unpack: Unknown compression mode 0x%02x for %s. Skipping.\n", compression_mode, file_name);
                free(input_data);
                fclose(output_fp);
                continue;
        }

        size_t decompressed_size = (size_t)decompressed_size_long;

        if (output_data == NULL || decompressed_size_long == -1) {
            fprintf(stderr, "mxa_unpack: Decompression/memory error for %s.\n", file_name);
            if (input_data) free(input_data);
            if (output_data != input_data && output_data) free(output_data);
            fclose(output_fp);
            fclose(archive_fp);
            return 1;
        }

        fwrite(output_data, 1, decompressed_size, output_fp);
        fclose(output_fp);
        const char* mode_str;
        switch (compression_mode) {
            case COMPRESSION_NONE: mode_str = "NONE"; break;
            case COMPRESSION_RLE: mode_str = "RLE"; break;
            case COMPRESSION_LZ_LITE: mode_str = "LZ_LITE"; break;
            default: mode_str = "UNKNOWN"; break;
        }
        printf("Extracted: %s (original: %zu bytes, mode: %s)\n",
               file_name, decompressed_size, mode_str);
        if (compression_mode != COMPRESSION_NONE) {
            free(output_data);
        }
        free(input_data);
    }
    fclose(archive_fp);
    printf("Extraction complete.\n");
    return 0;
}
