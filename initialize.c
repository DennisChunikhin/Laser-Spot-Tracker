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
    
    GenApiGetLastErrorDetail( NULL, &length );
    errMsg = (char *) malloc(length);
    GenApiGetLastErrorDetail( errMsg, &length );
    
    fprintf(stderr, "%s\n", errMsg);
    
    free(errMsg);

    PylonTerminate();
    exit( EXIT_FAILURE );
}

// Get first device and return the device name
char *getFirstDevice(PYLON_DEVICE_HANDLE *hDev) {
    GENAPIC_RESULT res;
    
    // Number of devices
    int numDevices;
    
    res = PylonEnumerateDevices(&numDevices);
    CHECK(res);
    if (numDevices == 0) {
        fprintf( stderr, "No devices found!\n" );

        PylonTerminate();
        exit( EXIT_FAILURE );
    }
    
    // Get first device
    res = PylonCreateDeviceByIndex( 0, hDev );
    CHECK(res);
    
    // Open the device
    res = PylonDeviceOpen( *hDev, PYLONC_ACCESS_MODE_CONTROL | PYLONC_ACCESS_MODE_STREAM );
    CHECK(res);
    
    // Get the name of the device
    char *buf;
    if (PylonDeviceFeatureIsReadable(*hDev, "DeviceModelName")) {
        size_t siz;
        
        // Allocate memory for name string
        res = PylonDeviceFeatureToString(*hDev, "DeviceModelName", NULL, &siz);
        CHECK(res);
        buf = (char *)malloc(sizeof(char)*siz);
        if (buf == NULL) {
            fprintf(stderr, "Out of memory!\n");
            PylonTerminate();
            exit(EXIT_FAILURE);
        }
        
        // Get name string
        res = PylonDeviceFeatureToString(*hDev, "DeviceModelName", buf, &siz);
        CHECK(res);
    } else {
        // Empty name string
        buf = (char *)malloc(sizeof(char));
        if (buf == NULL) {
            fprintf(stderr, "Out of memory!\n");
            PylonTerminate();
            exit(EXIT_FAILURE);
        }
        buf[0] = '\0';
    }
    
    return buf;
}

void initializeDevice(PYLON_DEVICE_HANDLE hDev, int64_t *sizeX, int64_t *sizeY) {
    GENAPIC_RESULT res;
    
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
    
    // Use continuous frame acquisition mode
    res = PylonDeviceFeatureFromString(hDev, "AcquisitionMode", "Continuous");
    CHECK(res);
    
    // Temp: Set minimum exposure time
    /*if (PylonDeviceFeatureIsWritable(hDev, "ExposureTimeRaw")) {
        int64_t expTime;
        res = PylonDeviceGetIntegerFeatureMin(hDev, "ExposureTimeRaw", &expTime);
        CHECK(res);
        printf("Exposure time: %lld us\n", expTime);
        res = PylonDeviceSetIntegerFeature(hDev, "ExposureTimeRaw", expTime);
    }*/
    
    // Get camera width and height
    if (PylonDeviceFeatureIsReadable(hDev, "Width")) {
        res = PylonDeviceGetIntegerFeatureMax(hDev, "Width", sizeX);
        CHECK(res);
        if (PylonDeviceFeatureIsWritable(hDev, "Width")) {
            res = PylonDeviceSetIntegerFeature(hDev, "Width", *sizeX);
            CHECK(res);
        }
    } else *sizeX = 0;
    if (PylonDeviceFeatureIsReadable(hDev, "Height")) {
        res = PylonDeviceGetIntegerFeatureMax(hDev, "Height", sizeY);
        CHECK(res);
        if (PylonDeviceFeatureIsWritable(hDev, "Width")) {
            res = PylonDeviceSetIntegerFeature(hDev, "Height", *sizeY);
            CHECK(res);
        }
    } else *sizeY = 0;
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