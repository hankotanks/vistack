#include "corners.h"
#include "image.h"
#include <stdlib.h>
#include <stdio.h>
#include <glenv.h>
#include <vistack.h>

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("[ERROR] Must provide path to image.\n");
        return 1;
    }

    vi_ImageRaw temp = vi_ImageRaw_load(argv[1]);
    vi_ImageIntensity img = vi_ImageIntensity_from_ImageRaw(temp);
    free(temp);
    vi_ImageIntensity_show(img);
    vi_HarrisCorners_from_ImageIntensity(img, 0.9f);
    free(img);
    return 0;
}
