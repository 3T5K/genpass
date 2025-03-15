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
// TODO: let user chose the characters to use (lower, upper, digits, special)

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#define MIN(a, b) ((a < b) ? a : b)
#define CHAR_OFFSET (uint8_t)33
#define CHAR_AMOUNT (uint8_t)94
#define UPPER_BOUND 100 * 1000
#define BUF_SIZE 50 * 1000
const char *RANDFILE = "/dev/urandom";
const char *HELP =
    "This simple utility generates random passwords\n"
    "in ascii armor from /dev/urandom.\n"
    "\n"
    "USAGE: genpass [OPTIONS]...\n"
    "OPTIONS:\n"
    "    -h, --help   show this message\n"
    "    -r, --random use /dev/random instead\n"
    "    -n, --length password length (def: 30)\n"
    "    -c, --count  amount of passwords (def: 1)\n"
    "\n"
    "The argument to '--length' has an upper bound\n"
    "of 100,000.\n";

void die(void) {
    fprintf(stderr, "try running with '--help' or '-h' for more info\n");
    exit(EXIT_FAILURE);
}

size_t try_parse_num_arg() {
    char *endptr; errno = 0;
    size_t val = strtoul(optarg, &endptr, 10);
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

void armorize(uint8_t *buf, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        buf[i] = buf[i] % CHAR_AMOUNT + CHAR_OFFSET;
    }
}

void genpass() {}

int main(int argc, char **argv) {
    struct option opts[] = {
        {"help",   no_argument,       NULL, 'h'},
        {"random", no_argument,       NULL, 'r'},
        {"length", required_argument, NULL, 'n'},
        {"count",  required_argument, NULL, 'c'},
        {NULL,     0,                 NULL, 0  }
    };

    size_t pw_len = 30, pw_count = 1; int opt;
    while ((opt = getopt_long(argc, argv, "hrn:c:", opts, NULL)) != -1) {
        switch (opt) {
            case 'h': printf("%s", HELP);             return EXIT_SUCCESS;
            case 'r': RANDFILE = "/dev/random";       break;
            case 'n': pw_len = try_parse_num_arg();   break;
            case 'c': pw_count = try_parse_num_arg(); break;
            default: die();
        }
    }

    if (optind != argc) {
        fprintf(stderr, "excess arguments found\n");
        die();
    }
    
    int rand_src = open(RANDFILE, O_RDONLY);
    if (rand_src == -1) {
        fprintf(stderr, "failed to open %s\n", RANDFILE); 
        return EXIT_FAILURE;
    }

    static uint8_t buf[BUF_SIZE] = {0};
    for (size_t pw = 0; pw < pw_count; pw++) {
        for (size_t printed = 0; printed != pw_len;) {
            if (read(rand_src, buf, BUF_SIZE) != BUF_SIZE) {
                fprintf(stderr, "failed to read %i bytes from %s\n", BUF_SIZE, RANDFILE);
                return EXIT_FAILURE;
            }

            armorize(buf, BUF_SIZE);
            size_t out = fwrite(buf, sizeof(uint8_t), MIN(BUF_SIZE, pw_len - printed), stdout);
            if (out != MIN(BUF_SIZE, pw_len - printed)) {
                    // arguably useless, but prevents hanging if that actually somehow fails...
                fprintf(stderr, "failed to write to stdout\n");
                return EXIT_FAILURE;
            }

            printed += out;
        }
        puts("");
    }

    return EXIT_SUCCESS;
}
