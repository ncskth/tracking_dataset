#include <stdlib.h>

#include <SDL2/SDL.h>
#include <thread>
#include <chrono>
#include <stdio.h>
#include <eigen3/Eigen/Geometry>
#include <iostream>
#include <math.h>

#include "protocol.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define PI 3.141525

#define CAMERA_VERTICAL_FOV (65.0 * DEG_TO_RAD)

#define CAMERA_HORIZONTAL_FOV (95.0 * DEG_TO_RAD)

#define RAD_TO_DEG (180.0/PI)
#define DEG_TO_RAD (PI/180.0)

#define RECTANGLE_WIDTH 0.1
#define RECTANGLE_HEIGHT 0.1

#define EVENT_DIVIDER_PER_MS 1

Eigen::Vector3<double> camera_position;
Eigen::Quaternion<double> camera_attitude;
Eigen::Vector3<double> object_position;
Eigen::Quaternion<double> object_attitude;
int object_id;
int camera_id;

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
    out[0] = (-1 * atan2(pos.x(), -pos.z())) / (CAMERA_HORIZONTAL_FOV / 2) * WINDOW_WIDTH + WINDOW_WIDTH / 2;
    out[1] = atan2(pos.y(), -pos.z()) / (CAMERA_VERTICAL_FOV / 2) * WINDOW_HEIGHT + WINDOW_HEIGHT / 2;
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
                if (0 == rand() % event_divider) {
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
                camera_position = {header.pos_x, header.pos_y, header.pos_z};
                camera_attitude = {header.q_x, header.q_y, header.q_z, header.q_w};
                camera_id = header.object_id;
            } else {
                object_position = {header.pos_x, header.pos_y, header.pos_z};
                object_attitude = {header.q_x, header.q_y, header.q_z, header.q_w};
                object_id = header.object_id;
            }
        }
        else {
            return -2;
        }

        if (current_timestamp - last_updated_timestamp > protocol_ms_per_frame * 1000) {
            if (first_timestamp == 0) {
                first_timestamp = current_timestamp;
            }
            camera_attitude = camera_attitude * camera_offset_q();

            // draw optitrack
            Eigen::Vector3<double> relative_position = (object_position - camera_position);
            relative_position = camera_attitude.inverse().normalized() * relative_position;

            Eigen::Quaternion<double> relative_attitude = object_attitude.normalized() * camera_attitude.normalized();
            Eigen::Vector3<double> relative_angle = relative_attitude.toRotationMatrix().eulerAngles(0, 1, 2);
            Eigen::Vector3<double> camera_angle = camera_attitude.toRotationMatrix().eulerAngles(0, 1, 2);

            Eigen::Vector2<double> center = position_to_pixel(relative_position);
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            const int cross_size = 20;
            SDL_RenderDrawLine(renderer, center.x() - cross_size, center.y(), center.x() + cross_size, center.y());
            SDL_RenderDrawLine(renderer, center.x(), center.y() - cross_size, center.x(), center.y() + cross_size);
            if (object_id == RECTANGLE0) {
                Eigen::Vector3<double> tl3 = {-RECTANGLE_WIDTH/2.0, 0, -RECTANGLE_HEIGHT/2.0};
                Eigen::Vector3<double> tr3 = {RECTANGLE_WIDTH/2.0, 0, -RECTANGLE_HEIGHT/2.0};
                Eigen::Vector3<double> bl3 = {-RECTANGLE_WIDTH/2.0, 0, RECTANGLE_HEIGHT/2.0};
                Eigen::Vector3<double> br3 = {RECTANGLE_WIDTH/2.0, 0, RECTANGLE_HEIGHT/2.0};

                object_attitude = object_attitude * Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ());

                tl3 = object_attitude * tl3;
                tr3 = object_attitude * tr3;
                bl3 = object_attitude * bl3;
                br3 = object_attitude * br3;

                tl3 += object_position - camera_position;
                tr3 += object_position - camera_position;
                bl3 += object_position - camera_position;
                br3 += object_position - camera_position;

                tl3 = camera_attitude.inverse().normalized() * tl3;
                tr3 = camera_attitude.inverse().normalized() * tr3;
                bl3 = camera_attitude.inverse().normalized() * bl3;
                br3 = camera_attitude.inverse().normalized() * br3;

                printf("center %f %f %f\n", relative_position.x(), relative_position.y(), relative_position.z());
                printf("top left %f %f %f\n", tl3.x(), tl3.y(), tl3.z());
                printf("top right %f %f %f\n", tr3.x(), tr3.y(), tr3.z());
                printf("bot left %f %f %f\n", bl3.x(), bl3.y(), bl3.z());
                printf("bot right %f %f %f\n", br3.x(), br3.y(), br3.z());

                Eigen::Vector2<double> tl = position_to_pixel(tl3);
                Eigen::Vector2<double> tr = position_to_pixel(tr3);
                Eigen::Vector2<double> bl = position_to_pixel(bl3);
                Eigen::Vector2<double> br = position_to_pixel(br3);

                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                SDL_RenderDrawLine(renderer, tl.x(), tl.y(), tr.x(), tr.y());
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
                SDL_RenderDrawLine(renderer, tr.x(), tr.y(), br.x(), br.y());
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                SDL_RenderDrawLine(renderer, br.x(), br.y(), bl.x(), bl.y());
                SDL_RenderDrawLine(renderer, bl.x(), bl.y(), tl.x(), tl.y());

                printf("top left %f %f\n", tl.x(), tl.y());
                printf("top right %f %f\n", tr.x(), tr.y());
                printf("bot left %f %f\n", bl.x(), bl.y());
                printf("bot right %f %f\n", br.x(), br.y());
            }


            // present frame
            SDL_RenderPresent(renderer);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);


            printf("frame update %fs\n", (current_timestamp - first_timestamp) / 1000000.0);
            printf("relative position %f %f %f\n", relative_position.x(), relative_position.y(), relative_position.z());
            printf("center pixel %f %f\n", center.x(), center.y());


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
