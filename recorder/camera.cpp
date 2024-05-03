#include <thread>
#include <chrono>
#include <metavision/sdk/driver/camera.h>
#include <metavision/sdk/base/events/event_cd.h>
#include <string.h>
#include <iomanip>
#include "camera.h"
#include "protocol.h"

uint32_t num_events;
uint8_t camera_frame[1280*720];

void init_camera(struct flow_struct & flow) {
    Metavision::Camera cam; // create the camera
    cam = Metavision::Camera::from_first_available();
    // cam = Metavision::Camera::from_serial("00050948");
    auto &biases = cam.biases();
    auto facility = biases.get_facility();
    for (auto k : {"bias_diff", "bias_diff_on", "bias_diff_off", "bias_fo",
                    "bias_hpf", "bias_refr"}) {
        std::cout << "Bias " << std::setw(20) << k << ": " << facility->get(k)
                << std::endl;
    }

    try {
        // OLD
        // facility->set("bias_diff_on", 120);
        // facility->set("bias_diff_off", 45);
        // facility->set("bias_fo", 45);
        // facility->set("bias_hpf", 0);
        // facility->set("bias_refr", 40);

        facility->set("bias_diff_on", 37);
        facility->set("bias_diff_off", 37);
        facility->set("bias_fo", 45);
        facility->set("bias_hpf", 0);
        facility->set("bias_refr", 40);

    } catch (std::runtime_error e) {
        std::cout << e.what() << std::endl;
    }
    cam.cd().add_callback([&flow](const Metavision::EventCD *begin, const Metavision::EventCD *end) {
        struct size_buf data;
        int offset = 0;
        int i = 0;
        data.buf = new uint8_t[sizeof(struct camera_event) * 500];
        data.buf[0] = CAMERA_HEADER;
        offset += 1;
        offset += sizeof(struct camera_header);
        struct timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);
        uint32_t pc_t = (time.tv_sec * 1000000000 + time.tv_nsec) / 1000;
        for (const Metavision::EventCD *ev = begin; ev != end; ++ev) {
            camera_frame[ev->x + ev->y * 1280]++;
            struct camera_event entry;
            entry.p = ev->p;
            entry.x = ev->x;
            entry.y = ev->y;
            entry.t = ev->t;
            memcpy(data.buf + offset, &entry, sizeof(entry));
            offset += sizeof(entry);
            i++;
        }
        num_events += i;

        struct camera_header header;
        header.num_events = i;
        header.camera_id = 0;
        header.pc_t = pc_t;
        memcpy(data.buf + 1, &header, sizeof(header));
        data.size = offset;
        std::unique_lock lock{flow.queue_mutex, std::defer_lock};
        lock.lock();
        flow.data_queue.push(data);
        lock.unlock();
        flow.data_available++;
    });

    printf("resolution: %d %d\n", cam.geometry().width(), cam.geometry().height());
    cam.start();
    printf("camera started\n");
    while (true) {
        if (flow.stop) {
            cam.stop();
            return;
        }
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000s);
    }
}
