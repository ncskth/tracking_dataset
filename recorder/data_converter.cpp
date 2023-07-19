#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <filesystem>
#include <iomanip>

#include "aedat/aedat4.hpp"
#include "protocol.h"
#include "aedat/aer.hpp"

#define CONVERTER_VERSION "1.0"

#define SKIP_BEGINNING 5000000

#define MEAN_FROM_OPTITRACK_DELTAS 500

#define EVENT_RANDOM_SAMPLING 1000
#define MEAN_FROM_EVENT_DELTAS (300 * 5)

using json = nlohmann::json;

struct optitrack_object {
    uint32_t t = 0;
    float x;
    float y;
    float z;
    float qw;
    float qx;
    float qy;
    float qz;
};

template <typename T>
T interpolate(float start_time, T start_value, float end_time, T end_value, float wanted_time) {
    return (wanted_time - start_time) / (end_time - start_time) * (end_value - start_value) + start_value;
}


std::map<int, std::vector<optitrack_object>> optitrack_data; //we should be able to fit many many hours in memory

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: [in file] [out directory]\n");
        return -1;
    }
    std::filesystem::create_directory(std::string(argv[2]));
    std::string metadata_output_path = std::string(argv[2]) + "/metadata.txt";
    std::string object_output_path = std::string(argv[2]) + "/positions.json";
    std::string event_output_path = std::string(argv[2]) + "/events.aedat4";

    std::fstream event_output;
    event_output.open(event_output_path, std::fstream::out | std::fstream::in | std::fstream::trunc |
                                    std::fstream::binary);
    std::fstream object_output;
    object_output.open(object_output_path, std::fstream::out |
                                    std::fstream::binary);
    std::fstream metadata_output;
    metadata_output.open(metadata_output_path, std::fstream::out |
                                    std::fstream::binary);

    FILE *input_file = fopen(argv[1], "r");

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    metadata_output << "date: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << std::endl;
    metadata_output << "source file: " << argv[1] << std::endl;
    metadata_output << "version: " << CONVERTER_VERSION << std::endl;

    uint32_t first_pc_timestamp = 0;
    bool active = false;
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
                fread(&entry, 1, sizeof(entry), input_file);
                if (not active) {
                    continue;
                }
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
    first_pc_timestamp = 0;
    uint32_t first_event = 0;
    uint32_t last_event = 0;
    size_t event_count = 0;

    int aedat_header_offset = AEDAT4::save_header(event_output);
    while (!feof(input_file)) {
        int id = fgetc(input_file);
        if (id == CAMERA_HEADER) {
            struct camera_header header;
            fread(&header, 1, sizeof(header), input_file);
            std::vector<AEDAT::PolarityEvent> events;
            for (int i = 0; i < header.num_events; i++) {
                struct camera_event entry;
                fread(&entry, 1, sizeof(entry), input_file);
                if (not active) {
                    continue;
                }
                uint32_t t_adjusted = entry.t - event_correction - start_timestamp;
                // printf("t adjusted %d\n", t_adjusted);
                AEDAT::PolarityEvent formal_event =  {
                    .timestamp = t_adjusted,
                    .x = entry.x,
                    .y = entry.y,
                    .valid = true,
                    .polarity = entry.p ? true : false,
                };
                if (first_event == 0) {
                    first_event = t_adjusted;
                }
                event_count++;
                last_event = t_adjusted;
                events.push_back(formal_event);
            }
            AEDAT4::save_events(event_output, events);
        }
        else if (id == TIMESTAMP_HEADER) {
            struct timestamp_header header;
            fread(&header, 1, sizeof(header), input_file);
            if (first_pc_timestamp == 0) {
                first_pc_timestamp = header.pc_t;
            }
            if (not active && header.pc_t - first_pc_timestamp > SKIP_BEGINNING) {
                start_timestamp = header.pc_t - SKIP_BEGINNING;
                active = true;
            }
        }
        else if (id == OPTITRACK_HEADER) {
            struct optitrack_header header;
            fread(&header, 1, sizeof(header), input_file);
            if (not active) {
                continue;
            }
            uint32_t t_adjusted = header.t - optitrack_correction - start_timestamp;
            optitrack_object object = {
                .t = t_adjusted,
                .x = header.pos_x,
                .y = header.pos_y,
                .z = header.pos_z,
                .qw = header.q_w,
                .qx = header.q_x,
                .qy = header.q_y,
                .qz = header.q_z,
            };
            optitrack_data[header.object_id].push_back(object);
        }
        else {
            return -2;
        }
    }

    AEDAT4::save_footer(event_output, aedat_header_offset, first_event, last_event, event_count);
    event_output.flush();
    event_output.close();

    json optitrack_json;

    for (auto v : optitrack_data) {
        std::string id = std::to_string(v.first);
        std::vector<optitrack_object> tracks = v.second;
        optitrack_object previous_object;
        for (auto object : tracks) {
            // std::map<std::string, float> object_map;
            // object_map["t"] = object.t;
            // object_map["x"] = object.x;
            // object_map["y"] = object.y;
            // object_map["z"] = object.z;
            // object_map["qw"] = object.qw;
            // object_map["qx"] = object.qx;
            // object_map["qy"] = object.qy;
            // object_map["qz"] = object.qz;
            // optitrack_json["data"][id].push_back(object_map);

            if (previous_object.t != 0) {
                for (int i = previous_object.t / 1000; i < object.t / 1000; i++) {
                    std::map<std::string, float> interp_map;
                    interp_map["t"] = interpolate(previous_object.t, previous_object.t, object.t, object.t, i * 1000);
                    interp_map["x"] = interpolate(previous_object.t, previous_object.x, object.t, object.x, i * 1000);
                    interp_map["y"] = interpolate(previous_object.t, previous_object.y, object.t, object.y, i * 1000);
                    interp_map["z"] = interpolate(previous_object.t, previous_object.z, object.t, object.z, i * 1000);
                    interp_map["qw"] = interpolate(previous_object.t, previous_object.qw, object.t, object.qw, i * 1000);
                    interp_map["qx"] = interpolate(previous_object.t, previous_object.qx, object.t, object.qx, i * 1000);
                    interp_map["qy"] = interpolate(previous_object.t, previous_object.qy, object.t, object.qy, i * 1000);
                    interp_map["qz"] = interpolate(previous_object.t, previous_object.qz, object.t, object.qz, i * 1000);
                    optitrack_json["data"][id].push_back(interp_map);
                }
            }
            previous_object = object;
        }
    }
    object_output << optitrack_json;
}