#ifndef __FEATURE_H__
#define __FEATURE_H__

#include <stddef.h>
#include <stdbool.h>

#include "plot.h"
#include "mat.h"
#include "image.h"

typedef struct {
    vi_Mat image;
    vi_Mat dx;
    vi_Mat dy;
    size_t offset;
} vi_ImageData;

typedef struct {
    size_t* rows;
    size_t* cols;
} vi_CornerList;

// contains image gradients and information for feature detection
vi_ImageData
vi_ImageData_init(vi_ImageIntensity image, vi_Mat kernel);
void
vi_ImageData_free(vi_ImageData data);

// function used to process corner strength
typedef void (*vi_CornerDetector)(vi_Mat, vi_Mat[static 3]);
// pre-implemented detectors
void 
vi_CornerDetector_Harris(vi_Mat r, vi_Mat m[static 3]);
void 
vi_CornerDetector_ShiTomasi(vi_Mat r, vi_Mat m[static 3]);

// functionality for handling vi_CornerList
vi_CornerList
vi_CornerList_init(vi_ImageData data, vi_CornerDetector op, double t);
void
vi_CornerList_free(vi_CornerList corners);
vi_Plotter
vi_CornerList_plot(vi_ImageData data, vi_CornerList corners);

// set of descriptors corresponding to an image
typedef struct __FEATURE_H__vi_Desc* vi_Desc;

// function type used to build descriptors from corner coordinates
typedef struct {
    size_t desc_size, data_size;
    void (*setup)(vi_ImageData, void*);
    bool (*build)(const void*, size_t, size_t, unsigned char[]);
} vi_DescBuilder;

// pre-implemented descriptor builders
vi_DescBuilder
__FEATURE_H__vi_DescBuilder_SIFT();
#define vi_DescBuilder_SIFT __FEATURE_H__vi_DescBuilder_SIFT()

// vi_Desc functionality
vi_Desc
vi_Desc_init(vi_ImageData data, vi_CornerList* corners, vi_DescBuilder builder);
void
vi_Desc_show(vi_Desc desc);
void
vi_Desc_free(vi_Desc desc);

#endif // __FEATURE_H__
