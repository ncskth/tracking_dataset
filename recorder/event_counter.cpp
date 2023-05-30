#include <stdio.h>
#include "protocol.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        return -1;
    }
    FILE *f = fopen(argv[1], "r");
    uint64_t t = 0;
    uint64_t events = 0;
    while (!feof(f)) {
        int id = fgetc(f);
        // printf("id: %d\n", id);
        if (id == CAMERA_HEADER) {
            struct camera_header header;
            fread(&header, 1, sizeof(header), f);
            for (int i = 0; i < header.num_events; i++) {
                struct camera_event entry;
                fread(&entry, 1, sizeof(entry), f);
                // printf("event : %d %d %d %d\n", entry.x, entry.y, entry.p, entry.t / 1000000);
                if (entry.t / 1000000 != t / 1000000) {
                    t = entry.t;
                    printf("Events %lld:%lld\n", entry.t / 1000000, events);
                    events = 0;
                } else {
                    events++;
                }
            }
        }
        else if (id == TIMESTAMP_HEADER) {
            static uint32_t prev;
            static int i = 0;
            struct timestamp_header header;
            fread(&header, 1, sizeof(header), f);
            i += 1;
            float div = 100000;
            if (i == div) {
                i = 0;
                printf("delta: %f\n", (header.t - prev) / div);
                prev = header.t;
            }
        } else {
            return -2;
        }
    }
}