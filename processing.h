#ifndef PROCESSING_H
#define PROCESSING_H

// Given column (X) and row (Y) coordinates of a 2D array gives the equivalent index of the array in row-major form
#define WRAP(X, Y, sizeX) ((Y)*(sizeX) + (X))

// Given and index of a 2D array in row-major form and the 2D array's dimensions, defines X and Y as the equivalent column and row (correspondingly) coordinates
#define UNWRAP(index, sizeX, X, Y) \
    Y = (index)/(sizeX); \
    X = ((index)%(sizeX))

struct beamProperties {
    int xMax;
    int yMax;
    int xAvg;
    int yAvg;
    int xStd;
    int yStd;
};

struct simpleStruct {
    int val;
    int val2;
    int val3;
    int val4;
    int val5;
};

int getBeamProperties(const unsigned char *imgBuf, long long int sizeX, long long int sizeY, struct beamProperties *p);

#endif
