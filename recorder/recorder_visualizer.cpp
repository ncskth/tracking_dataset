#include <stdlib.h>

#include <SDL2/SDL.h>
#include <thread>
#include <chrono>
#include <stdio.h>
#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/Core>
#include <iostream>
#include <math.h>
#include <map>

#include "recorder_visualizer.h"

#include "protocol.h"
#include "optitrack.h"
#include "camera.h"
#include "camera_stuff.h"

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
    const double triangle_width = 0.29;
    const double triangle_height =  triangle_width / 2 * tan(60 * DEG_TO_RAD);
    id_to_polygon[TRIANGLE0] = {
        {-triangle_height / 2, 0, 0},
        {triangle_height / 2, 0, -triangle_width / 2},
        {triangle_height / 2, 0, triangle_width / 2},
        {-triangle_height / 2, 0, 0}
    };

    // plier
    id_to_polygon[PLIER0] = {
        {-0.025, 0, -0.15},
        {0, 0, 0},
        {0, 0, 0.045},
        {0, 0, 0},
        {0.025, 0, -0.15}
    };

    // hammer
    id_to_polygon[HAMMER0] = {
        {0, 0, -0.10},
        {0, 0, 0.155},
        {0.04, 0, 0.12},
        {0, 0, 0.155},
        {-0.04, 0, 0.155},
    };
    id_to_polygon[HAMMER1] = {
        {0, 0, -0.10},
        {0, 0, 0.155},
        {0.04, 0, 0.12},
        {0, 0, 0.155},
        {-0.04, 0, 0.155},
    };
    id_to_polygon[HAMMER_NEW] = {
        {0, 0, -0.24},
        {0, 0, 0.015},
        {0.05, 0, -0.03},
        {0, 0, 0.015},
        {-0.05, 0, 0.015},
    };
    id_to_polygon[HAMMER_NEW_NEW] = {
        {0, 0, -0.24},
        {0, 0, 0.015},
        {0.05, 0, -0.03},
        {0, 0, 0.015},
        {-0.05, 0, 0.015},
    };

    id_to_polygon[OLD_RECT] = {
        {-rectangle_width / 2.0, 0, -rectengle_height / 2.0},
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

void init_drawer(struct flow_struct & data) {
    populate_id_to_polygons();

    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

    while (true) {
        // draw frame
        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
        for (uint64_t i = 0; i < 1280*720; i++) {
            if (camera_frame[i] > 0) {
                SDL_RenderDrawPoint(renderer, i % 1280, i / 1280);
            }
        }
        memset(camera_frame, 0, sizeof(camera_frame));

        // draw optitrack
        struct tracked_object camera_object;
        for (auto object : tracked_objects) {
            if (object.second.id == CAMERA0) {
                camera_object = object.second;
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawLine(renderer, 1280 / 2, 0, 1280 / 2, 720);
        SDL_RenderDrawLine(renderer, 0, 720 / 2, 1280, 720 / 2);


        for (auto object : tracked_objects) {
            if (object.second.id < 10) {
                continue;
            }
            //draw cross
            Eigen::Vector3<double> relative_position = (object.second.position - camera_object.position);
            relative_position = camera_object.attitude.inverse() * relative_position;
            Eigen::Vector2<double> center = position_to_pixel(relative_position);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
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
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
            break;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}