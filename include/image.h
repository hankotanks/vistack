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
vi_ImageIntensity_from_ImageRaw(const vi_ImageRaw img);

void 
vi_ImageIntensity_show(vi_ImageIntensity img, size_t* corners, size_t count);
unsigned long
vi_ImageIntensity_rows(vi_ImageIntensity img);
unsigned long
vi_ImageIntensity_cols(vi_ImageIntensity img);
const double* 
vi_ImageIntensity_buffer(vi_ImageIntensity img);

vi_Mat
vi_ImageIntensity_to_Mat(vi_ImageIntensity img);
vi_ImageIntensity
vi_Mat_to_ImageIntensity(vi_Mat mat);

vi_Plotter
vi_ImageIntensity_plotter(vi_ImageIntensity img);

#define QUICK_SHOW_MAT_AS_IMAGE(_mat) {\
    vi_ImageIntensity _mat##_img_tmp = vi_Mat_to_ImageIntensity((_mat));\
    vi_Plot _mat##_plot_tmp;\
    _mat##_plot_tmp = vi_Plot_init("FLA_Obj Image View", 1, 1);\
    vi_Plot_add_layer(_mat##_plot_tmp, 0, 0, vi_ImageIntensity_plotter(_mat##_img_tmp));\
    vi_Plot_show(_mat##_plot_tmp);\
    vi_Plot_free(_mat##_plot_tmp);\
    free(_mat##_img_tmp);\
}

#endif // __IMAGE_H__

