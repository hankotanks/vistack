#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stddef.h>
#include <FLAME.h>

typedef struct __IMAGE_H__vi_ImageRaw* vi_ImageRaw;
typedef struct __IMAGE_H__vi_ImageIntensity* vi_ImageIntensity;

vi_ImageRaw 
vi_ImageRaw_load(const char* path);
vi_ImageIntensity 
vi_ImageIntensity_from_ImageRaw(const vi_ImageRaw img);

const float* 
vi_ImageIntensity_buffer(vi_ImageIntensity img);

void 
vi_ImageIntensity_show(vi_ImageIntensity img);

unsigned long
vi_ImageIntensity_rows(vi_ImageIntensity img);
unsigned long
vi_ImageIntensity_cols(vi_ImageIntensity img);

FLA_Obj
vi_ImageIntensity_to_Obj(vi_ImageIntensity img);

#endif // __IMAGE_H__

