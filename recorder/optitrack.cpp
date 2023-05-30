
#include <thread>

#include "libnatnet/NatNetTypes.h"
#include "libnatnet/NatNetCAPI.h"
#include "libnatnet/NatNetClient.h"

#include "optitrack.h"

static const ConnectionType kDefaultConnectionType = ConnectionType_Multicast;

NatNetClient* g_pClient = NULL;


void NATNET_CALLCONV DataHandler(sFrameOfMocapData* data, void* pUserData)
{
    NatNetClient* pClient = (NatNetClient*) pUserData;
    // timecode - for systems with an eSync and SMPTE timecode generator - decode to values
	int hour, minute, second, frame, subframe;
    NatNet_DecodeTimecode( data->Timecode, data->TimecodeSubframe, &hour, &minute, &second, &frame, &subframe );
}

void init_optitrack(struct flow_struct & data) {
    sNatNetClientConnectParams g_connectParams;

    // create NatNet client
    g_pClient = new NatNetClient();

    // set the frame callback handler
    g_pClient->SetFrameReceivedCallback( DataHandler, g_pClient );	// this function will receive data from the server

    g_connectParams.connectionType = kDefaultConnectionType;

    g_connectParams.serverAddress = "172.16.222.18";
    g_connectParams.localAddress = "172.16.222.31";

    g_pClient->Connect( g_connectParams );

    while (true) {
        std::this_thread::yield();
    }
}