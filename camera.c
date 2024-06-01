#include <stdio.h>
#include <malloc.h>
#include <pylonc/PylonC.h>
#include "initialize.h"
#include "processing.h"

// TODO: Make debug fakecamera.c which generates a random array
// The size of the camera image is 2592 x 1944

int main(int argc, char *argv[]) {
    GENAPIC_RESULT res; // Return value of pylon methods
    PYLON_DEVICE_HANDLE hDev; // Handle for a pylon device
    
    size_t payloadSize; // Size of an image frame;
    unsigned char* imgBuf; // Image buffer
    int xMax, yMax, xAvg, yAvg; // Image properties

    puts("Hello!");

    PylonInitialize();

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
    res = PylonCreateDeviceByIndex( 0, &hDev );
    CHECK(res);
    
    // Initialize the device
    initializeDevice(hDev);
    
    // Get the name of the device
    if (PylonDeviceFeatureIsReadable(hDev, "DeviceModelName")) {
        char buf[256];
        size_t siz;

        res = PylonDeviceFeatureToString(hDev, "DeviceModelName", buf, &siz);
        CHECK(res);
        printf("Using camera %s\n", buf);
    }
    
    // Allocate memory for images
    getPayloadSize(hDev, &payloadSize);
    imgBuf = (unsigned char*) malloc(payloadSize);
    if (imgBuf == NULL) {
        fprintf(stderr, "Out of memory!\n");
        PylonTerminate();
        exit(EXIT_FAILURE);
    }
    
    // Grab some images
    puts("Grabbing Images!");
    for (int i = 0; i < 20; i++) {
        PylonGrabResult_t grabResult;
        _Bool bufferReady;
        
        // Grab one frame from stream channel 0
        // Camera is in single frame acquisition mode
        // Wait up to 500 ms for the image to be grabbed
        res = PylonDeviceGrabSingleFrame(hDev, 0, imgBuf, payloadSize, &grabResult, &bufferReady, 1000);
        
        if (res == GENAPI_E_OK && !bufferReady) {
            // Timeout occured!
            printf("Frame %d: timeout\n", i+1);
        }
        CHECK(res);
        
        if (grabResult.Status == Grabbed) {
            // TODO: Figure out why grab result payload size is 0
            // printf("%3d Pixel Value: %3u\n", i+1, imgBuf[100]);
            printf("Payload size: %ld\n", grabResult.PayloadSize);
            //printf("%d %d\n", grabResult.SizeX, grabResult.SizeY);
            getBeamProperties(imgBuf, grabResult.SizeX, grabResult.SizeY, &xMax, &yMax, &xAvg, &yAvg);
            //printf("Brightest pixel: (%4d, %4d)\tCentroid: (%4d %4d)\n", xMax, yMax, xAvg, yAvg);
            
            // Display Image
            res = PylonImageWindowDisplayImageGrabResult(0, &grabResult);
            CHECK(res);
        }
    }
    
    
    // Close and release the pylon device
    res = PylonDeviceClose(hDev);
    CHECK(res);
    res = PylonDestroyDevice(hDev);
    CHECK(res);

    PylonTerminate();
    
    puts("Goodbye!");
    return 0;
}