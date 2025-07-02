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
    // parse command line arguments
    if(argc != 3) {
        printf("[ERROR] Must provide path to 2 images.\n");
        return 1;
    }

    // load images
    vi_ImageRaw image_temp;
    vi_ImageIntensity image_a, image_b;
    image_temp = vi_ImageRaw_load(argv[1]);
    image_a = vi_ImageIntensity_from_ImageRaw(image_temp);
    free(image_temp);
    image_temp = vi_ImageRaw_load(argv[2]);
    image_b = vi_ImageIntensity_from_ImageRaw(image_temp);
    free(image_temp);
    
    // compute corners and descriptors
    vi_HarrisCorners corners_a, corners_b, corners_temp;
    corners_temp = vi_HarrisCorners_compute(image_a, HARRIS_THRESHOLD, HARRIS_NMS);
    vi_ImageDescriptor desc_a, desc_b;
    desc_a = vi_ImageDescriptor_from_HarrisCorners(corners_temp, &corners_a);
    vi_HarrisCorners_free(corners_temp);
    corners_temp = vi_HarrisCorners_compute(image_a, HARRIS_THRESHOLD, HARRIS_NMS);
    desc_b = vi_ImageDescriptor_from_HarrisCorners(corners_temp, &corners_b);
    vi_HarrisCorners_free(corners_temp);
    
    // plot the images
    vi_Plot plot;
    plot = vi_Plot_init();
    vi_Plot_add_layer(plot, vi_ImageIntensity_plotter(image_a));
    vi_Plot_add_layer(plot, vi_HarrisCorners_plotter(corners_a));
    vi_Plot_show(plot);
    vi_Plot_free(plot);
    plot = vi_Plot_init();
    vi_Plot_add_layer(plot, vi_ImageIntensity_plotter(image_b));
    vi_Plot_add_layer(plot, vi_HarrisCorners_plotter(corners_b));
    vi_Plot_show(plot);
    vi_Plot_free(plot);

    // clean up
    vi_HarrisCorners_free(corners_a);
    vi_HarrisCorners_free(corners_b);
    vi_ImageDescriptor_free(desc_a);
    vi_ImageDescriptor_free(desc_b);
    free(image_a);
    free(image_b);

    return 0;
}
