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

#include "camera.h"
#include "optitrack.h"
#include "flow_struct.h"
#include "protocol.h"

void init_timestamper(flow_struct & flow) {
    struct timespec time;
    struct timestamp_header entry;
    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &time);
        uint32_t t = (time.tv_sec * 1000000000 + time.tv_nsec) / 1000;
        if (t - entry.t >= 10) {
            entry.t = t;
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
        }
        std::this_thread::yield();
    }
}

void init_printer(flow_struct & flow) {
    while (true) {
        printf("queue size: %d\n", flow.data_queue.size());
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000ms);
    }
}

// main loop
int main(int argc, char *argv[]) {
    if (argc != 2) {
        return -1;
    }
    struct flow_struct flow;
    flow.data_available = 0;
    FILE *f = fopen(argv[1], "w");

    std::thread camera_t(init_camera, std::ref(flow));
    std::thread optitrack_t(init_optitrack, std::ref(flow));
    std::thread timestamper_t(init_timestamper, std::ref(flow));
    std::thread printer_t(init_printer, std::ref(flow));

    while(true){
        while (flow.data_available) {
            std::unique_lock lock{flow.queue_mutex, std::defer_lock};
            lock.lock();
            struct size_buf data = flow.data_queue.front();
            flow.data_queue.pop();
            lock.unlock();
            if (fwrite(data.buf, 1, data.size, f) != data.size) {
                return -2;
            };
            flow.data_available--;
            delete[] data.buf;
        }
    };
}
