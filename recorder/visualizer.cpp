#include <stdlib.h>

#include <SDL2/SDL.h>
#include <thread>
#include <chrono>
#include <stdio.h>
#include <eigen3/Eigen/Geometry>
#include <iostream>
#include <math.h>
#include <map>

#include "protocol.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define PI 3.141525

#define CAMERA_VERTICAL_FOV (80.0 * DEG_TO_RAD)

#define CAMERA_HORIZONTAL_FOV (145.0 * DEG_TO_RAD)

#define RAD_TO_DEG (180.0/PI)
#define DEG_TO_RAD (PI/180.0)

#define EVENT_DIVIDER_PER_MS 0.25


struct tracked_object {
    int id;
    Eigen::Vector3<double> position;
    Eigen::Quaternion<double> attitude;
};

tracked_object camera_object;

std::map<int, tracked_object> tracked_objects;

std::map<int, std::vector<Eigen::Vector3<double>>> id_to_polygon;


void populate_id_to_polygons() {
    // rectangle
    const double rectangle_width = 0.4;
    const double rectengle_height = 0.3;
    id_to_polygon[RECTANGLE0] = {
        {-rectangle_width / 2.0, 0, -rectengle_height / 2.0},
        {rectangle_width / 2.0, 0, -rectengle_height / 2.0},
        {rectangle_width / 2.0, 0, rectengle_height / 2.0},
        {-rectangle_width / 2.0, 0, rectengle_height / 2.0},
        {-rectangle_width / 2.0, 0, -rectengle_height / 2.0},
    };

    id_to_polygon[CHECKERBOARD] = {
        {-rectengle_height / 2.0, 0, -rectangle_width / 2.0},
        {rectengle_height / 2.0, 0, -rectangle_width / 2.0},
        {rectengle_height / 2.0, 0, rectangle_width / 2.0},
        {-rectengle_height / 2.0, 0, rectangle_width / 2.0},
        {-rectengle_height / 2.0, 0, -rectangle_width / 2.0},
    };

    // square
    const double square_width = 0.29;
    id_to_polygon[SQUARE0] = {
        {-square_width / 2.0, 0, -square_width / 2.0},
        {square_width / 2.0, 0, -square_width / 2.0},
        {square_width / 2.0, 0, square_width / 2.0},
        {-square_width / 2.0, 0, square_width / 2.0},
        {-square_width / 2.0, 0, -square_width / 2.0},
    };

    // triangle
    const double triangle_line_length = 0.29;
    id_to_polygon[TRIANGLE0] = {
        {-triangle_line_length / 2, 0, 0},
        {triangle_line_length / 2, 0, triangle_line_length / 2},
        {triangle_line_length / 2, 0, -triangle_line_length / 2},
        {-triangle_line_length / 2, 0, 0}
    };

    // circle
    const double circle_radius = 0.29/2;
    int iterations = 16;
    for (int i = 0; i <= iterations; i++) {
        id_to_polygon[CIRCLE0].push_back({
            sin(2 * PI * i / iterations) * circle_radius,
            0,
            cos(2 * PI * i / iterations) * circle_radius,
        });
    }
}

Eigen::Quaternion<double> camera_offset_q(void) {
    Eigen::AngleAxisd xAngle(0, Eigen::Vector3d::UnitX());
    Eigen::AngleAxisd yAngle(0, Eigen::Vector3d::UnitY());
    Eigen::AngleAxisd zAngle(0, Eigen::Vector3d::UnitZ());

    return xAngle * yAngle * zAngle;
}

Eigen::Quaternion<double> rectangle_offset_q(void) {
    Eigen::AngleAxisd xAngle(0, Eigen::Vector3d::UnitX());
    Eigen::AngleAxisd yAngle(0, Eigen::Vector3d::UnitY());
    Eigen::AngleAxisd zAngle(0, Eigen::Vector3d::UnitZ());

    return xAngle * yAngle * zAngle;
}

Eigen::Vector2<double> position_to_pixel(Eigen::Vector3<double> pos) {
    Eigen::Vector2<double> out;
    out[0] = atan2(pos.x(), -pos.z()) / (CAMERA_HORIZONTAL_FOV / 2) * WINDOW_WIDTH + WINDOW_WIDTH / 2;
    out[1] = -atan2(pos.y(), -pos.z()) / (CAMERA_VERTICAL_FOV / 2) * WINDOW_HEIGHT + WINDOW_HEIGHT / 2;
    return out;
}

int main(int argc, char **argv) {
    char* file_path;
    int real_ms_per_frame;
    int protocol_ms_per_frame;
    int start_at = 0;

    if (argc == 4) {
        file_path = argv[1];
        real_ms_per_frame = atoi(argv[2]);
        protocol_ms_per_frame = atoi(argv[3]);
    }
    else if (argc == 5) {
        file_path = argv[1];
        real_ms_per_frame = atoi(argv[2]);
        protocol_ms_per_frame = atoi(argv[3]);
        start_at = atoi(argv[4]);
    }
    else {
        printf("usage: [in file] [frame ms] [data ms] [optional: start at]\n");
        return -1;
    }

    populate_id_to_polygons();

    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

    FILE *f = fopen(file_path, "r");
    uint32_t current_timestamp = 0;
    uint32_t last_updated_timestamp = 0;
    uint64_t before_render = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;;
    uint64_t event_divider = EVENT_DIVIDER_PER_MS * protocol_ms_per_frame;
    uint64_t first_timestamp = 0;
    while (!feof(f)) {
        int id = fgetc(f);
        if (id == CAMERA_HEADER) {
            struct camera_header header;
            fread(&header, 1, sizeof(header), f);
            for (int i = 0; i < header.num_events; i++) {
                struct camera_event entry;
                current_timestamp = entry.t;
                fread(&entry, 1, sizeof(entry), f);
                if (0 == rand() % (event_divider + 1)) {
                    if (entry.p) {
                        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                    } else {
                        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    }
                    SDL_RenderDrawPoint(renderer, entry.x, entry.y);
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
            // printf("id %d\n", header.object_id);
            if (header.object_id  < 10) {
                camera_object.position = {header.pos_x, header.pos_y, header.pos_z};
                camera_object.attitude = Eigen::Quaternion<double>{header.q_w, header.q_x, header.q_y, header.q_z}.normalized();
                camera_object.id = header.object_id;
            } else {
                struct tracked_object object;
                object.position = {header.pos_x, header.pos_y, header.pos_z};
                object.attitude = Eigen::Quaternion<double>{header.q_w, header.q_x, header.q_y, header.q_z}.normalized();
                object.id = header.object_id;
                tracked_objects[object.id] = object;
            }
        }
        else {
            return -2;
        }

        if (current_timestamp - last_updated_timestamp > protocol_ms_per_frame * 1000) {
            if (first_timestamp == 0) {
                first_timestamp = current_timestamp;
            }

            // draw optitrack
            for (auto object : tracked_objects) {
                //draw cross
                Eigen::Vector3<double> relative_position = (object.second.position - camera_object.position);
                relative_position = camera_object.attitude.inverse() * relative_position;
                Eigen::Vector2<double> center = position_to_pixel(relative_position);
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                const int cross_size = 20;
                SDL_RenderDrawLine(renderer, center.x() - cross_size, center.y(), center.x() + cross_size, center.y());
                SDL_RenderDrawLine(renderer, center.x(), center.y() - cross_size, center.x(), center.y() + cross_size);


                //render 3d
                std::vector<Eigen::Vector3<double>> points = id_to_polygon.at(object.first);
                bool first = true;
                Eigen::Vector2<double> prev_pixel;
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); //set color for first
                for (auto point : points) {
                    Eigen::Vector3<double> current_point = point;
                    Eigen::Vector2<double> current_pixel;
                    current_point = object.second.attitude * current_point;
                    current_point += object.second.position - camera_object.position;
                    current_point = camera_object.attitude.inverse().normalized() * current_point;
                    current_pixel = position_to_pixel(current_point);
                    if (first) {
                        first = false;
                        prev_pixel = current_pixel;
                        continue;
                    }
                    SDL_RenderDrawLine(renderer, prev_pixel.x(), prev_pixel.y(), current_pixel.x(), current_pixel.y());
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // set color after so it's different for the first line
                    prev_pixel = current_pixel;
                }
            }

            // present frame
            SDL_RenderPresent(renderer);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);

            last_updated_timestamp = current_timestamp;
            uint64_t now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
            if (current_timestamp > first_timestamp + start_at * 1000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(real_ms_per_frame - (now - before_render)));
            }
            before_render = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        }

        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
            break;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
