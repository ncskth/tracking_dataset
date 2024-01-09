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
#include <SDL2/SDL_ttf.h>

#include "recorder_visualizer.h"

#include "protocol.h"
#include "optitrack.h"
#include "camera.h"
#include "camera_stuff.h"



auto start = std::chrono::high_resolution_clock::now();

void init_drawer(struct flow_struct & data) {
    populate_id_to_polygons();

    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

    TTF_Init();
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 100);

    while (true) {
        // draw frame
        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
        for (uint64_t i = 0; i < 1280*720; i++) {
            if (camera_frame[i] > 0) {
                int x = i % 1280;
                int y = i / 1280;
                auto undistored_pixel = undistort_pixel({x, y});
                SDL_RenderDrawPoint(renderer, undistored_pixel.x(), undistored_pixel.y());
            }
        }
        memset(camera_frame, 0, sizeof(camera_frame));



        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start);
        char buf[128];
        snprintf(buf, sizeof(buf), "%d", duration.count());

        SDL_Rect destRect;
        destRect.x = WINDOW_WIDTH * 3 / 4;  // X-coordinate
        destRect.y = 25;  // Y-coordinate

        SDL_Surface* surface = TTF_RenderText_Solid(font, buf, {255, 255, 255, 255});
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_QueryTexture(textTexture, NULL, NULL, &destRect.w, &destRect.h);
        SDL_RenderCopy(renderer, textTexture, NULL, &destRect);
        SDL_FreeSurface(surface);

        // draw optitrack
        struct tracked_object camera_object;
        for (auto object : tracked_objects) {
            if (object.second.id == CAMERA0) {
                camera_object = object.second;
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 64);
        SDL_RenderDrawLine(renderer, 1280 / 2, 0, 1280 / 2, 720);
        SDL_RenderDrawLine(renderer, 0, 720 / 2, 1280, 720 / 2);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        for (auto object : tracked_objects) {
            if (object.second.id < 10) {
                continue;
            }
            //draw cross
            Eigen::Vector3<double> relative_position = (object.second.position - camera_object.position);
            relative_position = camera_object.attitude.inverse().normalized() * relative_position;
            Eigen::Vector2<double> center = position_to_pixel(relative_position);
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
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


        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_q) {
                        data.stop = true;
                    }
                    break;

                case SDL_QUIT:
                    data.stop = true;
                    break;
            }
        }

        if (data.stop) {
            break;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}