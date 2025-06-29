#ifndef __FLA_H__
#define __FLA_H__

#include <string.h>
#include <FLAME.h>

#define FLA_OBJ_INIT(_var, _r, _c, ...)\
    FLA_Obj _var;\
    do {\
        double* _var##_tmp = (double[]) { __VA_ARGS__ };\
        FLA_Obj_create(FLA_DOUBLE, (_r), (_c), 0, 0, &(_var));\
        memcpy(FLA_Obj_buffer_at_view(_var), _var##_tmp, (_r) * (_c) * sizeof(double));\
    } while(0)

#endif // __FLA_H__
