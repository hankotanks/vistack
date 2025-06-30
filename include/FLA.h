#ifndef __FLA_H__
#define __FLA_H__

#include <string.h>
#include <FLAME.h>

#define FLA_OBJ_INIT(_var, _r, _c, ...)\
    do {\
        double* _var##_tmp = (double[]) { __VA_ARGS__ };\
        FLA_Obj_create(FLA_DOUBLE, (_r), (_c), 0, 0, &(_var));\
        memcpy(FLA_Obj_buffer_at_view(_var), _var##_tmp, (_r) * (_c) * sizeof(double));\
    } while(0)

#define FLA_OBJ_GET(_obj, _x, _y) (((double*) FLA_Obj_buffer_at_view(_obj))[(_x) + (_y) * (size_t) FLA_Obj_col_stride(_obj)])
#define FLA_OBJ_W(_obj) ((size_t) FLA_Obj_width(_obj))
#define FLA_OBJ_H(_obj) ((size_t) FLA_Obj_length(_obj))
#define FLA_OBJ_SCALE(_obj, _s) {\
    FLA_Obj _obj##_temp;\
    FLA_Obj_create_constant((_s), &(_obj##_temp));\
    FLA_Scal(_obj##_temp, _obj);\
    FLA_Obj_free(&(_obj##_temp));\
}

#endif // __FLA_H__
