#include "corners.h"
#include <X11/Xlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <float.h>
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#include <FLAME.h>
#include "FLA.h"
#include "image.h"
#include "log.h"

size_t
reflect(size_t x, size_t dx, size_t max) {
    // assume baked in -1 offset to dx
    long long idx = (long long) x + (long long) dx - 1;
    if(idx < 0) return (size_t) (-idx);
    if (idx >= (long long) max) return max * 2 - x - 2;
    return (size_t) idx;
}

void 
convolve(FLA_Obj i, FLA_Obj j, FLA_Obj k) {
    double sum;
    size_t dx, dy, kx, ky, ix, iy;
    for(dy = 0; dy < FLA_OBJ_H(i); ++dy) for(dx = 0; dx < FLA_OBJ_W(i); ++dx) {
        for(ky = 0, sum = 0.0; ky < 3; ++ky) for(kx = 0; kx < 3; ++kx) {
            ix = reflect(dx, kx, FLA_OBJ_W(i));
            iy = reflect(dy, ky, FLA_OBJ_H(i));
            sum += FLA_OBJ_GET(i, ix, iy) * FLA_OBJ_GET(k, kx, ky);
        }
        FLA_OBJ_GET(j, dx, dy) = sum;
    }
}

double 
sum_kernel(FLA_Obj mat, size_t x, size_t y) {
    double sum = 0.0;
    for(size_t dy = y - 1, dx; dy <= y + 1; ++dy) for(dx = x - 1; dx <= x + 1; ++dx)
        sum += FLA_OBJ_GET(mat, dx, dy);
    return sum;
}

bool
local_maxima_test(FLA_Obj r, size_t x, size_t y, size_t s) {
    s /= 2;
    size_t dx, dy, xf, yf;
    dy = (y > s) ? y - s : 0;
    xf = ((x + s + 1) < FLA_OBJ_W(r)) ? (x + s + 1) : FLA_OBJ_W(r);
    yf = ((y + s + 1) < FLA_OBJ_H(r)) ? (y + s + 1) : FLA_OBJ_H(r);
    double rxy = FLA_OBJ_GET(r, x, y), rab;
    LOG(LOG_INFO, "[%zu, %zu], x: [%zu, %zu], y: [%zu, %zu]", x, y, (x > s) ? x - s : 0, xf, dy, yf);
    for(; dy < yf; ++dy) for(dx = (x > s) ? x - s : 0; dx < xf; ++dx) {
        if(x == dx && y == dy) continue;
        rab = FLA_OBJ_GET(r, dx, dy);
        if(rab > rxy) return false;
        if(rab == rxy && (dy < y || (dy == y && dx < x))) return false;
    }
    return true;
}

vi_HarrisCorners
vi_HarrisCorners_from_ImageIntensity(vi_ImageIntensity img, double t, size_t s) {
    vi_HarrisCorners corners = { .corners = NULL, .corner_count = 0 };
    FLA_Init();
    FLA_Obj mat = vi_ImageIntensity_to_Obj(img);

    // instantiate kernel
    FLA_Obj kx, ky;
    FLA_OBJ_INIT(kx, 3, 3, 
         3.0, 0.0,  -3.0,
        10.0, 0.0, -10.0,
         3.0, 0.0,  -3.0);
    FLA_OBJ_SCALE(kx, 1.0 / 32.0);
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, kx, &ky);
    FLA_Copy(kx, ky);    
    FLA_Transpose(ky);

    // declare gradients
    FLA_Obj jxx, jxy, jyy;
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &jxx);
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &jyy);
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &jxy);
    convolve(mat, jxx, kx);
    convolve(mat, jyy, ky);
    FLA_Obj_free(&mat);
    FLA_Obj_free(&kx);
    FLA_Obj_free(&ky);
    FLA_Copy(jxx, jxy);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jyy, jxy);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jxx, jxx);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jyy, jyy);
    // declare M structure matrices
    FLA_Obj mxx, mxy, myy;
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, jxx, &mxx);
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, jxy, &mxy);  
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, jyy, &myy);
    FLA_Set(FLA_ZERO, mxx);
    FLA_Set(FLA_ZERO, mxy);
    FLA_Set(FLA_ZERO, myy);
    
    // instantiate R matrix
    FLA_Obj r;
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mxx, &r);
    FLA_Copy(mxx, r);

    // calculate M values
    size_t dx, dy;
    for(dy = 1; dy < FLA_OBJ_H(mxx) - 1; ++dy) for(dx = 1; dx < FLA_OBJ_W(mxx) - 1; ++dx) {
        FLA_OBJ_GET(mxx, dx, dy) = sum_kernel(jxx, dx, dy);
        FLA_OBJ_GET(myy, dx, dy) = sum_kernel(jyy, dx, dy);
        FLA_OBJ_GET(mxy, dx, dy) = sum_kernel(jxy, dx, dy);
    }
    FLA_Obj_free(&jxx);
    FLA_Obj_free(&jxy);
    FLA_Obj_free(&jyy);

    // calculate R values
    for(dy = 1; dy < FLA_OBJ_H(r) - 1; ++dy) for(dx = 1; dx < FLA_OBJ_W(r) - 1; ++dx) {
        FLA_OBJ_GET(r, dx, dy) = \
            (FLA_OBJ_GET(mxx, dx, dy) * FLA_OBJ_GET(myy, dx, dy)) - \
            (FLA_OBJ_GET(mxy, dx, dy) * FLA_OBJ_GET(mxy, dx, dy)) - \
            (FLA_OBJ_GET(mxx, dx, dy) + FLA_OBJ_GET(myy, dx, dy)) * \
            (FLA_OBJ_GET(mxx, dx, dy) + FLA_OBJ_GET(myy, dx, dy)) * 0.05;
    }
    FLA_Obj_free(&mxx);
    FLA_Obj_free(&myy);
    FLA_Obj_free(&mxy);

    // normalize and filter R values
    double minima = DBL_MAX, maxima = DBL_MIN, v;
    for(dy = 0; dy < FLA_OBJ_H(r); ++dy) for(dx = 0; dx < FLA_OBJ_W(r); ++dx) {
        v = FLA_OBJ_GET(r, dx, dy);
        if(v < 0.0) continue;
        minima = (minima > v) ? v : minima;
        maxima = (maxima < v) ? v : maxima;
    }
    for(dy = 0; dy < FLA_OBJ_H(r); ++dy) for(dx = 0; dx < FLA_OBJ_W(r); ++dx) {
        v = FLA_OBJ_GET(r, dx, dy);
        v = (v - minima) / (maxima - minima);
        if(v > t && local_maxima_test(r, dx, dy, s)) {
            stbds_arrput(corners.corners, dx);
            stbds_arrput(corners.corners, dy);
            ++(corners.corner_count);
        }
    }
    FLA_Obj_free(&r);

    FLA_Finalize();

    return corners;
}
