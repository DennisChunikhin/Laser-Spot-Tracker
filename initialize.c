#include <stdio.h>
#include <pylonc/PylonC.h>
#include "initialize.h"

void printErrorAndExit( GENAPIC_RESULT errc ) {
    char *errMsg;
    size_t length;

    // Retrieve last error message
    GenApiGetLastErrorMessage( NULL, &length );
    errMsg = (char *) malloc(length);
    GenApiGetLastErrorMessage( errMsg, &length );

    fprintf(stderr, "%s\n", errMsg);

    free(errMsg);

    PylonTerminate();
    exit( EXIT_FAILURE );
}

void initializeDevice(PYLON_DEVICE_HANDLE hDev) {
    GENAPIC_RESULT res;
    
    // Open the device
    res = PylonDeviceOpen( hDev, PYLONC_ACCESS_MODE_CONTROL | PYLONC_ACCESS_MODE_STREAM );
    CHECK(res);

    /* DEVICE CONFIGURATIONS */
    
    // Set the pixel format to Mono12 if available
    if (PylonDeviceFeatureIsAvailable(hDev, "EnumEntry_PixelFormat_Mono12")) {
        res = PylonDeviceFeatureFromString(hDev, "PixelFormat", "Mono12");
        CHECK(res);
    } else if (PylonDeviceFeatureIsAvailable(hDev, "EnumEntry_PixelFormat_Mono8")) {
        fprintf(stderr, "Mono12 not available, using mono8.\n");
        res = PylonDeviceFeatureFromString(hDev, "PixelFormat", "Mono8");
        CHECK(res);
    }
    
    // Disable start trigger if available
    if (PylonDeviceFeatureIsAvailable(hDev, "EnumEntry_TriggerSelector_AcquisitionStart")) {
        res = PylonDeviceFeatureFromString(hDev, "TriggerSelector", "AcquisitionStart");
        CHECK(res);
        res = PylonDeviceFeatureFromString(hDev, "TriggerMode", "Off");
        CHECK(res);
    }
    
    // Disable frame burst start trigger if available
    if (PylonDeviceFeatureIsAvailable(hDev, "EnumEntry_TriggerSelector_FrameBurstStart")) {
        res = PylonDeviceFeatureFromString(hDev, "TriggerSelector", "FrameBurstStart");
        CHECK(res);
        res = PylonDeviceFeatureFromString(hDev, "TriggerMode", "Off");
        CHECK(res);
    }
    
    // Disable frame start trigger if available
    if (PylonDeviceFeatureIsAvailable(hDev, "EnumEntry_TriggerSelector_FrameStart")) {
        res = PylonDeviceFeatureFromString(hDev, "TriggerSelector", "FrameStart");
        CHECK(res);
        res = PylonDeviceFeatureFromString(hDev, "TriggerMode", "Off");
        CHECK(res);
    }
    
    // Set packet size
    if (PylonDeviceFeatureIsWritable(hDev, "GevSCPSPacketSize")) {
        res = PylonDeviceSetIntegerFeature(hDev, "GevSCPSPacketSize", PACKET_SIZE);
        CHECK(res);
    }
}

int getPayloadSize(PYLON_DEVICE_HANDLE hDev, int *payloadSize) {
    GENAPIC_RESULT res;
    
    PYLON_STREAMGRABBER_HANDLE hGrabber;
    res = PylonDeviceGetStreamGrabber(hDev, 0, &hGrabber);
    CHECK(res);
    res = PylonStreamGrabberOpen(hGrabber);
    CHECK(res);
    
    res = PylonStreamGrabberGetPayloadSize(hDev, hGrabber, payloadSize);
    CHECK(res);
    
    res = PylonStreamGrabberClose(hGrabber);
    CHECK(res);
    
    return *payloadSize;
}