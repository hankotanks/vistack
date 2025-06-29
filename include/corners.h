#ifndef __CORNERS_H__
#define __CORNERS_H__

#include "image.h"

typedef struct {
    size_t* corners, corner_count;
} vi_HarrisCorners;

vi_HarrisCorners
vi_HarrisCorners_from_ImageIntensity(vi_ImageIntensity img, double t);

#endif // __CORNERS_H__