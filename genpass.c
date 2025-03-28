/*
 *  genpass.c
 *
 *  Copyright (C) 2025

 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// TODO: remove bias in the armorization
// TODO: optimize writes

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define MIN(a, b) ((a < b) ? a : b)
#define BUF_SIZE 50 * 1000
#define C_UPPR "abcdefghijklmnopqrstuvwxyz"
#define C_LOWR "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define C_DIGT "0123456789"
#define C_SPEC "`~!@#$%^&*()_-+={}[]:;'\"\\|/?.>,<"
#define C_FULL C_UPPR C_LOWR C_DIGT C_SPEC
#define C_FAIR C_UPPR C_LOWR C_DIGT "&@"
#define C_FULL_COUNT (uint8_t)94
#define M_DFLT 0  // 0b00000000
#define M_SPEC 1  // 0b00000001
#define M_UPPR 2  // 0b00000010
#define M_LOWR 4  // 0b00000100
#define M_DIGT 8  // 0b00001000
#define M_FAIR 16 // 0b00010000
const char *P_RNG      = "/dev/urandom";
const char *const HELP =
    "This simple utility generates random passwords in ASCII\n"
    "armor using data from /dev/urandom.\n"
    "\n"
    "USAGE: genpass [OPTIONS]\n"
    "\n"
    "OPTIONS:\n"
    "    -h, --help           show this message\n"
    "    -r, --random         use /dev/random instead\n"
    "    -n, --length  <NUM>  password length (def: 30)\n"
    "    -c, --count   <NUM>  amount of passwords (def: 1)\n"
    "    -o, --out     <PATH> file to write to (def: stdout)\n"
    "    -u, --upper          use uppercase letters\n"
    "    -l, --lower          use lowercase letters\n"
    "    -s, --special        use special characters\n"
    "    -d, --digits         use digits\n"
    "    -f, --fair           use a 64 character charset\n"
    "\n"
    "By default, the flags '-u', '-l', '-s', and '-d' are\n"
    "enabled. If any of these flags (including '-f') is used,\n"
    "only the specified characters will be included in the\n"
    "output.\n"
    "Rejection sampling is not performed. For unbiased output,\n"
    "use '--fair'.\n";

void die(void) {
    fprintf(stderr, "try running with '--help' or '-h' for more info\n");
    exit(EXIT_FAILURE);
}

size_t try_parse_num_arg() {
    char *end; errno = 0;
    const size_t val = strtoul(optarg, &end, 10);
    if (errno) {
        fprintf(stderr, "argument for '--length' out of range\n");
        die();
    }

    if (*end != '\0') {
        fprintf(stderr, "argument for '--length' contains unexpected character(s)\n");
        die(); 
    }

    return val;
}

int main(const int argc, char *const *const argv) {
    const struct option opts[] = {
        {"help",    no_argument,       NULL, 'h'},
        {"random",  no_argument,       NULL, 'r'},
        {"length",  required_argument, NULL, 'n'},
        {"count",   required_argument, NULL, 'c'},
        {"out",     required_argument, NULL, 'o'},
        {"upper",   no_argument,       NULL, 'u'},
        {"lower",   no_argument,       NULL, 'l'},
        {"special", no_argument,       NULL, 's'},
        {"digits",  no_argument,       NULL, 'd'},
        {"fair",    no_argument,       NULL, 'f'},
        {NULL,      0,                 NULL, 0  }
    };

    size_t      pw_len                    = 30;
    size_t      pw_count                  = 1;
    const char *p_dst                     = NULL;
    static char charset[C_FULL_COUNT + 1] = {0};
    int         c_mask                    = M_DFLT;
    size_t      c_len;
    int         opt;
    while ((opt = getopt_long(argc, argv, "hrn:c:o:ulsdf", opts, NULL)) != -1) {
        switch (opt) {
            case 'h': printf("%s", HELP);             return EXIT_SUCCESS;
            case 'r': P_RNG = "/dev/random";          break;
            case 'o': p_dst = optarg;                 break;
            case 'n': pw_len = try_parse_num_arg();   break;
            case 'c': pw_count = try_parse_num_arg(); break;
            case 'u': c_mask |= M_UPPR;               break;
            case 'l': c_mask |= M_LOWR;               break;
            case 's': c_mask |= M_SPEC;               break;
            case 'd': c_mask |= M_DIGT;               break;
            case 'f': c_mask  = M_FAIR;               break;
            default: die();
        }
    }

    if (optind != argc) {
        fprintf(stderr, "excess arguments found\n");
        die();
    }

    if (c_mask == M_DFLT) {
        strcpy(charset, C_FULL);
    } else if (c_mask == M_FAIR) {
        strcpy(charset, C_FAIR);
    } else {
        if (c_mask & M_UPPR) { strcat(charset, C_UPPR); }
        if (c_mask & M_LOWR) { strcat(charset, C_LOWR); }
        if (c_mask & M_SPEC) { strcat(charset, C_SPEC); }
        if (c_mask & M_DIGT) { strcat(charset, C_DIGT); }
    }

    c_len = strlen(charset);
    const int fd_rng = open(P_RNG, O_RDONLY);
    if (fd_rng == -1) {
        fprintf(stderr, "failed to open \"%s\"\n", P_RNG); 
        return EXIT_FAILURE;
    }

    const int fd_dst = (p_dst) ? open(p_dst, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR) : STDOUT_FILENO;
    if (fd_dst == -1) {
        fprintf(stderr, "failed to open \"%s\": ", p_dst);
        perror("");
        return EXIT_FAILURE;
    }

    static uint8_t buf[BUF_SIZE] = {0};
    for (size_t cursor   = BUF_SIZE,
                written  = 0,
                password = 0; password != pw_count;) {
        if (cursor == BUF_SIZE) {
            cursor = 0;
            if (read(fd_rng, buf, BUF_SIZE) != BUF_SIZE) {
                fprintf(stderr, "failed to read %i bytes from \"%s\"\n", BUF_SIZE, P_RNG);
                return EXIT_FAILURE;
            }

            for (size_t i = 0; i < BUF_SIZE; i++) {
                buf[i] = charset[buf[i] % c_len];
            }
        }

        const size_t n = MIN(BUF_SIZE - cursor, pw_len - written);
        const size_t w = write(fd_dst, buf + cursor, n);
        if (w != n) {
            fprintf(stderr, "failed to write to \"%s\"\n", p_dst);
            return EXIT_FAILURE;
        }

        written += w; cursor += w;
        if (written == pw_len) {
            written = 0; ++password;
            if (write(fd_dst, "\n", 1) != 1) {
                fprintf(stderr, "failed to write to \"%s\"\n", p_dst);
                return EXIT_FAILURE;
            };
        }
    }

    return EXIT_SUCCESS;
}
