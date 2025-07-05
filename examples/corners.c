#include <stdlib.h>
#include <stdio.h>
#include <stb_ds.h>

#include "vistack.h"

#define HARRIS_THRESHOLD 0.5
#define HARRIS_NMS 5
#define LOWES_CRITERION 0.5

#define SHOW_PLOT

int main(int argc, char* argv[]) {
    // parse command line arguments
    if(argc != 3) {
        printf("[ERROR] Must provide path to 2 images.\n");
        return 1;
    }

    // begin environment
    vi_start();

    // load images
    vi_ImageRaw image_temp;
    vi_ImageIntensity images[2];
    image_temp = vi_ImageRaw_load(argv[1]);
    images[0] = vi_ImageIntensity_from_ImageRaw(image_temp);
    free(image_temp);
    image_temp = vi_ImageRaw_load(argv[2]);
    images[1] = vi_ImageIntensity_from_ImageRaw(image_temp);
    free(image_temp);
    
    vi_Mat kernel = vi_Mat_init(3, 3,
         3.0,  10.0,  3.0,
         0.0,   0.0,  0.0,
        -3.0, -10.0, -3.0
    );
    vi_Mat_scale(kernel, 1.0 / 32.0);

    vi_ImageData image_data[2];
    image_data[0] = vi_ImageData_init(images[0], kernel);
    image_data[1] = vi_ImageData_init(images[1], kernel);
    vi_Mat_free(kernel);

    vi_CornerList corners[2];
    corners[0] = vi_CornerList_init(image_data[0], vi_CornerDetector_Harris, HARRIS_THRESHOLD);
    corners[1] = vi_CornerList_init(image_data[1], vi_CornerDetector_Harris, HARRIS_THRESHOLD);

    // descriptors
    vi_Desc desc[2];
    desc[0] = vi_Desc_init(image_data[0], &corners[0], vi_DescBuilder_SIFT);
    desc[1] = vi_Desc_init(image_data[1], &corners[1], vi_DescBuilder_SIFT);

    // matches
    vi_ImageMatches matches = vi_ImageMatches_init(desc, LOWES_CRITERION);
    
    // plot the images
#ifdef SHOW_PLOT
    vi_Plot plot;
    plot = vi_Plot_init("feature matches", 1, 2);
    vi_Plot_add_layer(plot, 0, 0, 1, 1, vi_ImageIntensity_plotter(images[0]));
    vi_Plot_add_layer(plot, 0, 0, 1, 1, vi_CornerList_plot(image_data[0], corners[0]));
    vi_Plot_add_layer(plot, 0, 1, 1, 1, vi_ImageIntensity_plotter(images[1]));
    vi_Plot_add_layer(plot, 0, 1, 1, 1, vi_CornerList_plot(image_data[1], corners[1]));
    vi_Plot_add_layer(plot, 0, 0, 1, 2, vi_ImageMatches_plot(matches, image_data, corners));
    vi_Plot_show(plot);
    vi_Plot_free(plot);
#endif

    // clean up
    free(images[0]);
    free(images[1]);
    vi_ImageData_free(image_data[0]);
    vi_ImageData_free(image_data[1]);
    vi_CornerList_free(corners[0]);
    vi_CornerList_free(corners[1]);
    vi_Desc_free(desc[0]);
    vi_Desc_free(desc[1]);
    vi_ImageMatches_free(matches);

    // end environment
    vi_finish();

    return 0;
}
