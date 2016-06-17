#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mtbl.h>

void check(bool ok, const char* message);
void check_next(struct mtbl_iter* it, char expected);

int main(int argc, char **argv) {
    char path[] = "/tmp/mtbl-test-get-range-XXXXXX";
    int fd = mkstemp(path);
    if (fd == -1) {
        fprintf(
            stderr, "Failed to make tempfile at %s (error %d): %s\n",
            path, errno, strerror(errno)
        );
        return 1;
    }
     
    struct mtbl_writer_options* wopt = mtbl_writer_options_init();
    check(wopt, "mtbl_writer_options_init");
    struct mtbl_writer* writer = mtbl_writer_init_fd(fd, wopt);
    check(writer, "mtbl_writer_init_fd");
    mtbl_writer_options_destroy(&wopt);
    
    for (unsigned char c = 'a'; c < 'g'; ++c) {
        check(
            mtbl_writer_add(writer, &c, 1, &c, 1) == mtbl_res_success,
            "mtbl_writer_add"
        );
    }
    
    mtbl_writer_destroy(&writer);
    check(close(fd) == 0, "close");
    
    struct mtbl_reader* reader = mtbl_reader_init(path, NULL);
    check(reader, "mtbl_reader_init");

    const struct mtbl_source* source = mtbl_reader_source(reader);
    
    struct mtbl_iter* it =
        mtbl_source_get_range(source, (uint8_t*)"b", 1, (uint8_t*)"cc", 2);
    check_next(it, 'b');
    check_next(it, 'c');
    check_next(it, 0);
    mtbl_iter_destroy(&it);

    // A zero-length null "end" is equivalent to an empty key.
    it = mtbl_source_get_range(source, (uint8_t*)"b", 1, NULL, 0);
    check_next(it, 0);
    mtbl_iter_destroy(&it);

    // A positive-length null "end" means "iterate until the end".
    it = mtbl_source_get_range(source, (uint8_t*)"b", 1, NULL, 1);
    for (char c = 'b'; c < 'g'; ++c) {
        check_next(it, c);
    }
    check_next(it, 0);
    mtbl_iter_destroy(&it);

    // A zero-length key always sorts first
    it = mtbl_source_get_range(source, NULL, 0, (uint8_t*)"b", 1);
    check_next(it, 'a');
    check_next(it, 'b');
    check_next(it, 0);
    mtbl_iter_destroy(&it);

    mtbl_reader_destroy(&reader);
      
    return 0;
}

void check(bool ok, const char* err) {
    if (!ok) {
        if (errno) {
            fprintf(stderr, "Error %d: %s: %s", errno, err, strerror(errno));
        } else {
            fprintf(stderr, "Error: %s", err);
        }
        exit(1);
    }
}

void check_next(struct mtbl_iter* it, char expected) {
    uint8_t e = expected;
    const uint8_t* key;
    const uint8_t* val;
    size_t key_len, val_len;
    mtbl_res res = mtbl_iter_next(it, &key, &key_len, &val, &val_len);
    if (expected) {
        check(res == mtbl_res_success, "mtbl_iter_next success");
        check(key_len == 1 && val_len == 1, "key or value length != 1");
        check(key[0] == e && val[0] == e, "wrong key or value");
    } else {
        check(res == mtbl_res_failure, "mtbl_iter_next end");
    }
}
