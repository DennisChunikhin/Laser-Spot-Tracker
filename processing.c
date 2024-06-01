#include <stdio.h>
#include <math.h>
#include "processing.h"

// TODO: Implement this
int getBeamProperties(const unsigned char *imgBuf, int sizeX, int sizeY, int *xMax, int *yMax, int *xAvg, int *yAvg) {
    // TEMP
    *xAvg = 0;
    *yAvg = 0;
    
    unsigned char max = 0;
    int maxIndex = 0;
    
    for (int i=0; i < sizeX * sizeY; i++) {
        // Find brightest pixel
        unsigned char val = imgBuf[i];
        if (val > max) {
            max = val;
            maxIndex = i;
        }
    }
    
    printf("%d\n",maxIndex);
    
    return 0;
}