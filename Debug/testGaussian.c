#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../processing.h"

#define SIZEX 2592
#define SIZEY 1944

#define CENTX 1000
#define CENTY 300
#define SIGMX 50
#define SIGMY 35

int main(int argc, char *argv[]) {
    unsigned long int N = SIZEX * SIZEY;
    unsigned char *gaussian = (unsigned char*) malloc(sizeof(unsigned char)*N);
    
    // Generate a gaussian beam profile
    long int i = 0;
    for (int y = 0; y < SIZEY; y++) {
        for (int x = 0; x < SIZEX; x++) {
            int x1 = x-CENTX;
            int y1 = y-CENTY;
            
            gaussian[i] = exp( -((double) x1*x1 / 2 / SIGMX / SIGMX + (double) y1*y1 / 2 / SIGMY / SIGMY) ) * 255;
            
            i++;
        }
    }

    puts("Generated Gaussian");
    
    
    // Write gaussian profile to file
    FILE *fp = fopen("gaussian.txt", "w");
    if (fp == NULL) {
        fprintf(stderr, "Could not open file.\n");
        exit(EXIT_FAILURE);
    }
    
    
    for (unsigned char *p = gaussian; p < gaussian+N; p++) {
        fprintf(fp, "%d ", *p);
    }
    
    fclose(fp);

    puts("Wrote Gaussian");
    
    
    // Test processing function
    struct beamProperties p;
    getBeamProperties(gaussian, SIZEX, SIZEY, &p);
    
    // TODO: How to properly store Mono12 (can't in unsigned char--only 1 byte)
    printf("Max: (%4u, %4u)\n", p.xMax, p.yMax);
    printf("Avg: (%4.2f, %4.2f)\n", p.xAvg, p.yAvg);
    printf("Std: (%4.2f, %4.2f)\n", p.xStd, p.yStd);
    
    return 0;
}
