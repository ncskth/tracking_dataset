
#include <thread>
#include <time.h>
#include <stdio.h>

#include "libnatnet/NatNetTypes.h"
#include "libnatnet/NatNetCAPI.h"
#include "libnatnet/NatNetClient.h"

#include "optitrack.h"
#include "protocol.h"

static const ConnectionType kDefaultConnectionType = ConnectionType_Multicast;

NatNetClient* g_pClient = NULL;

double optitrack_start_time = 0;
std::map<int, tracked_object> tracked_objects;

void NATNET_CALLCONV DataHandler(sFrameOfMocapData* track, void* pUserData)
{
    struct flow_struct *flow = (struct flow_struct *) pUserData;
    double timestamp = track->fTimestamp;
    if (optitrack_start_time == 0) {
        optitrack_start_time = timestamp;
    }
    timestamp -= optitrack_start_time;

    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    uint32_t pc_t = (time.tv_sec * 1000000000 + time.tv_nsec) / 1000;
    // fprintf(stdout, "optitrack timetamp%f\n", timestamp);
    struct size_buf data;
    data.buf = new uint8_t[track->nRigidBodies * (sizeof(struct optitrack_header) + 1)];
    int offset = 0;
    for(int i = 0; i < track->nRigidBodies; i++) {
        struct optitrack_header entry;
        entry.pc_t = pc_t;
        entry.t = timestamp * 1000000;
        entry.params = track->RigidBodies[i].params;
        entry.object_id = track->RigidBodies[i].ID;
        entry.error = track->RigidBodies[i].MeanError;
        entry.pos_x = track->RigidBodies[i].x;
        entry.pos_y = track->RigidBodies[i].y;
        entry.pos_z = track->RigidBodies[i].z;
        entry.q_w = track->RigidBodies[i].qw;
        entry.q_x = track->RigidBodies[i].qx;
        entry.q_y = track->RigidBodies[i].qy;
        entry.q_z = track->RigidBodies[i].qz;
        data.buf[offset] = OPTITRACK_HEADER;
        offset += 1;
        memcpy(data.buf + offset, &entry, sizeof(entry));
        offset += sizeof(entry);

        struct tracked_object object;
        object.attitude = {track->RigidBodies[i].qw, track->RigidBodies[i].qx, track->RigidBodies[i].qy, track->RigidBodies[i].qz};
        object.position = {track->RigidBodies[i].x, track->RigidBodies[i].y, track->RigidBodies[i].z};
        object.id = track->RigidBodies[i].ID;
        tracked_objects[object.id] = object;
	}
    data.size = offset;
    std::unique_lock lock{flow->queue_mutex, std::defer_lock};
    lock.lock();
    flow->data_queue.push(data);
    lock.unlock();
    flow->data_available++;
}

void init_optitrack(struct flow_struct & data) {
    sNatNetClientConnectParams g_connectParams;

    // create NatNet client
    g_pClient = new NatNetClient();

    // set the frame callback handler
    g_pClient->SetFrameReceivedCallback( DataHandler, &data );	// this function will receive data from the server

    g_connectParams.connectionType = kDefaultConnectionType;

    g_connectParams.serverAddress = "172.16.222.18"; // optitrack server
    g_connectParams.localAddress = "172.16.223.224"; // local ip
    printf("Optitrack connecting...\n");
    int e;
    e = g_pClient->Connect( g_connectParams );
    if (e) {
        printf("Optitrack could not connect. (the IP address is hardcoded for now. please check in optitrack.cpp) Code: %d\n", e);
    } else {
        printf("Optitrack connected\n");
    }

    while (true) {
        using namespace std::chrono_literals;
        if (data.stop) {
            return;
        }
        std::this_thread::sleep_for(1000s);
    }
}
