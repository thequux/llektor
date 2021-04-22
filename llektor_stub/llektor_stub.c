//
//

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#define MODID_SZ 16
#define PAGE_SZ 4096

static off_t file_size = 0;
static int trace_file_fd = -1;

struct tbuf_record {
    char modid[MODID_SZ];
    struct tbuf_record* next;
    void* buf;
};

static struct tbuf_record* records = NULL;

static uint8_t read_buf[PAGE_SZ];

static void open_tbuf() {
    if (trace_file_fd != -1) {
        return;
    }

    char* tbuf_file = getenv("LLEKTOR_TRACE");
    if (tbuf_file == NULL) {
        trace_file_fd = -2;
    } else {
        trace_file_fd = open(tbuf_file, O_RDWR | O_CREAT, 0666);
        if (trace_file_fd < 0) {
            err(1, "Failed to open trace file");
        }
        // read existing records in
        ssize_t filepos = lseek(trace_file_fd, 0, SEEK_END);
        if (filepos > 0) {
            filepos &= -PAGE_SZ;
            file_size = filepos;
            uint8_t *base_buf = mmap(NULL, filepos, PROT_READ | PROT_WRITE, MAP_SHARED, trace_file_fd, 0);
            uint64_t curpos = 0;
            while (curpos < filepos) {
                struct tbuf_record *record = malloc(sizeof *record);
                memcpy(record->modid, base_buf + curpos, MODID_SZ);
                record->buf = base_buf + curpos + 24;
                uint64_t bufsz = 0;
                for (int i = 0; i < 8; i++) {
                    bufsz |= (uint64_t) (base_buf[curpos + 16 + i]) << (i * 8);
                }
                if (bufsz == 0) {
                    // bad file size :-/
                    file_size = curpos;
                    ftruncate(trace_file_fd, file_size);
                    break;
                }
                curpos += bufsz;
                record->next = records;
                records = record;
            }
        }
    }
}

uint8_t* LLEKTOR_get_trace_buf(const char* modid, uint32_t size) {
    // The format of a section is
    // modid: 16 bytes
    // size: 8 bytes -- the size of the region, in bytes, including the modid and size field
    // rest: one byte per bb

    if (trace_file_fd == -1) {
        open_tbuf();
    }

    // scan for existing slot for the module
    for (struct tbuf_record* cur_record = records; cur_record != NULL; cur_record = cur_record->next) {
        if (memcmp(modid, cur_record->modid, MODID_SZ) == 0) {
            return cur_record->buf;
        }
    }

    // We need a new one :-/
    struct tbuf_record* new_rec = malloc(sizeof(struct tbuf_record));
    memcpy(new_rec->modid, modid, MODID_SZ);
    if (trace_file_fd < 0) {
        new_rec->buf = malloc(size);
    } else {
        uint64_t len = ((uint64_t)size + PAGE_SZ - 1 + 24) & -PAGE_SZ;
        // extend the file.
        ftruncate(trace_file_fd, file_size + len);
        uint8_t *buf = mmap(NULL, len, PROT_WRITE | PROT_READ, MAP_SHARED, trace_file_fd, file_size);
        file_size += len;

        memcpy(buf, modid, MODID_SZ);
        for (int i = 0; i < 8; i++) {
            buf[i + MODID_SZ] = (len >> (i*8)) & 0xff;
        }
        new_rec->buf = buf + MODID_SZ + 8;
    }
    new_rec->next = records;
    records = new_rec;
    return new_rec->buf;
}