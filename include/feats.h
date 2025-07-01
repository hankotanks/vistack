#ifndef __FEATS_H__
#define __FEATS_H__

#include <stddef.h>
#include "plot.h"
#include "image.h"

typedef struct {
    double* jx;
    double* jy;
    size_t* corners;
    size_t count;
    size_t rows, cols;
} vi_HarrisCorners;

vi_HarrisCorners
vi_HarrisCorners_compute(vi_ImageIntensity img, double t, size_t s);
vi_Plotter
vi_HarrisCorners_plotter(vi_HarrisCorners corners);
void
vi_HarrisCorners_free(vi_HarrisCorners corners);

typedef struct __FEATS_H__vi_ImageDescriptor* vi_ImageDescriptor;

vi_ImageDescriptor
vi_ImageDescriptor_from_HarrisCorners(vi_HarrisCorners in, vi_HarrisCorners* out);
void
vi_ImageDescriptor_free(vi_ImageDescriptor desc);
void
vi_ImageDescriptor_dump(vi_ImageDescriptor desc);

typedef struct __FEATS_H__vi_ImageMatches* vi_ImageMatches;

vi_ImageMatches
vi_ImageMatches_compute(vi_ImageDescriptor a, vi_ImageDescriptor b, double t);
void
vi_ImageMatches_free(vi_ImageMatches matches);
void
vi_ImageMatches_dump(vi_ImageMatches matches, vi_HarrisCorners a, vi_HarrisCorners b);

#endif // __FEATS_H__
