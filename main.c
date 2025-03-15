#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <getopt.h>

#define CHAR_OFFSET (uint8_t)33
#define CHAR_AMOUNT (uint8_t)94
#define UPPER_BOUND 100 * 1000
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
    "\n"
    "The argument to '--length' has an upper bound\n"
    "of 100,000.\n";

void die(void) {
    fprintf(stderr, "try running with '--help' or '-h' for more info\n");
    exit(EXIT_FAILURE);
}

void armorize(uint8_t *buf, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        buf[i] = buf[i] % CHAR_AMOUNT + CHAR_OFFSET;
    }
}

int main(int argc, char **argv) {
    struct option opts[] = {
        {"help",   no_argument,       NULL, 'h'},
        {"random", no_argument,       NULL, 'r'},
        {"length", required_argument, NULL, 'n'},
        {NULL,     0,                 NULL, 0  }
    };

    size_t buf_sz = 30; int opt;
    while ((opt = getopt_long(argc, argv, "hrn:", opts, NULL)) != -1) {
        switch (opt) {
            case 'h': printf("%s", HELP);       return EXIT_SUCCESS;
            case 'r': RANDFILE = "/dev/random"; break;
            case 'n': {
                char *endptr; errno = 0;
                buf_sz = strtoul(optarg, &endptr, 10);
                if (errno || buf_sz > UPPER_BOUND) {
                    fprintf(stderr, "argument for '--length' out of range\n");
                    die();
                }

                if (*endptr != '\0') {
                    fprintf(stderr, "argument for '--length' contains unexpected character(s)\n");
                    die(); 
                }
            } break;

            default: die(); break;
        }
    }

    if (optind != argc) {
        fprintf(stderr, "excess arguments found\n");
        die();
    }
    
    FILE *rand_src = fopen(RANDFILE, "r");
    if (!rand_src) {
        fprintf(stderr, "failed to open %s", RANDFILE); 
        return EXIT_FAILURE;
    }

    uint8_t buf[buf_sz + 1]; buf[buf_sz] = 0;
    if (fread(buf, sizeof(uint8_t), buf_sz, rand_src) != buf_sz) {
        fprintf(stderr, "failed to read %lu bytes from %s", buf_sz, RANDFILE);
        return EXIT_FAILURE;
    }

    armorize(buf, buf_sz);
    printf("%s\n", buf);
    return EXIT_SUCCESS;
}
