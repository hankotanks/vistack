#ifndef __CORNERS_H__
#define __CORNERS_H__

#include <stddef.h>
#include "plot.h"
#include "image.h"

const size_t*
vi_harris_corners_compute(vi_ImageIntensity img, double t, size_t s);

vi_Plotter
vi_harris_corners_plotter(vi_ImageIntensity img, const size_t* corners);

#endif // __CORNERS_H__
