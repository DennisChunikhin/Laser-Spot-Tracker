#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <windows.h>
#include <time.h>

#include <pylonc/PylonC.h>

#include "initialize.h"
#include "processing.h"
#include "camera.h"


// TODO: Make debug fakecamera.c which generates a random array
// The size of the camera image is 2592 x 1944

// TODO: Replace exits with g_task_return_error()

void camera(GTask *task, gpointer source_obj, gpointer _task_data, GCancellable *cancellable) {
    GENAPIC_RESULT res; // Return value of pylon methods
    
    struct TaskData *task_data = _task_data;

    // Pylon handles
    PYLON_DEVICE_HANDLE hDev = task_data->hDev; // Handle for a pylon device
    PYLON_STREAMGRABBER_HANDLE hGrabber;
    PYLON_WAITOBJECT_HANDLE hWait;
    
    // Image buffers
    unsigned char *buffers[NUM_BUFFERS];
    PYLON_STREAMBUFFER_HANDLE bufHandles[NUM_BUFFERS];
    
    size_t nStreams, payloadSize; // Number of streams, size of an image frame
    int64_t sizeX, sizeY; // Image AOI
    struct beamProperties *beamProps = (struct beamProperties*)malloc(sizeof(struct beamProperties)); // Image properties
    
    puts("Hello!");
    
    /* Setup  Camera */
    
    //PylonInitialize();

    // Get first device
    //char *name = getFirstDevice(&hDev);
    //printf("Using camera %s\n", name);
    //free(name);
    
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
    
    // Allocate the resources required for grabbing
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
    
    // Open Data File
    //FILE *dataFile = setupDataFile();
    
    // Grab images
    //for (int i=0; i < 100; i++) {
    int i=0;
    while (!g_cancellable_is_cancelled(cancellable)) {
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
            //printf("Frame %d; Brightest: (%4u, %4u); Mean: (%4.4f, %4.4f); Std: (%4.4f, %4.4f)\n", i, beamProps->xMax, beamProps->yMax, beamProps->xAvg, beamProps->yAvg, beamProps->xStd, beamProps->yStd);
            
            // Write Beam Data
	    g_mutex_lock(task_data->lock);
	    if (task_data->save)
            	writeData(task_data->fp, beamProps);
	    g_mutex_unlock(task_data->lock);
            
	    // Display Image
            res = PylonImageWindowDisplayImageGrabResult(0, &grabResult);
            CHECK(res);
	    
	    GBytes *bytes = mono8_to_rgb_bytes(buffer, sizeX, sizeY);
	    GdkMemoryTexture *texture = gdk_memory_texture_new(sizeX, sizeY, GDK_MEMORY_R8G8B8, bytes, sizeX*BYTES_PER_R8G8B8);
	    gtk_picture_set_paintable(task_data->image_window, GDK_PAINTABLE(texture));
        }
        else if (grabResult.Status == Failed) {
            fprintf(stderr, "Frame %d was not grabbed successfully. Error code = 0x%08X\n", i, grabResult.ErrorCode);
        }
        
        // Requeue the buffer to be filled again
        res = PylonStreamGrabberQueueBuffer(hGrabber, grabResult.hBuffer, (void *)bufferIndex);
        CHECK(res);
    }

    
    /* Clean up */

    //fclose(dataFile);
    
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
    //res = PylonDeviceClose(hDev);
    //CHECK(res);
    //res = PylonDestroyDevice(hDev);
    //CHECK(res);

    //PylonTerminate();
    
    //puts("Goodbye!");
}

// Set up and open a file to store acquired data
FILE *setupDataFile() {
    // Set file name to current time
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char fname[34] = "Data/";
    strcat(fname, asctime(timeinfo));
    fname[18] = '-';
    fname[21] = '-';
    fname[29] = '.';
    fname[30] = 't';
    fname[31] = 'x';
    fname[32] = 't';
    fname[33] = '\0';

    CreateDirectory("Data", NULL);

    FILE *fp = fopen(fname, "a");
    if (fp == NULL) {
        fprintf(stderr, "Could not create a data file, exiting.");
        exit(EXIT_FAILURE);
    }

    // Write Header
    fprintf(fp, "xMax yMax xMean yMean xStd yStd\n");
    
    return fp;
}

void writeData(FILE *fp, const struct beamProperties *p) {
    int res;

    if( fprintf(fp, "%u %u %f %f %f %f\n", p->xMax, p->yMax, p->xAvg, p->yAvg, p->xStd, p->yStd) < 0 ) {
        // Failed Write
        fprintf(stderr, "Failed to write to write to data file, creating new data file.");

        fclose(fp);
        fp = setupDataFile(fp); // TODO: Check that this is fine

        writeData(fp, p);
    }
}

GBytes *mono8_to_rgb_bytes(unsigned char *data, int width, int height) {
	GByteArray *pixels = g_byte_array_new();

	for (int row = 0; row < height; row ++) {
		for (int col = 0; col < width; col++) {
			g_byte_array_append(pixels, data, 1); // R
			g_byte_array_append(pixels, data, 1); // G
			g_byte_array_append(pixels, data++, 1); // B
		}
	}

	return g_byte_array_free_to_bytes(pixels);
}
