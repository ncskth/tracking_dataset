#include <stdio.h>
#include "protocol.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        return -1;
    }
    FILE *f = fopen(argv[1], "r");

    while (!feof(f)) {
        int id = fgetc(f);
        // printf("id: %d\n", id);
        if (id == CAMERA_HEADER) {
            static uint64_t t = 0;
            static uint64_t events = 0;
            struct camera_header header;
            fread(&header, 1, sizeof(header), f);
            for (int i = 0; i < header.num_events; i++) {
                struct camera_event entry;
                fread(&entry, 1, sizeof(entry), f);
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
            if (header.t - prev > 1000000) {
                printf("timestamp %lld: %lld\n", header.t / 1000000, i);
                prev = header.t;
                i = 0;
            }
        }
        else if (id == OPTITRACK_HEADER) {
            static uint32_t prev;
            static int i;
            static uint8_t ids[255] = {0};
            struct optitrack_header header;
            fread(&header, 1, sizeof(header), f);
            i++;
            ids[header.object_id] = 1;
            if (header.t - prev > 1000000) {
                int sum = 0;
                for (int j = 0; j < 256; j++) {
                    sum += ids[j];
                }
                printf("tracking %d objects at %fHz\n", sum,  (float) i / sum);
                i = 0;
                prev = header.t;
            }
        }
        else {
            return -2;
        }
    }
}