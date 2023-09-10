#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <h5pp/h5pp.h>
#include <thread>

#include "aedat/aedat4.hpp"
#include "protocol.h"
#include "aedat/aer.hpp"

#define CONVERTER_VERSION "1.0"

#define SKIP_BEGINNING 6000000

#define MEAN_FROM_OPTITRACK_DELTAS 500

#define EVENT_RANDOM_SAMPLING 1000
#define MEAN_FROM_EVENT_DELTAS (300 * 5)

#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720

#define FRAME_DELTA 1000

#define SAVE_FRAMES_AFTER 8000000

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


const int frame_length = 720 * 1280;
std::array<std::array<uint16_t, frame_length>, 1> event_frame;
// std::array<uint8_t, frame_length> event_frame;
uint64_t frame_index = 0;

std::string optitrack_id_to_name(enum optitrack_ids id) {
    switch (id) {
        case CAMERA0:
        case CAMERA1:
        case CAMERA2:
        case CAMERA3:
        case CAMERA4:
        case CAMERA5:
        case CAMERA6:
        case CAMERA7:
        case CAMERA8:
        case CAMERA9:
            return std::string("camera");
        case RECTANGLE0:
            return std::string("rectangle");
        case SQUARE0:
            return std::string("square");
        case CIRCLE0:
            return std::string("circle");
        case TRIANGLE0:
            return std::string("triangle");
        case CHECKERBOARD:
            return std::string("checkerboard");
        default:
            return std::string("lol wut");
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: [in file] [out directory]\n");
        return -1;
    }
    std::filesystem::create_directory(std::string(argv[2]));
    std::string metadata_output_path = std::string(argv[2]) + "/metadata.txt";
    std::string object_output_path = std::string(argv[2]) + "/positions.json";
    std::string event_output_path = std::string(argv[2]) + "/events.aedat4";
    std::string frame_output_path = std::string(argv[2]) + "/frames.h5";

    std::fstream event_output;
    event_output.open(event_output_path, std::fstream::out | std::fstream::in | std::fstream::trunc |
                                    std::fstream::binary);
    std::fstream object_output;
    object_output.open(object_output_path, std::fstream::out |
                                    std::fstream::binary);
    std::fstream metadata_output;
    metadata_output.open(metadata_output_path, std::fstream::out |
                                    std::fstream::binary);

    h5pp::File frame_file(frame_output_path, h5pp::FileAccess::REPLACE);

    std::array<std::array<uint16_t, frame_length>, 0> init_frame;
    frame_file.setCompressionLevel(1);
    frame_file.writeDataset(init_frame, "frames", H5D_CHUNKED, {0, frame_length}, {3, frame_length});
    // frame_file.appendToDataset(event_frame, "frames", 0);

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
    uint32_t optitrack_max_time = 0;
    uint32_t event_max_time = 0;
    // first sweep for time sync
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
                event_max_time = entry.t;
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
            optitrack_max_time = header.t;
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

    event_max_time = event_max_time - event_correction;
    optitrack_max_time = optitrack_max_time - optitrack_correction;
    uint32_t frame_max_time = std::min(event_max_time, optitrack_max_time);

    fseek(input_file, 0, SEEK_SET);
    active = false;
    uint32_t start_timestamp = 0;
    first_pc_timestamp = 0;
    uint32_t first_event = 0;
    uint32_t last_event = 0;
    size_t event_count = 0;

    int aedat_header_offset = AEDAT4::save_header(event_output);
    uint32_t last_event_frame_time;
    uint32_t last_optitrack_frame_time;
    // second sweep for data
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
                int index = entry.x + entry.y * FRAME_WIDTH;
                if (entry.p) {
                    ((uint8_t*) &event_frame[0][index])[0]++;
                } else {
                    ((uint8_t*) &event_frame[0][index])[1]++;
                }
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
                //optitrack interpolation misses the last frame
                if (t_adjusted >= frame_index * FRAME_DELTA + SAVE_FRAMES_AFTER
                && t_adjusted <= frame_max_time - start_timestamp - FRAME_DELTA) {
                    frame_file.appendToDataset(event_frame, "frames", 0, {1, frame_length});
                    frame_index++;
                    last_event_frame_time = t_adjusted;
                    // zero out frame
                    int negative_events = 0;
                    int positive_events = 0;
                    for (int i = 0; i < frame_length; i++) {
                        negative_events += (event_frame[0][i] & 0xff00) >> 8;
                        positive_events += event_frame[0][i] & 0x00ff;
                        event_frame[0][i] = 0;
                    }
                    // printf("events frame %d %d \n", positive_events, negative_events);
                }
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
        std::string name = optitrack_id_to_name((enum optitrack_ids) std::stoi(id));
        std::vector<std::array<float, 7>> ahrs_vector;

        for (auto object : tracks) {
            if (previous_object.t != 0) {
                for (int i = previous_object.t / FRAME_DELTA; i < object.t / FRAME_DELTA; i++) {
                    std::map<std::string, float> interp_map;
                    interp_map["t"] = interpolate(previous_object.t, previous_object.t, object.t, object.t, i * FRAME_DELTA);
                    interp_map["x"] = interpolate(previous_object.t, previous_object.x, object.t, object.x, i * FRAME_DELTA);
                    interp_map["y"] = interpolate(previous_object.t, previous_object.y, object.t, object.y, i * FRAME_DELTA);
                    interp_map["z"] = interpolate(previous_object.t, previous_object.z, object.t, object.z, i * FRAME_DELTA);
                    interp_map["qw"] = interpolate(previous_object.t, previous_object.qw, object.t, object.qw, i * FRAME_DELTA);
                    interp_map["qx"] = interpolate(previous_object.t, previous_object.qx, object.t, object.qx, i * FRAME_DELTA);
                    interp_map["qy"] = interpolate(previous_object.t, previous_object.qy, object.t, object.qy, i * FRAME_DELTA);
                    interp_map["qz"] = interpolate(previous_object.t, previous_object.qz, object.t, object.qz, i * FRAME_DELTA);
                    optitrack_json["data"][id].push_back(interp_map);
                    last_optitrack_frame_time = i * FRAME_DELTA;

                    if (i * FRAME_DELTA >= SAVE_FRAMES_AFTER && i * FRAME_DELTA <= frame_max_time - start_timestamp) {
                        std::array<float, 7> ahrs {interp_map["x"], interp_map["y"], interp_map["z"], interp_map["qw"], interp_map["qx"], interp_map["qy"], interp_map["qz"]};
                        ahrs_vector.push_back(ahrs);
                        // last_optitrack_frame_time = i * FRAME_DELTA;
                    }
                }
            }
            previous_object = object;
        }
        frame_file.writeDataset(ahrs_vector, name, {ahrs_vector.size(), 7});
    }
    object_output << optitrack_json;

    printf("last_event_frame_time %u\n", last_event_frame_time);
    printf("last_optitrack_frame_time %u\n", last_optitrack_frame_time);
    printf("optitrack max time %u\n", optitrack_max_time);
    printf("event max time %u\n", event_max_time);
    printf("frame_max_time %u\n", frame_max_time);
    printf("start_timestamp %u\n", start_timestamp);
    if (last_event_frame_time != last_optitrack_frame_time) {
        printf("Something is probably wrong with the frames\n");
    }
}