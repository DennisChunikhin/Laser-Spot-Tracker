#include <stdio.h>
#include <pylonc/PylonC.h>

int main(int argc, char *argv[]) {
    puts("Initializing");
    
    PylonInitialize();
    
    int numDevices;
    PylonEnumerateDevices(&numDevices);
    printf("Number of pylon devices: %d\n", numDevices);
    
    PylonTerminate();
    return 0;
}