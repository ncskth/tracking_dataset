#pragma once

#include <queue>
#include <atomic>
#include <mutex>
#include <stdint.h>

struct size_buf {
    uint8_t *buf;
    int size;
};

struct flow_struct {
    std::atomic<int> data_available;
    std::mutex queue_mutex;
    std::queue<struct size_buf> data_queue;
};