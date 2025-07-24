#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>

#define MAX_LINE_LEN 4096

#define GREEN_COLOR "\033[32m"
#define RESET_COLOR "\033[0m"


int opt_s_only_match = 0;
int opt_n_line_numbers = 0;


enum MatchFilterType {
    NO_FILTER = 0,
    GREATER_THAN,        // >N
    LESS_THAN,           // <N
    EQUALS,              // =N
    GREATER_THAN_EQUALS, // >=N
    LESS_THAN_EQUALS     // <=N
};
enum MatchFilterType opt_m_filter_type = NO_FILTER;
int opt_m_filter_value = 0;

static void print_substring(const char *str, int start, int end) {
    for (int i = start; i < end; ++i) {
        putchar(str[i]);
    }
}

static int process_line(char *line, const char *pattern, int line_num) {
    int matches_count = 0;
    char *current_pos = line;
    char *match_pos;
    int line_len = strlen(line);

    if (line_len > 0 && line[line_len - 1] == '\n') {
        line[line_len - 1] = '\0';
        line_len--;
    }

    if (opt_s_only_match) {
        while ((match_pos = strstr(current_pos, pattern)) != NULL) {
            matches_count++;
            printf(GREEN_COLOR);
            print_substring(match_pos, 0, strlen(pattern));
            printf(RESET_COLOR);
            current_pos = match_pos + strlen(pattern);
            if (*current_pos != '\0') {
                putchar(' ');
            }
        }
        if (matches_count > 0) {
            putchar('\n');
        }
    } else {

        char *temp_pos = line;
        while (strstr(temp_pos, pattern) != NULL) {
            matches_count++;
            temp_pos = strstr(temp_pos, pattern) + strlen(pattern);
        }

        int filter_passed = 0;
        switch (opt_m_filter_type) {
            case NO_FILTER: filter_passed = 1; break;
            case GREATER_THAN: filter_passed = (matches_count > opt_m_filter_value); break;
            case LESS_THAN: filter_passed = (matches_count < opt_m_filter_value); break;
            case EQUALS: filter_passed = (matches_count == opt_m_filter_value); break;
            case GREATER_THAN_EQUALS: filter_passed = (matches_count >= opt_m_filter_value); break;
            case LESS_THAN_EQUALS: filter_passed = (matches_count <= opt_m_filter_value); break;
        }

        if (filter_passed) {
            if (opt_n_line_numbers) {
                printf("%6d:", line_num);
            }

            current_pos = line;
            while ((match_pos = strstr(current_pos, pattern)) != NULL) {
                print_substring(current_pos, 0, match_pos - current_pos);
                printf(GREEN_COLOR);
                print_substring(match_pos, 0, strlen(pattern));
                printf(RESET_COLOR);
                current_pos = match_pos + strlen(pattern);
            }
            printf("%s\n", current_pos);
            return 1;
        }
    }
    return 0;
}



void print_mgrip_help() {
    fprintf(stdout, "=================================================================\\n");
    fprintf(stdout, "Usage: mx grep [OPTIONS] <pattern> [file...]\\n");
    fprintf(stdout, "Search for PATTERN in each FILE or standard input.\\n\\n");
    fprintf(stdout, "Options:\\n");
    fprintf(stdout, "  -s        Show only the matched part of the line (self-print).\\n");
    fprintf(stdout, "  -n        Print line numbers.\\n");
    fprintf(stdout, "  -m>N      Filter by more than N matches.\\n");
    fprintf(stdout, "  -m<N      Filter by less than N matches.\\n");
    fprintf(stdout, "  -m=N      Filter by exactly N matches.\\n");
    fprintf(stdout, "  -m>=N     Filter by N or more matches.\\n");
    fprintf(stdout, "  -m<=N     Filter by N or less matches.\\n");
    fprintf(stdout, "  -h        Display this help message.\\n");
    fprintf(stdout, "=========================[MX grep version 0.2]===================\\n");
}

int mgrip_cmd_internal(int argc, char *argv[]) {
    char line_buffer[MAX_LINE_LEN];
    const char *pattern = NULL;
    FILE *input_file = NULL;
    int current_line_num = 0;
    int opt;
    long m_val;
    char *endptr;

    opt_s_only_match = 0;
    opt_n_line_numbers = 0;
    opt_m_filter_type = NO_FILTER;
    opt_m_filter_value = 0;

    optind = 1;

    while ((opt = getopt(argc, argv, "snm:h")) != -1) {
        switch (opt) {
            case 's':
                opt_s_only_match = 1;
                break;
            case 'n':
                opt_n_line_numbers = 1;
                break;
            case 'm':
                if (strlen(optarg) < 2) {
                    fprintf(stderr, "Error: Invalid argument for -m. Expected -m>N, -m<N, etc.\\n");
                    return EXIT_FAILURE;
                }
                char operator = optarg[0];
                m_val = strtol(optarg + 1, &endptr, 10);
                if (*endptr != '\\0' || m_val < 0) {
                    fprintf(stderr, "Error: Invalid numeric value for -m option: '%s'\\n", optarg + 1);
                    return EXIT_FAILURE;
                }
                opt_m_filter_value = (int)m_val;

                if (operator == '>') {
                    if (strlen(optarg) > 2 && optarg[1] == '=') {
                        opt_m_filter_type = GREATER_THAN_EQUALS;
                    } else {
                        opt_m_filter_type = GREATER_THAN;
                    }
                } else if (operator == '<') {
                    if (strlen(optarg) > 2 && optarg[1] == '=') {
                        opt_m_filter_type = LESS_THAN_EQUALS;
                    } else {
                        opt_m_filter_type = LESS_THAN;
                    }
                } else if (operator == '=') {
                    opt_m_filter_type = EQUALS;
                } else {
                    fprintf(stderr, "Error: Invalid operator for -m option. Use '>', '<', '=', '>=', '<='.\\n");
                    return EXIT_FAILURE;
                }
                break;
            case 'h':
                print_mgrip_help();
                return EXIT_SUCCESS;
            case '?':
                fprintf(stderr, "Usage: mx grep [-s] [-n] [-m>N|-m<N|-m=N|-m>=N|-m<=N] <pattern> [file...]\\n");
                return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        pattern = argv[optind];
        optind++;
    } else {
        fprintf(stderr, "Error: Pattern missing.\\n");
        fprintf(stderr, "Usage: mx grep [-s] [-n] [-m>N|-m<N|-m=N|-m>=N|-m<=N] <pattern> [file...]\\n");
        return EXIT_FAILURE;
    }

    if (optind == argc) {
        while (fgets(line_buffer, sizeof(line_buffer), stdin) != NULL) {
            current_line_num++;
            process_line(line_buffer, pattern, current_line_num);
        }
    } else {

        for (int i = optind; i < argc; ++i) {
            input_file = fopen(argv[i], "r");
            if (!input_file) {
                perror(argv[i]);
                continue;
            }

            current_line_num = 0;
            while (fgets(line_buffer, sizeof(line_buffer), input_file) != NULL) {
                current_line_num++;
                process_line(line_buffer, pattern, current_line_num);
            }
            fclose(input_file);
        }
    }

    return EXIT_SUCCESS;
}
