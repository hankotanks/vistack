#include "feats.h"
#include "image.h"
#include "plot.h"
#include <stdlib.h>
#include <stdio.h>
#include <stb_ds.h>
#include <glenv.h>
#include <vistack.h>

#define HARRIS_THRESHOLD 0.5f
#define HARRIS_NMS 5

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("[ERROR] Must provide path to image.\n");
        return 1;
    }
    vi_ImageRaw temp = vi_ImageRaw_load(argv[1]);
    vi_ImageIntensity image = vi_ImageIntensity_from_ImageRaw(temp);
    free(temp);
    vi_HarrisCorners corners = vi_HarrisCorners_compute(image, HARRIS_THRESHOLD, HARRIS_NMS);
    vi_ImageDescriptor desc = vi_ImageDescriptor_from_HarrisCorners(corners);
    vi_ImageDescriptor_dump(desc);
    vi_Plot plot = vi_Plot_init();
    vi_Plot_add_layer(plot, vi_ImageIntensity_plotter(image));
    vi_Plot_add_layer(plot, vi_HarrisCorners_plotter(corners));
    vi_Plot_show(plot);
    vi_Plot_show(plot);
    vi_Plot_free(plot);
    vi_HarrisCorners_free(corners);
    vi_ImageDescriptor_free(desc);
    free(image);
    return 0;
}
