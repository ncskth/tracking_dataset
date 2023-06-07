#include <stdio.h>
#include "protocol.h"

#define MAX(a, b) a < b ? b : a
#define MIN(a, b) a < b ? a : b

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("please provide a path to read from\n");
        return -1;
    }
    FILE *f = fopen(argv[1], "r");


    uint32_t pc_timestamp = 0;

    uint32_t min_optitrack_timestamp_delta = ~0;
    uint32_t max_optitrack_timestamp_delta = 0;
    uint32_t min_event_timestamp_delta = ~0;
    uint32_t max_event_timestamp_delta = 0;

    uint64_t num_events;
    uint64_t num_tracks;
    uint64_t num_timestamps;

    uint8_t optitrack_ids[255] = {0};

    uint32_t last_print = 0;

    uint32_t first_print = 0;

    while (!feof(f)) {
        int id = fgetc(f);
        if (id == CAMERA_HEADER) {
            struct camera_header header;
            pc_timestamp = header.pc_t;
            fread(&header, 1, sizeof(header), f);
            for (int i = 0; i < header.num_events; i++) {
                struct camera_event entry;
                fread(&entry, 1, sizeof(entry), f);
                uint32_t ts_delta = entry.t - pc_timestamp;
                min_event_timestamp_delta = MIN(min_event_timestamp_delta, ts_delta);
                max_event_timestamp_delta = MAX(max_event_timestamp_delta, ts_delta);
                num_events++;
            }
        }
        else if (id == TIMESTAMP_HEADER) {
            struct timestamp_header header;
            fread(&header, 1, sizeof(header), f);
            pc_timestamp = header.pc_t;
            num_timestamps++;
        }
        else if (id == OPTITRACK_HEADER) {
            struct optitrack_header header;
            fread(&header, 1, sizeof(header), f);
            optitrack_ids[header.object_id] = 1;
            pc_timestamp = header.pc_t;
            uint32_t ts_delta = header.t - pc_timestamp;
            // printf(" %f %f %f\n", header.pos_x, header.pos_y, header.pos_z);
            min_optitrack_timestamp_delta = MIN(min_optitrack_timestamp_delta, ts_delta);
            max_optitrack_timestamp_delta = MAX(max_optitrack_timestamp_delta, ts_delta);
            num_tracks++;
        }
        else {
            return -2;
        }

        if (pc_timestamp - last_print > 1e6) {
            int sum = 0;
            for (int j = 0; j < 255; j++) {
                sum += optitrack_ids[j];
            }
            last_print = pc_timestamp;
            if (first_print == 0) {
                first_print = pc_timestamp;
            }
            printf("*******%llu*******\n", (pc_timestamp - first_print) / 1000000);
            printf("events: %fM\n", num_events / 1e6);
            printf("events delta: %llu\n", max_event_timestamp_delta - min_event_timestamp_delta);
            if (sum != 0) {
                printf("optitrack: %llu objects at %dHz\n", sum,  num_tracks / sum);
                printf("optitrack delta: %llu\n", max_optitrack_timestamp_delta - min_optitrack_timestamp_delta);
            }
            printf("timestamps: %llu\n", num_timestamps);

            min_optitrack_timestamp_delta = ~0;
            max_optitrack_timestamp_delta = 0;
            min_event_timestamp_delta = ~0;
            max_event_timestamp_delta = 0;
            num_events = 0;
            num_timestamps = 0;
            num_tracks = 0;
        }
    }
}