#include <stdlib.h>

#include <SDL2/SDL.h>
#include <thread>
#include <chrono>
#include <stdio.h>

#include "protocol.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define EVENT_DIVIDER_PER_MS 1

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
            // current_timestamp = header.pc_t;
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
            // current_timestamp = header.pc_t;
        }
        else if (id == OPTITRACK_HEADER) {
            struct optitrack_header header;
            fread(&header, 1, sizeof(header), f);
            // current_timestamp = header.pc_t;
        }
        else {
            return -2;
        }

        if (current_timestamp - last_updated_timestamp > protocol_ms_per_frame * 1000) {
            if (first_timestamp == 0) {
                first_timestamp = current_timestamp;
            }
            printf("frame update %fs\n", (current_timestamp - first_timestamp) / 1000000.0);
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
