#include <stdio.h>
#include <math.h>
#include "processing.h"

// TODO: Implement this
int getBeamProperties(const unsigned char *imgBuf, long long int sizeX, long long int sizeY, struct beamProperties *p) {
    // Brightest pixel
    unsigned char max = 0;
    int xMax = 0, yMax = 0;

    // First moment
    double EX = 0, EY = 0;
    
    // Second moment
    double EX2 = 0, EY2 = 0;

    int i=0;
    for (int row=0; row < sizeY; row++) {
        for (int col=0; col < sizeX; col++) {
            // Find brightest pixel
            unsigned char val = imgBuf[i];
            if (val > max) {
                max = val;
                xMax = col;
                yMax = row;
            }

            // Measure mean and standard deviation
            double intensity = (double)val;
            double EX_term = intensity*row, EY_term = intensity*col;
            EX += EX_term;
            EY += EY_term;
            EX2 += EX_term * row;
            EY2 += EY_term * col;

            i++;
        }
    }

    long long int N = sizeX * sizeY;
    EX /= N;
    EY /= N;
    EX2 /= N;
    EY2 /= N;

    p->xAvg = EX;
    p->yAvg = EY;
    p->xStd = EX2 - EX*EX;
    p->yStd = EY2 - EY*EY;

    p->xMax = xMax;
    p->yMax = yMax;
    
    //UNWRAP(maxIndex, sizeX, p->xMax, p->yMax);
    
    return 0;
}
