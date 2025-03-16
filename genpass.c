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

// TODO: remove bias in armorize

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
#define CHARS_UPPER "abcdefghijklmnopqrstuvwxyz"
#define CHARS_LOWER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define CHARS_DIGITS "0123456789"
#define CHARS_SPECIAL "`~!@#$%^&*()_-+={}[]:;'\"\\|/?.>,<"
#define CHARS_ALL CHARS_UPPER CHARS_LOWER CHARS_DIGITS CHARS_SPECIAL
#define CHARS_NO_BIAS CHARS_UPPER CHARS_LOWER CHARS_DIGITS ":@"
#define CHAR_AMOUNT (uint8_t)94
#define C_DFLT 0  // 0b0000 0000
#define C_SPEC 1  // 0b0000 0001
#define C_UPPR 2  // 0b0000 0010
#define C_LOWR 4  // 0b0000 0100
#define C_DIGT 8  // 0b0000 1000
#define C_FAIR 16 // 0b0001 0000
#define UPPER_BOUND 100 * 1000
#define BUF_SIZE 50 * 1000
const char *RANDFILE   = "/dev/urandom";
const char *const HELP =
    "This simple utility generates random passwords in ASCII armor\n"
    "using data from /dev/urandom.\n"
    "\n"
    "USAGE: genpass [OPTIONS]...\n"
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
    "By default, the flags '-u', '-l', '-s', and '-d' are enabled.\n"
    "If any of these flags (including '-f') is used, only the specified\n"
    "characters will be included in the output.\n"
    "Bias rejection is not performed. For unbiased output, use '--fair'.\n";
    // "The argument to '--length' or '--count' has an\n"
    // "upper bound of (size_t)-1.\n";

void die(void) {
    fprintf(stderr, "try running with '--help' or '-h' for more info\n");
    exit(EXIT_FAILURE);
}

size_t try_parse_num_arg() {
    char *endptr; errno = 0;
    const size_t val = strtoul(optarg, &endptr, 10);
    if (errno) {
        fprintf(stderr, "argument for '--length' out of range\n");
        die();
    }

    if (*endptr != '\0') {
        fprintf(stderr, "argument for '--length' contains unexpected character(s)\n");
        die(); 
    }

    return val;
}

void armorize(
    uint8_t *buf,
    const size_t sz,
    const char *const charset,
    const size_t cs_len
) {
    for (size_t i = 0; i < sz; i++) {
        buf[i] = charset[buf[i] % cs_len];
    }
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

    size_t pw_len = 30, pw_count = 1; const char *dst_path = NULL; int opt;
    static char charset[CHAR_AMOUNT + 1] = {0}; size_t cs_len; int cs = C_DFLT;
    while ((opt = getopt_long(argc, argv, "hrn:c:o:ulsdf", opts, NULL)) != -1) {
        switch (opt) {
            case 'h': printf("%s", HELP);             return EXIT_SUCCESS;
            case 'r': RANDFILE = "/dev/random";       break;
            case 'o': dst_path = optarg;              break;
            case 'n': pw_len = try_parse_num_arg();   break;
            case 'c': pw_count = try_parse_num_arg(); break;
            case 'u': cs |= C_UPPR;                   break;
            case 'l': cs |= C_LOWR;                   break;
            case 's': cs |= C_SPEC;                   break;
            case 'd': cs |= C_DIGT;                   break;
            case 'f': cs =  C_FAIR;                   break;
            default: die();
        }
    }

    if (optind != argc) {
        fprintf(stderr, "excess arguments found\n");
        die();
    }

    if (cs == C_DFLT) {
        strcpy(charset, CHARS_ALL);
    } else if (cs == C_FAIR) {
        strcpy(charset, CHARS_NO_BIAS);
    } else {
        if (cs & C_UPPR) { strcat(charset, CHARS_UPPER);   }
        if (cs & C_LOWR) { strcat(charset, CHARS_LOWER);   }
        if (cs & C_SPEC) { strcat(charset, CHARS_SPECIAL); }
        if (cs & C_DIGT) { strcat(charset, CHARS_DIGITS);  }
    }

    cs_len = strlen(charset);
    const int rand_src = open(RANDFILE, O_RDONLY);
    if (rand_src == -1) {
        fprintf(stderr, "failed to open %s\n", RANDFILE); 
        return EXIT_FAILURE;
    }

    const int f_out = (dst_path) ? open(dst_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR) : STDOUT_FILENO;
    if (f_out == -1) {
        fprintf(stderr, "failed to open \"%s\": ", dst_path);
        perror("");
        return EXIT_FAILURE;
    }

    static uint8_t buf[BUF_SIZE] = {0};
    for (size_t pw = 0; pw < pw_count; pw++) {
        for (size_t printed = 0; printed != pw_len;) {
            if (read(rand_src, buf, BUF_SIZE) != BUF_SIZE) {
                fprintf(stderr, "failed to read %i bytes from %s\n", BUF_SIZE, RANDFILE);
                return EXIT_FAILURE;
            }

            armorize(buf, BUF_SIZE, charset, cs_len);
            const size_t out = write(f_out, buf, MIN(BUF_SIZE, pw_len - printed));
            if (out != MIN(BUF_SIZE, pw_len - printed)) {
                fprintf(stderr, "failed to write to %s\n", dst_path);
                return EXIT_FAILURE;
            }

            printed += out;
        }

        if (write(f_out, "\n", 1) != 1) {
            fprintf(stderr, "failed to write to %s\n", dst_path);
            return EXIT_FAILURE;
        };
    }

    return EXIT_SUCCESS;
}
