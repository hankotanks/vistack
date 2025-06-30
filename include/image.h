#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stddef.h>
#include <FLAME.h>
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

FLA_Obj
vi_ImageIntensity_to_Mat(vi_ImageIntensity img);
vi_ImageIntensity
vi_Mat_to_ImageIntensity(FLA_Obj mat);

vi_Plotter
vi_ImageIntensity_plotter(vi_ImageIntensity img);

#endif // __IMAGE_H__

