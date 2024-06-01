#ifndef PROCESSING_H

// Given column (X) and row (Y) coordinates of a 2D array gives the equivalent index of the array in row-major form
#define WRAP(X, Y, sizeX) ((Y)*(sizeX) + (X))

//TODO: MAKE SURE THIS IS FINE
// Given and index of a 2D array in row-major form and the 2D array's dimensions, defines X and Y as the equivalent column and row (correspondingly) coordinates
#define UNWRAP(index, sizeX, sizeY, X, Y) \
    Y = (index)/(sizeX); \
    X = ((index)-(Y))

int getBeamProperties(const unsigned char *imgBuf, int sizeX, int sizeY, int *xBrightest, int *yBrightest, int *xAverage, int *yAverage);

#endif