#include <stdio.h>
#include <string>
#include <vector>

#include "aedat/aedat4.hpp"
#include "protocol.h"
#include "aedat/aer.hpp"

void save_events(std::fstream &out, std::vector<struct camera_event> events) {
    std::vector<AEDAT::PolarityEvent> converted_events {events.size()};
    for (auto v : events) {
        AEDAT::PolarityEvent converted_event {
            .timestamp = v.t,
            .x = v.x,
            .y = v.y,
            .valid = true,
            .polarity = v.p,
        };
        converted_events.push_back(converted_event);
    }
    AEDAT4::save_events(out, converted_events);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: [in file] [out directory]\n");
        return -1;
    }
    std::string metadata_output_path = std::string(argv[2]) + "/metadata.txt";
    std::string object_output_path = std::string(argv[2]) + "/objects.hdf5";
    std::string event_output_path = std::string(argv[2]) + "/events.aedat4";

    std::fstream event_output;
    event_output.open(event_output_path, std::fstream::in | std::fstream::out |
                                    std::fstream::binary | std::fstream::trunc);
    std::fstream object_output;
    object_output.open(object_output_path, std::fstream::in | std::fstream::out |
                                    std::fstream::binary | std::fstream::trunc);
    std::fstream metadata_output;
    metadata_output.open(metadata_output_path, std::fstream::in | std::fstream::out |
                                    std::fstream::binary | std::fstream::trunc);

    FILE *input_file = fopen(argv[1], "r");

    uint32_t first_timestamp = 0;
    uint32_t last_timestamp = 0;
    size_t event_count = 0;

    while (!feof(input_file)) {
        int id = fgetc(input_file);
        if (id == CAMERA_HEADER) {
            struct camera_header header;
            fread(&header, 1, sizeof(header), input_file);
            std::vector<struct camera_event> events {header.num_events};
            for (int i = 0; i < header.num_events; i++) {
                struct camera_event entry;
                fread(&entry, 1, sizeof(entry), input_file);
                events.push_back(entry);

                if (first_timestamp == 0) {
                    first_timestamp = entry.t;
                }
                last_timestamp = entry.t;
                event_count++;
            }
            save_events(event_output, events);
        }
        else if (id == TIMESTAMP_HEADER) {
            struct timestamp_header header;
            fread(&header, 1, sizeof(header), input_file);
        }
        else if (id == OPTITRACK_HEADER) {
            struct optitrack_header header;
            fread(&header, 1, sizeof(header), input_file);
        }
        else {
            return -2;
        }
    }

    // Header
    auto headerOffset = AEDAT4::save_header(event_output);
   // Footer
    AEDAT4::save_footer(event_output, headerOffset, first_timestamp, last_timestamp, event_count);
    event_output.flush();
    event_output.close();
}