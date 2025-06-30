#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stddef.h>
#include <FLAME.h>

typedef struct __IMAGE_H__vi_ImageRaw* vi_ImageRaw;
typedef struct __IMAGE_H__vi_ImageIntensity* vi_ImageIntensity;

typedef struct {
    size_t* corners, corner_count;
} vi_HarrisCorners;

vi_ImageRaw 
vi_ImageRaw_load(const char* path);
vi_ImageIntensity 
vi_ImageIntensity_from_ImageRaw(const vi_ImageRaw img);

const double* 
vi_ImageIntensity_buffer(vi_ImageIntensity img);

void 
vi_ImageIntensity_show(vi_ImageIntensity img, size_t* corners, size_t count);

unsigned long
vi_ImageIntensity_rows(vi_ImageIntensity img);
unsigned long
vi_ImageIntensity_cols(vi_ImageIntensity img);

FLA_Obj
vi_ImageIntensity_to_Obj(vi_ImageIntensity img);

vi_ImageIntensity
vi_ImageIntensity_from_Obj(FLA_Obj mat);

#define VI_IMAGE_INTENSITY_SHOW_OBJ(_obj) {\
    vi_ImageIntensity _obj##_temp = vi_ImageIntensity_from_Obj(_obj);\
    vi_ImageIntensity_show(_obj##_temp, NULL, 0);\
    free(_obj##_temp);\
}

#endif // __IMAGE_H__

