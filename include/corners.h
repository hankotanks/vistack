#ifndef __CORNERS_H__
#define __CORNERS_H__

#include <stddef.h>
#include "image.h"

vi_HarrisCorners
vi_HarrisCorners_from_ImageIntensity(vi_ImageIntensity img, double t, size_t nms_rad);

#endif // __CORNERS_H__
