#include <thread>
#include <queue>
#include <memory>
#include <vector>
#include <stdint.h>
#include <semaphore>
#include <mutex>
#include <queue>
#include <atomic>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <chrono>
#include <string.h>

#include <iomanip>
#include <ctime>

#include "camera.h"
#include "optitrack.h"
#include "flow_struct.h"
#include "protocol.h"
#include "recorder_visualizer.h"

#define MAX(a, b) a > b ? a : b

std::atomic<int> max_queue_size = 0;


uint32_t initial_timestamp = 0;
uint32_t global_timer = 0;
void init_timestamper(flow_struct & flow) {
    struct timespec time;
    struct timestamp_header entry;
    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &time);
        uint32_t t = (time.tv_sec * 1000000000 + time.tv_nsec) / 1000;
        if (initial_timestamp == 0) {
            initial_timestamp = t;
        }
        t -= initial_timestamp;
        entry.pc_t = t;
        global_timer = t;
        struct size_buf data;
        data.size = sizeof(entry) + 1;
        data.buf = new uint8_t[data.size];
        data.buf[0] = TIMESTAMP_HEADER;
        memcpy(data.buf + 1, &entry, sizeof(entry));

        std::unique_lock lock{flow.queue_mutex, std::defer_lock};
        lock.lock();
        flow.data_queue.push(data);
        lock.unlock();
        flow.data_available++;
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
}

void init_printer(flow_struct & flow) {
    while (true) {
        std::cout << "queue size: " << max_queue_size << "\n";
        std::cout << "events: " << num_events / 1e6 << "Me\n";
        std::cout << "time: " << global_timer / 1e6 << "s\n";
        std::cout << "start: " << initial_timestamp << "s\n";
        std::cout << "tracked objects: " << tracked_objects.size() << "\n";
        num_events = 0;
        max_queue_size = 0;
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000ms);

    }
}

// main loop
int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 1) {
        printf("too many arguments\n");
        return -1;
    }
    struct flow_struct flow;
    flow.data_available = 0;

    std::string out_path;
    if (argc == 2) {
        std::string arg(argv[1]);
        std::ostringstream oss;
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        oss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");
        std::string date = oss.str();
        out_path = date + "_" + std::string(argv[1]);
    }

    if (argc == 1) {
        out_path = "/dev/null";
    }
    FILE *f = fopen(out_path.c_str(), "w");

    std::thread camera_t(init_camera, std::ref(flow));
    std::thread optitrack_t(init_optitrack, std::ref(flow));
    std::thread timestamper_t(init_timestamper, std::ref(flow));
    std::thread printer_t(init_printer, std::ref(flow));
    std::thread drawer_t(init_drawer, std::ref(flow));

    uint8_t write_buf[BUFSIZ];
    int write_index = 0;
    while(true){
        while (flow.data_available) {
            max_queue_size = MAX( (int) flow.data_available, (int) max_queue_size);
            std::unique_lock lock{flow.queue_mutex, std::defer_lock};
            lock.lock();
            struct size_buf data = flow.data_queue.front();
            flow.data_queue.pop();
            lock.unlock();

            if (sizeof(write_buf) - write_index >= data.size) {
                memcpy(write_buf + write_index, data.buf, data.size);
                write_index += data.size;
            } else {
                int to_write = sizeof(write_buf) - write_index;
                memcpy(write_buf + write_index, data.buf, to_write);
                int e;
                e = fwrite(write_buf, 1, sizeof(write_buf), f);
                if (e != sizeof(write_buf)) {
                    return -2;
                }
                write_index = data.size - to_write;
                memcpy(write_buf, data.buf + to_write, data.size - to_write);
            }
            flow.data_available--;
            delete[] data.buf;
        }
    };
}
