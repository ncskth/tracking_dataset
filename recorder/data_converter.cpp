#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

#include "aedat/aedat4.hpp"
#include "protocol.h"
#include "aedat/aer.hpp"

#define SKIP_BEGINNING 10000000

#define MEAN_FROM_OPTITRACK_DELTAS 500

#define EVENT_RANDOM_SAMPLING 1000
#define MEAN_FROM_EVENT_DELTAS (300 * 5)

using json = nlohmann::json;

struct optitrack_object {
    uint32_t t;
    float x;
    float y;
    float z;
    float qw;
    float qx;
    float qy;
    float qz;
};


float interpolate(float start_time, float start_value, float end_time, float end_value, float wanted_time) {
    return (wanted_time - start_time) / (end_time - start_time) * (end_value - start_value) + start_value;
}


std::map<int, std::vector<optitrack_object>> optitrack_data; //we should be able to fit many many hours in memory

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

    uint32_t first_pc_timestamp = 0;
    bool active = false;
    size_t event_count = 0;

    // do a first sweep to get timings and such
    std::vector<int> optitrack_deltas;
    std::vector<int> event_deltas;

    // do a second sweep to output data
    while (!feof(input_file)) {
        int id = fgetc(input_file);
        if (id == CAMERA_HEADER) {
            struct camera_header header;
            fread(&header, 1, sizeof(header), input_file);
            for (int i = 0; i < header.num_events; i++) {
                struct camera_event entry;
                if (not active) {
                    continue;
                }
                fread(&entry, 1, sizeof(entry), input_file);
                if (rand() % EVENT_RANDOM_SAMPLING == 0 && event_deltas.size() < MEAN_FROM_OPTITRACK_DELTAS) {
                    event_deltas.push_back(entry.t - header.pc_t);
                }
            }
        }
        else if (id == TIMESTAMP_HEADER) {
            struct timestamp_header header;
            fread(&header, 1, sizeof(header), input_file);
            if (first_pc_timestamp = 0) {
                first_pc_timestamp = header.pc_t;
            }
            if (header.pc_t - first_pc_timestamp > SKIP_BEGINNING) {
                active = true;
            }
        }
        else if (id == OPTITRACK_HEADER) {
            struct optitrack_header header;
            fread(&header, 1, sizeof(header), input_file);
            if (not active) {
                continue;
            }
            if (optitrack_deltas.size() < MEAN_FROM_OPTITRACK_DELTAS) {
                optitrack_deltas.push_back(header.t - header.pc_t);
            }
        }
        else {
            return -2;
        }
        if  (optitrack_deltas.size() >= MEAN_FROM_OPTITRACK_DELTAS && event_deltas.size() >= MEAN_FROM_EVENT_DELTAS) {
            break;
        }
    }

    int optitrack_correction = optitrack_deltas.at(optitrack_deltas.size() / 2);
    int event_correction = event_deltas.at(event_deltas.size() / 2);

    fseek(input_file, 0, SEEK_SET);
    active = false;
    uint32_t start_timestamp = 0;
    while (!feof(input_file)) {
        int id = fgetc(input_file);
        if (id == CAMERA_HEADER) {
            struct camera_header header;
            fread(&header, 1, sizeof(header), input_file);
            for (int i = 0; i < header.num_events; i++) {
                struct camera_event entry;
                fread(&entry, 1, sizeof(entry), input_file);
                if (not active) {
                    continue;
                }
                uint32_t t_adjusted = entry.t - event_correction;
            }
        }
        else if (id == TIMESTAMP_HEADER) {
            struct timestamp_header header;
            fread(&header, 1, sizeof(header), input_file);
            if (first_pc_timestamp = 0) {
                first_pc_timestamp = header.pc_t;
            }
            if (header.pc_t - first_pc_timestamp > SKIP_BEGINNING) {
                start_timestamp = header.pc_t - SKIP_BEGINNING / 4;
                active = true;
            }
        }
        else if (id == OPTITRACK_HEADER) {
            struct optitrack_header header;
            fread(&header, 1, sizeof(header), input_file);
            if (not active) {
                continue;
            }
        }
        else {
            return -2;
        }
        if  (optitrack_deltas.size() >= MEAN_FROM_OPTITRACK_DELTAS && event_deltas.size() >= MEAN_FROM_EVENT_DELTAS) {
            break;
        }
    }

    // Header
    auto headerOffset = AEDAT4::save_header(event_output);
   // Footer
    // AEDAT4::save_footer(event_output, headerOffset, first_timestamp, last_timestamp, event_count);
    event_output.flush();
    event_output.close();

    json optitrack_out;
}