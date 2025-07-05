#include <stdlib.h>
#include <stdio.h>
#include <stb_ds.h>
#include <glenv.h>

#include "vistack.h"


#define HARRIS_THRESHOLD 0.2
#define HARRIS_NMS 5
#define LOWES_CRITERION 0.5

int main(int argc, char* argv[]) {
    // parse command line arguments
    if(argc != 3) {
        printf("[ERROR] Must provide path to 2 images.\n");
        return 1;
    }

    vi_start();

    // load images
    vi_ImageRaw image_temp;
    vi_ImageIntensity image_a, image_b;
    image_temp = vi_ImageRaw_load(argv[1]);
    image_a = vi_ImageIntensity_from_ImageRaw(image_temp);
    free(image_temp);
    image_temp = vi_ImageRaw_load(argv[2]);
    image_b = vi_ImageIntensity_from_ImageRaw(image_temp);
    free(image_temp);
    
    vi_Mat kernel = vi_Mat_init(3, 3,
         3.0,  10.0,  3.0,
         0.0,   0.0,  0.0,
        -3.0, -10.0, -3.0
    );
    vi_Mat_scale(kernel, 1.0 / 32.0);

    vi_ImageData data_a, data_b;
    data_a = vi_ImageData_init(image_a, kernel);
    data_b = vi_ImageData_init(image_b, kernel);
    vi_Mat_free(kernel);

    vi_CornerList corners_a, corners_b;
    corners_a = vi_CornerList_init(data_a, vi_CornerDetector_Harris, HARRIS_THRESHOLD);
    corners_b = vi_CornerList_init(data_b, vi_CornerDetector_Harris, HARRIS_THRESHOLD);
    
    // plot the images
#if 1
    vi_Plot plot;
    plot = vi_Plot_init("corners", 1, 2);
    vi_Plot_add_layer(plot, 0, 0, vi_ImageIntensity_plotter(image_a));
    vi_Plot_add_layer(plot, 0, 0, vi_CornerList_plot(data_a, corners_a));
    vi_Plot_add_layer(plot, 1, 0, vi_ImageIntensity_plotter(image_b));
    vi_Plot_add_layer(plot, 1, 0, vi_CornerList_plot(data_b, corners_b));
    vi_Plot_show(plot);
    vi_Plot_free(plot);
#endif

    // descriptors
    vi_Desc desc_a = vi_Desc_init(data_a, &corners_a, vi_DescBuilder_SIFT);
    vi_Desc desc_b = vi_Desc_init(data_b, &corners_b, vi_DescBuilder_SIFT);

    // matches
    vi_ImageMatches matches = vi_ImageMatches_init(desc_a, desc_b, LOWES_CRITERION);
    // vi_ImageMatches_show(matches, desc_a, desc_b);
    
    // clean up
    vi_ImageData_free(data_a);
    vi_ImageData_free(data_b);
    vi_CornerList_free(corners_a);
    vi_CornerList_free(corners_b);
    vi_Desc_free(desc_a);
    vi_Desc_free(desc_b);
    vi_ImageMatches_free(matches);
    free(image_a);
    free(image_b);

    vi_finish();

    return 0;
}
