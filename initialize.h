#ifndef INITIALIZE_H
#define INITIALIZE_H

// GigE camera packet size
#define PACKET_SIZE 1500

// Simple error handling
#define CHECK(errc) if ( GENAPI_E_OK != errc ) printErrorAndExit( errc )

void printErrorAndExit(GENAPIC_RESULT errc);

char *getFirstDevice(PYLON_DEVICE_HANDLE *hDev); // Gets and opens the first device, returns the device name

void initializeDevice(PYLON_DEVICE_HANDLE hDev, int64_t *sizeX, int64_t *sizeY); // Sets up the device configurations

int getPayloadSize(PYLON_DEVICE_HANDLE hDev, int *payloadSize); // Gets the size of the image buffer by temporarily opening a stream grabber

#endif
