#include <stdio.h>
#include <math.h>
#include "processing.h"


int getBeamProperties(const unsigned char *imgBuf, long long int sizeX, long long int sizeY, struct beamProperties *p) {
    // Brightest pixel
    unsigned char max = 0;
    unsigned int xMax = 0, yMax = 0;

    // First moment
    unsigned long long int sumX = 0, sumY = 0;
    
    // Second moment
    unsigned long long int sumX2 = 0, sumY2 = 0;

    // Running sum (used to normalize)
    unsigned long long int sum = 0;

    //printf("%ld %ld %ld\n", sizeof(unsigned int), sizeof(unsigned long int), sizeof(unsigned long long int));
    
    unsigned long int i=0;
    for (unsigned int row=0; row < sizeY; row++) {
        for (unsigned int col=0; col < sizeX; col++) {
            // Find brightest pixel
            unsigned char val = imgBuf[i];
            if (val > max) {
                max = val;
                xMax = col;
                yMax = row;
            }

            // Measure mean and standard deviation
            unsigned long long int EX_term = (unsigned long long int)col*val, EY_term = (unsigned long long int)row*val;
            sum += val;
            sumX += EX_term;
            sumY += EY_term;
            sumX2 += EX_term * col;
            sumY2 += EY_term * row;

            i++;
        }
    }

    double EX = (double) sumX / sum;
    double EY = (double) sumY / sum;

    p->xAvg = EX;
    p->yAvg = EY;
    p->xStd = sqrt((double) sumX2/sum - EX*EX);
    p->yStd = sqrt((double) sumY2/sum - EY*EY);

    p->xMax = xMax;
    p->yMax = yMax;
    
    //UNWRAP(maxIndex, sizeX, p->xMax, p->yMax);
    
    return 0;
}
