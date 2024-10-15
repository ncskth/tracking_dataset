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
#include "camera_stuff.h"

#define CONVERTER_VERSION "1.0"

#define COLLECT_MEAN_AFTER 10000000

#define MEAN_FROM_OPTITRACK_DELTAS 500

#define EVENT_RANDOM_SAMPLING 1000
#define MEAN_FROM_EVENT_DELTAS (300 * 5)

#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720

#define FRAME_DELTA 1000

#define SAVE_FRAMES_AFTER 10000000
#define SKIP_ENDING 10000000

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


struct frame_key {
    uint32_t n;
    uint8_t p;
    uint16_t x;
    uint16_t y;
};

template <typename T>
T interpolate(double start_time, T start_value, double end_time, T end_value, double wanted_time) {
    return (wanted_time - start_time) / (end_time - start_time) * (end_value - start_value) + start_value;
}

std::map<int, std::vector<optitrack_object>> optitrack_data; //we should be able to fit many many hours in memory

std::array<std::array<std::pair<uint8_t, uint8_t>, FRAME_HEIGHT>, FRAME_WIDTH> event_frame;

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
        case BLOB0:
            return std::string("blob");
        case PLIER:
            return std::string("pliers");
        case HAMMER:
            return std::string("hammer");
        case SCREWDRIVER:
            return std::string("screwdriver");
        case WRENCH:
            return std::string("wrench");
        case SAW:
            return std::string("saw");

        default:
            printf("Invalid optitrack ID\n");
            assert(false);
            return std::string("lol wut");
    }
}

double quaternion_angle_distance(const Eigen::Quaterniond& q1, const Eigen::Quaterniond& q2) {
    Eigen::Quaterniond norm_q1 = q1.normalized();
    Eigen::Quaterniond norm_q2 = q2.normalized();

    // Compute the dot product
    double dot_product = norm_q1.dot(norm_q2);

    // Clamp the dot product to be within the valid range for acos
    dot_product = std::max(-1.0, std::min(1.0, dot_product));

    // Compute the angle
    double angle = 2.0 * std::acos(std::abs(dot_product));

    return angle;
}

Eigen::Quaternion<double> get_median_quaternion(std::vector<optitrack_object> arr, int index_to_get, int width) {
    if (index_to_get < width || index_to_get + width >= arr.size()) {
        auto a = arr[index_to_get];
        return {a.qw, a.qx, a.qy, a.qz};
    }

    std::vector<std::pair<int, int>> arr_indexed;
    auto a = arr[index_to_get];
    Eigen::Quaternion<double> q_a = { a.qw, a.qx, a.qy, a.qz};
    for (int i = index_to_get - width; i < index_to_get + width + 1; i++) {
        auto b = arr[i];
        Eigen::Quaternion<double> q_b = { b.qw, b.qx, b.qy, b.qz};
        double distance = quaternion_angle_distance(q_a, q_b);
        arr_indexed.push_back({distance, i});
    }
    std::sort(arr_indexed.begin(), arr_indexed.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        return a.first < b.first;
    });
    int median_i = arr_indexed[width].second;
    auto m = arr[median_i];
    Eigen::Quaternion<double> q_m = { m.qw, m.qx, m.qy, m.qz};
    return q_m;
}

Eigen::Vector3<double> get_median_position(std::vector<optitrack_object> arr, int index_to_get, int width) {
    if (index_to_get < width || index_to_get + width >= arr.size()) {
        auto a = arr[index_to_get];
        return {a.x, a.y, a.z};
    }

    std::vector<std::pair<int, int>> arr_indexed;
    auto a = arr[index_to_get];
    Eigen::Vector3<double> p_a = {a.x, a.y, a.z};
    for (int i = index_to_get - width; i < index_to_get + width + 1; i++) {
        auto b = arr[i];
        Eigen::Vector3<double> p_b = {b.x, b.y, b.z};
        double distance = (p_a - p_b).norm();
        arr_indexed.push_back({distance, i});
    }
    std::sort(arr_indexed.begin(), arr_indexed.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        return a.first < b.first;
    });
    int median_i = arr_indexed[width].second;
    auto m = arr[median_i];
    Eigen::Vector3<double> p_m = {m.x, m.y, m.z};
    return p_m;
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

    std::vector<struct frame_key> init_frame_keys;
    std::vector<uint8_t> init_frame_values;
    std::vector<uint32_t> frame_start_indexes;
    uint32_t current_frame_start_index = 0;

    h5pp::hid::h5t H5_KEY_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(frame_key));
    H5Tinsert(H5_KEY_TYPE, "n", HOFFSET(frame_key, n), H5T_NATIVE_UINT32);
    H5Tinsert(H5_KEY_TYPE, "p", HOFFSET(frame_key, p), H5T_NATIVE_UINT8);
    H5Tinsert(H5_KEY_TYPE, "x", HOFFSET(frame_key, x), H5T_NATIVE_UINT16);
    H5Tinsert(H5_KEY_TYPE, "y", HOFFSET(frame_key, y), H5T_NATIVE_UINT16);


    frame_file.setCompressionLevel(2);

    h5pp::Options options;
    options.dsetChunkDims = {10000};
    options.h5Type = H5_KEY_TYPE;
    options.h5Layout = H5D_CHUNKED;
    options.linkPath = "frame_keys";
    frame_file.writeDataset(init_frame_keys, options);
    frame_file.writeDataset(init_frame_values, "frame_values", H5D_CHUNKED, {0}, {10000});

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
            //ignore this for now...
        }
        else if (id == OPTITRACK_HEADER) {
            struct optitrack_header header;
            fread(&header, 1, sizeof(header), input_file);

            if (first_pc_timestamp = 0) {
                first_pc_timestamp = header.pc_t;
            }
            if (header.pc_t - first_pc_timestamp > COLLECT_MEAN_AFTER) {
                active = true;
            }

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

    std::sort(optitrack_deltas.begin(), optitrack_deltas.end());
    std::sort(event_deltas.begin(), event_deltas.end());

    int optitrack_correction = optitrack_deltas.at(optitrack_deltas.size() / 2);
    int event_correction = event_deltas.at(event_deltas.size() / 2);

    event_max_time = event_max_time - event_correction;
    optitrack_max_time = optitrack_max_time - optitrack_correction;
    uint32_t frame_max_time = std::min(event_max_time, optitrack_max_time) - SKIP_ENDING;

    fseek(input_file, 0, SEEK_SET);
    active = false;
    uint32_t start_timestamp = 0;
    first_pc_timestamp = 0;
    uint32_t first_event = 0;
    uint32_t last_event = 0;
    size_t event_count = 0;


    int aedat_header_offset = AEDAT4::save_header(event_output);
    uint32_t last_event_frame_time = 0;
    uint32_t first_event_frame_time = 0;
    uint32_t last_optitrack_frame_time = 0;
    uint32_t first_optitrack_frame_time = 0;
    uint32_t last_status_print = 0;
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
                if (t_adjusted < SAVE_FRAMES_AFTER - FRAME_DELTA) {
                    continue;
                }
                int index = entry.x + entry.y * FRAME_WIDTH;
                if (entry.p) {
                    event_frame[entry.x][entry.y].second++;
                } else {
                    event_frame[entry.x][entry.y].first++;
                }
                AEDAT::PolarityEvent formal_event =  {
                    .timestamp = t_adjusted,
                    .x = entry.x,
                    .y = entry.y,
                    .valid = true,
                    .polarity = entry.p ? true : false,
                };
                // std::cout << (entry.p ? "true" : "false") << (int) entry.p << std::endl;
                if (first_event == 0) {
                    first_event = t_adjusted;
                }
                event_count++;
                last_event = t_adjusted;
                events.push_back(formal_event);

                // if (rand() % 1000 == 0) {
                //     std::cout << "event" << t_adjusted / 1000 << std::endl;
                // }
                //optitrack interpolation misses the last frame
                if (t_adjusted >= frame_index * FRAME_DELTA + SAVE_FRAMES_AFTER
                && t_adjusted < frame_max_time - start_timestamp) {
                    if (first_event_frame_time == 0) {
                        first_event_frame_time = t_adjusted;
                    }
                    std::vector<struct frame_key> event_frame_keys;

                    std::vector<uint8_t> event_frame_values;

                    for (uint16_t x = 0; x < FRAME_WIDTH; x++) {
                        for (uint16_t y = 0; y < FRAME_HEIGHT; y++) {
                            if (event_frame[x][y].first > 0) {
                                struct frame_key key;
                                key.n = frame_index;
                                Eigen::Vector2<double> undistorted = undistort_pixel({x, y});
                                key.x = undistorted.x();
                                key.y = undistorted.y();
                                key.p = 0;

                                if (key.x >= 0 and key.x < WINDOW_WIDTH and key.y >= 0 and key.y < WINDOW_HEIGHT) {
                                    event_frame_keys.push_back(key);
                                    event_frame_values.push_back(event_frame[x][y].first);
                                }
                            }
                            if (event_frame[x][y].second > 0) {
                                struct frame_key key;
                                key.n = frame_index;
                                Eigen::Vector2<double> undistorted = undistort_pixel({x, y});
                                key.x = undistorted.x();
                                key.y = undistorted.y();
                                key.p = 1;
                                if (key.x >= 0 and key.x < WINDOW_WIDTH and key.y >= 0 and key.y < WINDOW_HEIGHT) {
                                    event_frame_keys.push_back(key);
                                    event_frame_values.push_back(event_frame[x][y].second);
                                }
                            }
                            event_frame[x][y] = {0, 0};
                        }
                    }
                    frame_start_indexes.push_back(current_frame_start_index);
                    frame_file.appendToDataset(event_frame_keys, "frame_keys", 0, {event_frame_keys.size()});
                    frame_file.appendToDataset(event_frame_values, "frame_values", 0, {event_frame_values.size()});
                    current_frame_start_index += event_frame_keys.size();
                    frame_index++;
                    last_event_frame_time = t_adjusted;
                    if (t_adjusted - last_status_print > 1000000) {
                        printf("Progress: %.2f%\n", ((float) t_adjusted - SAVE_FRAMES_AFTER) / (frame_max_time - start_timestamp - SKIP_ENDING) * 100);
                        last_status_print = t_adjusted;
                    }
                }
            }
            AEDAT4::save_events(event_output, events);
        }
        else if (id == TIMESTAMP_HEADER) {
            struct timestamp_header header;
            fread(&header, 1, sizeof(header), input_file);
            // skip this for now
        }
        else if (id == OPTITRACK_HEADER) {
            struct optitrack_header header;
            fread(&header, 1, sizeof(header), input_file);

            if (first_pc_timestamp == 0) {
                first_pc_timestamp = header.pc_t;
            }
            if (not active && header.pc_t - first_pc_timestamp > COLLECT_MEAN_AFTER) {
                start_timestamp = header.pc_t - COLLECT_MEAN_AFTER;
                active = true;
            }

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
    frame_file.writeDataset(frame_start_indexes, "frame_start_indexes");
    event_output.flush();
    event_output.close();

    std::map<std::string, std::vector<std::array<float, 9>>> ahrs_matrix;


    json optitrack_json;
    int optitrack_track_count = (*optitrack_data.begin()).second.size();
    uint32_t poopy;
    for (int row = 1; row < optitrack_track_count; row++) {
        Eigen::Vector3<double> future_camera_pos;
        Eigen::Quaternion<double> future_camera_q;
        Eigen::Vector3<double> old_camera_pos;
        Eigen::Quaternion<double> old_camera_q;

        for (auto column : optitrack_data) {
            if (column.first != CAMERA0) {
                continue;
            }
            future_camera_pos = get_median_position(column.second, row, 2);
            future_camera_q = get_median_quaternion(column.second, row, 2);
            old_camera_pos = get_median_position(column.second, row - 1, 2);
            old_camera_q = get_median_quaternion(column.second, row - 1, 2);
        }

        for (auto column : optitrack_data) {
            if (column.first == CAMERA0) {
                continue;
            }
            Eigen::Vector3<double> future_object_pos = get_median_position(column.second, row, 2);
            Eigen::Quaternion<double> future_object_q = get_median_quaternion(column.second, row, 2);
            int t_future = column.second[row].t;
            Eigen::Vector3<double> old_object_pos = get_median_position(column.second, row - 1, 2);
            Eigen::Quaternion<double> old_object_q = get_median_quaternion(column.second, row - 1, 2);
            int t_old = column.second[row - 1].t;
            std::string name = optitrack_id_to_name((enum optitrack_ids) column.first);

            for (int i = t_old / FRAME_DELTA; i < t_future / FRAME_DELTA; i++) {
                // if (i - poopy != 1) {
                    // printf("oh no %d %d\n", i);
                // }
                // poopy = i;
                int t_interp = i * FRAME_DELTA;
                Eigen::Vector3<double> interp_camera_pos = interpolate(t_old, old_camera_pos, t_future, future_camera_pos, t_interp);
                Eigen::Quaternion<double> interp_camera_q = {
                    interpolate(t_old, old_camera_q.w(), t_future, future_camera_q.w(), t_interp),
                    interpolate(t_old, old_camera_q.x(), t_future, future_camera_q.x(), t_interp),
                    interpolate(t_old, old_camera_q.y(), t_future, future_camera_q.y(), t_interp),
                    interpolate(t_old, old_camera_q.z(), t_future, future_camera_q.z(), t_interp),
                };
                interp_camera_q.normalize();

                Eigen::Vector3<double> interp_object_pos = interpolate(t_old, old_object_pos, t_future, future_object_pos, t_interp);
                Eigen::Quaternion<double> interp_object_q =  {
                    interpolate(t_old, old_object_q.w(), t_future, future_object_q.w(), t_interp),
                    interpolate(t_old, old_object_q.x(), t_future, future_object_q.x(), t_interp),
                    interpolate(t_old, old_object_q.y(), t_future, future_object_q.y(), t_interp),
                    interpolate(t_old, old_object_q.z(), t_future, future_object_q.z(), t_interp),
                };
                interp_object_q.normalize();

                Eigen::Vector3<double> object_relative_pos = interp_camera_q.inverse().normalized() * (interp_object_pos - interp_camera_pos);
                Eigen::Quaternion<double> object_relative_q = interp_camera_q.inverse().normalized() * interp_object_q;

                Eigen::Vector2<double> pixel = position_to_pixel(object_relative_pos);
                std::map<std::string, float> interp_map;
                interp_map["t"] = t_interp;
                interp_map["px"] = pixel.x();
                interp_map["py"] = pixel.y();
                interp_map["x"] = object_relative_pos[0];
                interp_map["y"] = object_relative_pos[1];
                interp_map["z"] = object_relative_pos[2];
                interp_map["qw"] = object_relative_q.w();
                interp_map["qx"] = object_relative_q.x();
                interp_map["qy"] = object_relative_q.y();
                interp_map["qz"] = object_relative_q.z();

                // interp_map["camera_qw"] = interp_camera_q.w();
                // interp_map["camera_qx"] = interp_camera_q.x();
                // interp_map["camera_qy"] = interp_camera_q.y();
                // interp_map["camera_qz"] = interp_camera_q.z();
                // interp_map["camera_x"] = interp_camera_pos.x();
                // interp_map["camera_y"] = interp_camera_pos.y();
                // interp_map["camera_z"] = interp_camera_pos.z();

                // interp_map["object_qw"] = interp_object_q.w();
                // interp_map["object_qx"] = interp_object_q.x();
                // interp_map["object_qy"] = interp_object_q.y();
                // interp_map["object_qz"] = interp_object_q.z();
                // interp_map["object_x"] = interp_object_pos.x();
                // interp_map["object_y"] = interp_object_pos.y();
                // interp_map["object_z"] = interp_object_pos.z();


                optitrack_json["data"][name].push_back(interp_map);

                if (i * FRAME_DELTA >= SAVE_FRAMES_AFTER && i * FRAME_DELTA < frame_max_time - start_timestamp) {
                    std::array<float, 9> ahrs {interp_map["px"], interp_map["py"], interp_map["x"], interp_map["y"], interp_map["z"], interp_map["qw"], interp_map["qx"], interp_map["qy"], interp_map["qz"]};
                    ahrs_matrix[name].push_back(ahrs);
                    last_optitrack_frame_time = i * FRAME_DELTA;

                    if (first_optitrack_frame_time == 0) {
                        first_optitrack_frame_time = i * FRAME_DELTA;
                    }
                }
            }
        }
    }
    object_output << optitrack_json;

    for (auto v : ahrs_matrix) {
        frame_file.writeDataset(v.second, v.first, {v.second.size(), 9});
    }


    printf("last_event_frame_time %u\n", last_event_frame_time);
    printf("last_optitrack_frame_time %u\n", last_optitrack_frame_time);
    printf("first_event_frame_time %u\n", first_event_frame_time);
    printf("first_optitrack_frame_time %u\n", first_optitrack_frame_time);


    printf("num event frames %u\n", frame_index);
    printf("num optitrack frames %u\n", (*ahrs_matrix.begin()).second.size());
    if (last_event_frame_time / 1000 != last_optitrack_frame_time / 1000) {
        printf("[error] End time is different\n");
    }
    if (first_event_frame_time / 1000 != first_optitrack_frame_time / 1000) {
        printf("[error] Start time is different\n");
    }

    if (frame_index != (*ahrs_matrix.begin()).second.size()) {
        printf("[error] inconsistent number of frames\n");
    }
}