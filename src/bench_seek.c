#define NULL 0L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "libmy/print_string.h"
#include <mtbl.h>

static const char *program_name;
static char *target_key;
static char *mtbl_file;

static void
usage(void)
{
    fprintf(stderr,
        "Usage: %s [-t <TARGET_KEY>] [-m <MTBL_FILE>]\n"
        ,
        program_name
    );
    exit(EXIT_FAILURE);
}

int
main(int argc, char **argv) {
    int c;
    bool use_gallop = false;
    struct mtbl_reader *r;
    struct mtbl_reader_options *opt;
    const struct mtbl_source *s1;
    struct mtbl_iter *it;
    const uint8_t *key, *val;
    size_t klen, vlen;

    program_name = argv[0];

    while ((c = getopt(argc, argv, "t:m:g")) != -1) 
    {
        switch (c) 
        {
        case 't':
            target_key = optarg;
            break;
        case 'm':
            mtbl_file = optarg;
            break;
        case 'g':
            use_gallop = true;
            break;
        default:
            usage();
        }
    }
    if (target_key == NULL || mtbl_file == NULL) {
        usage();
    }

    const uint8_t *prefix = (uint8_t *)target_key;

    opt = mtbl_reader_options_init();
    mtbl_reader_options_set_block_search(opt, use_gallop);
    r = mtbl_reader_init(mtbl_file, opt);
    mtbl_reader_options_destroy(&opt);

    s1 = mtbl_reader_source(r);

    it = mtbl_source_get(s1, prefix, strlen(target_key));

    int count = 0;

    while (mtbl_iter_next(it, &key, &klen, &val, &vlen)) {
        print_string(key, klen, stdout);
        fputc(' ', stdout);
        print_string(val, vlen, stdout);
        fputc('\n', stdout);
        count++;
    }

    mtbl_iter_destroy(&it);
    mtbl_reader_destroy(&r);

    printf("Done! Found %d keys\n", count);
    return 0;
}