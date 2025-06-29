#ifndef __FLA_H__
#define __FLA_H__

#include <string.h>
#include <FLAME.h>

typedef struct {
    dim_t r, c;
    float* buf;
    FLA_Obj obj;
} vi_Matf;

#if 0
#define VI_MATF(_var, _r, _c, ...)\
    float* _var##_data = (float[]) { __VA_ARGS__ };\
    vi_Matf _var;\
    do {\
        _var.r = (_r);\
        _var.c = (_c);\
        _var.buf = (_var##_data);\
        FLA_Obj_create_without_buffer(FLA_FLOAT, (_r), (_c), &(_var.obj));\
        FLA_Obj_attach_buffer((_var##_data), 1, (_c), &(_var.obj));\
    } while(0)
#endif

#define FLA_OBJ_INIT(_var, _r, _c, ...)\
    FLA_Obj _var;\
    do {\
        float* _var##_tmp = (float[]) { __VA_ARGS__ };\
        FLA_Obj_create(FLA_FLOAT, (_r), (_c), 0, 0, &(_var));\
        memcpy(FLA_Obj_buffer_at_view(_var), _var##_tmp, (_r) * (_c) * sizeof(float));\
    } while(0)

#endif // __FLA_H__
