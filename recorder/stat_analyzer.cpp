#include <stdio.h>
#include <vector>
#include <string.h>
#include <stdlib.h>

#include "protocol.h"

#define MAX(a, b) a < b ? b : a
#define MIN(a, b) a < b ? a : b

uint32_t event_counter;

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("usage: [in file] [divider] [out file]\n");
        return -1;
    }
    FILE *f = fopen(argv[1], "r");
    uint32_t divider = atoi(argv[2]);
    FILE *f_out = fopen(argv[3], "w");

    std::vector<uint16_t> deltas;

    while (!feof(f)) {
        int id = fgetc(f);
        if (id == CAMERA_HEADER) {
            struct camera_header header;
            fread(&header, 1, sizeof(header), f);
            for (int i = 0; i < header.num_events; i++) {
                struct camera_event entry;
                fread(&entry, 1, sizeof(entry), f);
                if (event_counter++ % divider == 0) {
                    uint32_t delta = header.pc_t - entry.t;
                    fprintf(f_out, "%lld\n", delta);
                }
            }
        }
        else if (id == TIMESTAMP_HEADER) {
            struct timestamp_header header;
            fread(&header, 1, sizeof(header), f);
        }
        else if (id == OPTITRACK_HEADER) {
            struct optitrack_header header;
            fread(&header, 1, sizeof(header), f);
        }
        else {
            return -2;
        }
    }
}