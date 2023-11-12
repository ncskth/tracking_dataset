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
#include <eigen3/Eigen/Geometry>

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

#define PI 3.141525
#define RAD_TO_DEG (180.0/PI)
#define DEG_TO_RAD (PI/180.0)

#define FRAME_DELTA 1000

#define SAVE_FRAMES_AFTER 8000000

#define CAMERA_VERTICAL_FOV (46.0 * DEG_TO_RAD)
#define CAMERA_HORIZONTAL_FOV (80.0 * DEG_TO_RAD)

#define UNDISTORT_K1 -0.07377374561760958
#define UNDISTORT_K2 0.3134840895065068
#define UNDISTORT_K3 0

#define DEG_TO_RAD (PI/180.0)

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

Eigen::Vector2<double> position_to_pixel(Eigen::Vector3<double> pos) {
    Eigen::Vector2<double> out;
    out[0] = atan2(pos.x(), -pos.z()) / (CAMERA_HORIZONTAL_FOV / 2) * FRAME_WIDTH + FRAME_WIDTH / 2;
    out[1] = -atan2(pos.y(), -pos.z()) / (CAMERA_VERTICAL_FOV / 2) * FRAME_HEIGHT + FRAME_HEIGHT / 2;
    return out;
}

Eigen::Vector2<double> undistort_pixel(Eigen::Vector2<double> pixel) {
    Eigen::Vector2<double> out;
    pixel[0] -= FRAME_WIDTH / 2;
    pixel[1] -= FRAME_HEIGHT / 2;
    double r = sqrt(pow(pixel[0], 2) + pow(pixel[1], 2));
    // r /= sqrt(pow(WINDOW_WIDTH, 2) + pow(WINDOW_HEIGHT, 2));
    r /= sqrt(pow(FRAME_WIDTH, 2) + pow(FRAME_HEIGHT, 2) / 2);
    // r /= WINDOW_WIDTH / 2;
    // r = 0;
    double correction = 1 + UNDISTORT_K1 * pow(r, 2) + UNDISTORT_K2 * pow(r, 4) + UNDISTORT_K3 * pow(r, 6);
    out[0] = pixel[0] / correction;
    out[1] = pixel[1] / correction;
    out[0] += FRAME_WIDTH / 2;
    out[1] += FRAME_HEIGHT / 2;
    return out;
}

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
        case PLIER0:
            return std::string("plier");
        case HAMMER0:
        case HAMMER1:
        case HAMMER_NEW:
        case HAMMER_NEW_NEW:
            return std::string("hammer");
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

    std::map<std::string, std::vector<std::array<float, 9>>> ahrs_matrix;


    json optitrack_json;
    int optitrack_track_count = (*optitrack_data.begin()).second.size();
    for (int row = 1; row < optitrack_track_count; row++) {
        Eigen::Vector3<double> future_camera_pos;
        Eigen::Quaternion<double> future_camera_q;
        Eigen::Vector3<double> old_camera_pos;
        Eigen::Quaternion<double> old_camera_q;

        for (auto column : optitrack_data) {
            if (column.first != CAMERA0) {
                continue;
            }
            future_camera_pos = {column.second[row].x, column.second[row].y, column.second[row].z};
            future_camera_q = {column.second[row].qw, column.second[row].qx, column.second[row].qy, column.second[row].qz};
            old_camera_pos = {column.second[row - 1].x, column.second[row - 1].y, column.second[row - 1].z};
            old_camera_q = {column.second[row - 1].qw, column.second[row - 1].qx, column.second[row - 1].qy, column.second[row - 1].qz};
        }

        for (auto column : optitrack_data) {
            if (column.first == CAMERA0) {
                continue;
            }
            Eigen::Vector3<double> future_object_pos = {column.second[row].x, column.second[row].y, column.second[row].z};
            Eigen::Quaternion<double> future_object_q = {column.second[row].qw, column.second[row].qx, column.second[row].qy, column.second[row].qz};
            int t_future = column.second[row].t;
            Eigen::Vector3<double> old_object_pos = {column.second[row - 1].x, column.second[row - 1].y, column.second[row - 1].z};
            Eigen::Quaternion<double> old_object_q = {column.second[row - 1].qw, column.second[row - 1].qx, column.second[row - 1].qy, column.second[row - 1].qz};
            int t_old = column.second[row - 1].t;
            std::string name = optitrack_id_to_name((enum optitrack_ids) column.first);

            for (int i = t_old / FRAME_DELTA; i < t_future / FRAME_DELTA; i++) {
                int t_interp = t_future;
                Eigen::Vector3<double> interp_camera_pos = interpolate(t_old, old_camera_pos, t_future, future_camera_pos, t_interp);
                Eigen::Quaternion<double> interp_camera_q = {
                    interpolate(t_old, old_camera_q.w(), t_future, future_camera_q.w(), t_interp),
                    interpolate(t_old, old_camera_q.x(), t_future, future_camera_q.x(), t_interp),
                    interpolate(t_old, old_camera_q.y(), t_future, future_camera_q.y(), t_interp),
                    interpolate(t_old, old_camera_q.z(), t_future, future_camera_q.z(), t_interp),
                };

                Eigen::Vector3<double> interp_object_pos = interpolate(t_old, old_object_pos, t_future, future_object_pos, t_interp);
                Eigen::Quaternion<double> interp_object_q =  {
                    interpolate(t_old, old_object_q.w(), t_future, future_object_q.w(), t_interp),
                    interpolate(t_old, old_object_q.x(), t_future, future_object_q.x(), t_interp),
                    interpolate(t_old, old_object_q.y(), t_future, future_object_q.y(), t_interp),
                    interpolate(t_old, old_object_q.z(), t_future, future_object_q.z(), t_interp),
                };

                Eigen::Vector3<double> object_relative_pos = interp_camera_q.inverse() * (interp_object_pos - interp_camera_pos);
                Eigen::Quaternion<double> object_relative_q = interp_camera_q.inverse() * interp_object_q;

                Eigen::Vector2<double> pixel = position_to_pixel(object_relative_pos);
                // pixel = undistort_pixel(pixel);
                std::map<std::string, float> interp_map;
                interp_map["t"] = t_interp;
                interp_map["cx"] = pixel.x();
                interp_map["cy"] = pixel.y();
                interp_map["x"] = object_relative_pos[0];
                interp_map["y"] = object_relative_pos[1];
                interp_map["z"] = object_relative_pos[2];
                interp_map["qw"] = object_relative_q.w();
                interp_map["qx"] = object_relative_q.x();
                interp_map["qy"] = object_relative_q.y();
                interp_map["qz"] = object_relative_q.z();
                optitrack_json["data"][name].push_back(interp_map);

                if (i * FRAME_DELTA >= SAVE_FRAMES_AFTER && i * FRAME_DELTA <= frame_max_time - start_timestamp) {
                    std::array<float, 9> ahrs {interp_map["cx"], interp_map["cy"], interp_map["x"], interp_map["y"], interp_map["z"], interp_map["qw"], interp_map["qx"], interp_map["qy"], interp_map["qz"]};
                    ahrs_matrix[name].push_back(ahrs);
                    last_optitrack_frame_time = i * FRAME_DELTA;
                }
            }
        }
    }
    object_output << optitrack_json;

    for (auto v : ahrs_matrix) {
        frame_file.writeDataset(v.second, v.first, {v.second.size(), 9});
    }

    // printf("last_event_frame_time %u\n", last_event_frame_time);
    // printf("last_optitrack_frame_time %u\n", last_optitrack_frame_time);
    // printf("optitrack max time %u\n", optitrack_max_time);
    // printf("event max time %u\n", event_max_time);
    // printf("frame_max_time %u\n", frame_max_time);
    // printf("start_timestamp %u\n", start_timestamp);
    if (last_event_frame_time / 1000 != last_optitrack_frame_time / 1000) {
        printf("Something is probably wrong with the frames\n");
    }
}