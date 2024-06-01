#ifndef INITIALIZE_H

// GigE camera packet size
#define PACKET_SIZE 1500

// Simple error handling
#define CHECK(errc) if ( GENAPI_E_OK != errc ) printErrorAndExit( errc )

void printErrorAndExit(GENAPIC_RESULT errc);

void initializeDevice(PYLON_DEVICE_HANDLE hDev); // Opens the device and sets up its configurations

int getPayloadSize(PYLON_DEVICE_HANDLE hDev, int *payloadSize); // Gets the size of the image buffer by temporarily opening a stream grabber

#endif