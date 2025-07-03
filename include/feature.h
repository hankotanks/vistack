#ifndef __FEATURE_H__
#define __FEATURE_H__

#include <stddef.h>
#include "plot.h"

#include "mat.h"
#include "image.h"

typedef struct {
    size_t* x;
    size_t* y;
} vi_CornerList;

typedef struct {
    vi_Mat image;
    vi_Mat dx;
    vi_Mat dy;
    size_t offset;
} vi_ImageData;

vi_ImageData
vi_ImageData_init(vi_ImageIntensity image, vi_Mat kernel);
void
vi_ImageData_free(vi_ImageData data);

typedef void (* vi_CornerDetector)(vi_Mat, vi_Mat[static 3]);

void vi_HarrisDetector(vi_Mat r, vi_Mat m[static 3]);

void vi_ShiTomasiDetector(vi_Mat r, vi_Mat m[static 3]);

vi_CornerList
vi_CornerList_init(vi_ImageData data, vi_CornerDetector op, double t);
void
vi_CornerList_free(vi_CornerList corners);
vi_Plotter
vi_CornerList_plot(vi_ImageData data, vi_CornerList corners);

#endif // __FEATURE_H__
