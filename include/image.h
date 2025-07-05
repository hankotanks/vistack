#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stddef.h>

#include "mat.h"
#include "plot.h"

typedef struct __IMAGE_H__vi_ImageRaw* vi_ImageRaw;
typedef struct __IMAGE_H__vi_ImageIntensity* vi_ImageIntensity;

vi_ImageRaw 
vi_ImageRaw_load(const char* path);
vi_ImageIntensity 
vi_ImageIntensity_from_ImageRaw(const vi_ImageRaw image_raw);

void 
vi_ImageIntensity_show(vi_ImageIntensity image, size_t* corners, size_t count);
unsigned long
vi_ImageIntensity_rows(vi_ImageIntensity image);
unsigned long
vi_ImageIntensity_cols(vi_ImageIntensity image);
const double* 
vi_ImageIntensity_buffer(vi_ImageIntensity image);

vi_Mat
vi_ImageIntensity_to_Mat(vi_ImageIntensity image);
vi_ImageIntensity
vi_Mat_to_ImageIntensity(vi_Mat mat);

vi_Plotter
vi_ImageIntensity_plotter(vi_ImageIntensity image);

#define QUICK_SHOW_MAT_AS_IMAGE(mat) {\
    vi_Mat _mat = mat;\
    vi_ImageIntensity _mat##_image_temp = vi_Mat_to_ImageIntensity((_mat));\
    vi_Plot _mat##_plot_temp;\
    _mat##_plot_temp = vi_Plot_init("FLA_Obj Image View", 1, 1);\
    vi_Plot_add_layer(_mat##_plot_temp, 0, 0, vi_ImageIntensity_plotter(_mat##_image_temp));\
    vi_Plot_show(_mat##_plot_temp);\
    vi_Plot_free(_mat##_plot_temp);\
    free(_mat##_image_temp);\
}

#endif // __IMAGE_H__

