#include <stdio.h>
#include <math.h>
#include "processing.h"

// TODO: Implement this
int getBeamProperties(const unsigned char *imgBuf, long long int sizeX, long long int sizeY, struct beamProperties *p) {
    // TEMP
    p->xAvg = 0;
    p->yAvg = 0;
    
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
    UNWRAP(maxIndex, sizeX, p->xMax, p->yMax);
    
    return 0;
}