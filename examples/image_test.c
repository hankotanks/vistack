#include "corners.h"
#include "image.h"
#include <stdlib.h>
#include <stdio.h>
#include <glenv.h>
#include <stb_ds.h>
#include <vistack.h>

#define HARRIS_THRESHOLD 0.5f
#define HARRIS_NMS 5
#define HARRIS_PRINT_CORNERS

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("[ERROR] Must provide path to image.\n");
        return 1;
    }
    vi_ImageRaw temp = vi_ImageRaw_load(argv[1]);
    vi_ImageIntensity img = vi_ImageIntensity_from_ImageRaw(temp);
    free(temp);
    vi_HarrisCorners corners = vi_HarrisCorners_from_ImageIntensity(img, HARRIS_THRESHOLD, HARRIS_NMS);
#ifdef HARRIS_PRINT_CORNERS
    for(size_t i = 0; i < corners.corner_count; ++i) 
        printf("[%zu, %zu]\n", corners.corners[i * 2], corners.corners[i * 2 + 1]);
#endif
    vi_ImageIntensity_show(img, corners.corners, corners.corner_count);
    stbds_arrfree(corners.corners);
    free(img);
    return 0;
}
