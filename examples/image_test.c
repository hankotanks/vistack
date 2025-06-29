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

    vi_ImageRaw img = vi_ImageRaw_load(argv[1]);
    vi_ImageIntensity img_i = vi_ImageIntensity_from_ImageRaw(img);
    vi_ImageIntensity_show(img_i);
    free(img);
    free(img_i);
}
