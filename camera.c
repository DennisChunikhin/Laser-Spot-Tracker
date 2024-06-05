#include <stdio.h>
#include <malloc.h>
#include <pylonc/PylonC.h>
#include "initialize.h"
#include "processing.h"

#include <windows.h>

// TODO: Make debug fakecamera.c which generates a random array
// The size of the camera image is 2592 x 1944

#define NUM_BUFFERS 5
#define TIMEOUT 1000 // ms to wait for image before timing out

int main(int argc, char *argv[]) {
    GENAPIC_RESULT res; // Return value of pylon methods
    
    // Pylon handles
    PYLON_DEVICE_HANDLE hDev; // Handle for a pylon device
    PYLON_STREAMGRABBER_HANDLE hGrabber;
    PYLON_WAITOBJECT_HANDLE hWait;
    
    // Image buffers
    unsigned char *buffers[NUM_BUFFERS];
    PYLON_STREAMBUFFER_HANDLE bufHandles[NUM_BUFFERS];
    
    size_t nStreams, payloadSize; // Number of streams, size of an image frame
    int64_t sizeX, sizeY; // Image AOI
    struct beamProperties *beamProps = (struct beamProperties*)malloc(sizeof(struct beamProperties)); // Image properties
    
    puts("Hello!");
    
    
    /* Setup */
    
    PylonInitialize();

    // Get first device
    char *name = getFirstDevice(&hDev);
    printf("Using camera %s\n", name);
    free(name);
    
    // Initialize the device
    initializeDevice(hDev, &sizeX, &sizeY);
    
    
    /* Set up stream grabbing */
    
    // Check number of stream grabber channels
    res = PylonDeviceGetNumStreamGrabberChannels(hDev, &nStreams);
    CHECK(res);
    //printf("Number of channels: %ld\n", nStreams);
    if (nStreams < 1) {
        fprintf(stderr, "The transport layer doesn't support image streams.\n");
        PylonTerminate();
        exit(EXIT_FAILURE);
    }
    
    // Create and open a stream grabber
    res = PylonDeviceGetStreamGrabber(hDev, 0, &hGrabber);
    CHECK(res);
    res = PylonStreamGrabberOpen(hGrabber);
    CHECK(res);
    
    // Get a handle for the stream grabber's wait object
    res = PylonStreamGrabberGetWaitObject(hGrabber, &hWait);
    CHECK(res);
    
    // Get minimum size of grab buffer
    res = PylonStreamGrabberGetPayloadSize(hDev, hGrabber, &payloadSize);
    CHECK(res);
    
    // Allocate memory for grabbing
    for (int i=0; i<NUM_BUFFERS; i++) {
        buffers[i] = (unsigned char *)malloc(sizeof(unsigned char)*payloadSize);
        if (buffers[i] == NULL) {
            fprintf(stderr, "Out of memory!\n");
            PylonTerminate();
            exit(EXIT_FAILURE);
        }
    }
    
    // Tell stream grabber number and size of buffers
    res = PylonStreamGrabberSetMaxNumBuffer(hGrabber, NUM_BUFFERS);
    CHECK(res);
    res = PylonStreamGrabberSetMaxBufferSize(hGrabber, payloadSize);
    CHECK(res);
    
    // ALlocate the resources required for grabbing
    res = PylonStreamGrabberPrepareGrab(hGrabber);
    CHECK(res);
    
    // Register the buffers for grabbing
    // From now on bufHandles should be used instead of the pointers
    for (int i=0; i < NUM_BUFFERS; i++) {
        res = PylonStreamGrabberRegisterBuffer(hGrabber, buffers[i], payloadSize, &bufHandles[i]);
    }
    
    // Feed the buffers into the stream grabber's input queue
    // The index is used as optional context information
    for (int i=0; i < NUM_BUFFERS; i++) {
        res = PylonStreamGrabberQueueBuffer(hGrabber, bufHandles[i], (void *)i);
        CHECK(res);
    }
    
    
    /* Grab Images */
    
    puts("Grabbing Images!");
    
    // Start image acquisition
    res = PylonStreamGrabberStartStreamingIfMandatory(hGrabber);
    CHECK(res);
    
    res = PylonDeviceExecuteCommandFeature(hDev, "AcquisitionStart");
    CHECK(res);
    
    _Bool isReady;
    PylonGrabResult_t grabResult;
    
    //res = PylonImageWindowCreate(0, 0, 0, 1000, 1000);
    
    for (int i=0; i < 20; i++) {
        size_t bufferIndex;
        
        res = PylonWaitObjectWait(hWait, TIMEOUT, &isReady);
        CHECK(res);
        
        if (!isReady) {
            fprintf(stderr, "Grab timeout occured!\n");
            break;
        }
        
        // Retrieve grabbed image
        res = PylonStreamGrabberRetrieveResult(hGrabber, &grabResult, &isReady);
        CHECK(res);
        if (!isReady) {
            // Strange error!
            fprintf(stderr, "Failed to retrieve a grab result\n");
            break;
        }
        
        bufferIndex = (size_t) grabResult.Context;
        
        if (grabResult.Status == Grabbed) {
            // Image processing
            // Remaining buffers are filled while we do processing
            
            unsigned char *buffer = (unsigned char*) grabResult.pBuffer;
            
            // Process Image
            getBeamProperties(buffer, sizeX, sizeY, beamProps);
            printf("Frame %d; Brightest: (%4d, %4d)\n", i, beamProps->xMax, beamProps->yMax);
            
            // TODO: Write Data
            // Potentially maintain a buffer to write data in batches
            
            res = PylonImageWindowDisplayImageGrabResult(0, &grabResult);
            CHECK(res);
        }
        else if (grabResult.Status == Failed) {
            fprintf(stderr, "Frame %d was not grabbed successfully. Error code = 0x%08X\n", i, grabResult.ErrorCode);
        }
        
        // Requeue the buffer to be filled again
        res = PylonStreamGrabberQueueBuffer(hGrabber, grabResult.hBuffer, (void *)bufferIndex);
        CHECK(res);
    }
    
    
    /* Clean up */
    
    // Stop Grabbing Images
    res = PylonDeviceExecuteCommandFeature(hDev, "AcquisitionStop");
    CHECK(res);
    
    res = PylonStreamGrabberStopStreamingIfMandatory(hGrabber);
    CHECK(res);
    
    res = PylonStreamGrabberFlushBuffersToOutput(hGrabber);
    CHECK(res);
    
    // Retrieve all buffers from stream grabber
    do {
        res = PylonStreamGrabberRetrieveResult(hGrabber, &grabResult, &isReady);
        CHECK(res);
    } while(isReady);
    
    // Now the buffers can be deregistered
    for (int i = 0; i < NUM_BUFFERS; i++) {
        res = PylonStreamGrabberDeregisterBuffer(hGrabber, bufHandles[i]);
        CHECK(res);
        free(buffers[i]);
    }
    
    // Release grabber related resources
    res = PylonStreamGrabberFinishGrab(hGrabber);
    CHECK(res);
    res = PylonStreamGrabberClose(hGrabber);
    CHECK(res);
    
    free(beamProps);
    
    // Close and release the pylon device
    res = PylonDeviceClose(hDev);
    CHECK(res);
    res = PylonDestroyDevice(hDev);
    CHECK(res);

    PylonTerminate();
    
    puts("Goodbye!");
    return 0;
}