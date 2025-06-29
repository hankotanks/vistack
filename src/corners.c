#include "corners.h"
#include <stddef.h>
#include <float.h>
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#include <FLAME.h>
#include "FLA.h"
#include "image.h"
#include "log.h"

long 
reflect(long x, long max) {
    if(x < 0) return -x;
    if (x >= max) return 2 * max - x - 2;
    return x;
}

void 
convolve(FLA_Obj i, FLA_Obj j, FLA_Obj k) {
    long rc, cc, ld;
    rc = (long) FLA_Obj_length(i);
    cc = (long) FLA_Obj_width(i); 
    ld = (long) FLA_Obj_col_stride(i);

    double* i_buf,* k_buf,* j_buf;
    i_buf = (double*) FLA_Obj_buffer_at_view(i);
    j_buf = (double*) FLA_Obj_buffer_at_view(j);
    k_buf = (double*) FLA_Obj_buffer_at_view(k);

    double sum;
    long dx, dy, kx, ky, ix, iy;
    for(dy = 0; dy < rc; ++dy) for(dx = 0; dx < cc; ++dx) {
        for(ky = -1, sum = 0.0; ky <= 1; ++ky) for(kx = -1; kx <= 1; ++kx) {
            ix = reflect(dx + kx, cc);
            iy = reflect(dy + ky, rc);
            sum += i_buf[(size_t) (ix * ld + iy)] * k_buf[(size_t) (ky * 3 + kx + 4)];
        }
        j_buf[dx * ld + dy] = sum;
    }
}

double 
sum_kernel(FLA_Obj mat, size_t x, size_t y) {
    double* buf, sum = 0.0;
    buf = (double*) FLA_Obj_buffer_at_view(mat);
    size_t dy, dx, ld = (size_t) FLA_Obj_col_stride(mat);
    for(dy = y - 1; dy <= y + 1; ++dy) for(dx = x - 1; dx <= x + 1; ++dx)
        sum += buf[dx * ld + dy];
    return sum;
}

unsigned int
eig_vals(double a, double b, double c, double d, double* fst, double* snd) {
    double trace = a + d;
    double det = a * d - b * c;
    double disc = trace * trace - 4 * det;
    if(disc < 0) return 1;
    double root = sqrt(disc);
    *fst = 0.5 * (trace + root);
    *snd = 0.5 * (trace - root);
    return 0;
}

vi_HarrisCorners
vi_HarrisCorners_from_ImageIntensity(vi_ImageIntensity img, double t) {
    vi_HarrisCorners corners = { .corners = NULL, .corner_count = 0 };
    FLA_Init();
    FLA_Obj mat = vi_ImageIntensity_to_Obj(img);

    // instantiate kernel
    FLA_Obj temp;
    FLA_Obj_create_constant(1.0 / 32.0, &temp);
    FLA_OBJ_INIT(kx, 3, 3, 
         3.0, 0.0,  -3.0,
        10.0, 0.0, -10.0,
         3.0, 0.0,  -3.0);
    FLA_Scal(temp, kx);
    FLA_Obj_free(&temp);
    FLA_Obj ky;
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, kx, &ky);
    FLA_Copy(kx, ky);    
    FLA_Transpose(ky);

    // declare gradients
    FLA_Obj jxx, jyy;
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &jxx);
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &jyy);
    convolve(mat, jxx, kx);
    convolve(mat, jyy, ky);
#if 0
    vi_ImageIntensity_show(vi_ImageIntensity_from_Obj(jxx), NULL, 0);
    vi_ImageIntensity_show(vi_ImageIntensity_from_Obj(jyy), NULL, 0);
#endif
    FLA_Obj_free(&kx);
    FLA_Obj_free(&ky);
    FLA_Obj jxy, jyx;
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &jxy);
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &jyx);
    FLA_Copy(jxx, jxy);
    FLA_Copy(jyy, jyx);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jyy, jxy);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jxx, jyx);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jxx, jxx);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jyy, jyy);
    // declare M matrices
    FLA_Obj mxx, myy, mxy, myx;
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &mxx);
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &myy);
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &mxy);
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &myx);
    double* mxx_buf,* myy_buf,* mxy_buf,* myx_buf;
    mxx_buf = (double*) FLA_Obj_buffer_at_view(mxx);
    myy_buf = (double*) FLA_Obj_buffer_at_view(myy);
    mxy_buf = (double*) FLA_Obj_buffer_at_view(mxy);
    myx_buf = (double*) FLA_Obj_buffer_at_view(myx);
    size_t rc, cc, ld;
    rc = (size_t) FLA_Obj_length(mat);
    cc = (size_t) FLA_Obj_width(mat); 
    ld = (size_t) FLA_Obj_col_stride(mat);
    size_t dx, dy, id;

    // zero the edges of the M matrices
    for(dx = 0; dx < cc; ++dx) {
        mxx_buf[dx * ld] = 0.0;
        mxx_buf[dx * ld + rc - 1] = 0.0;
    }
    for(dy = 1; dy < rc - 1; ++dy) {
        mxx_buf[dy] = 0.0;
        mxx_buf[(cc - 1) * ld + dy] = 0.0;
    }
    FLA_Copy(mxx, myy);  
    FLA_Copy(mxx, mxy);  
    FLA_Copy(mxx, myx);  
    FLA_Obj r;
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, mat, &r);
    FLA_Copy(mxx, r);
    FLA_Obj_free(&mat);

    // calculate M values
    for(dy = 1; dy < rc - 1; ++dy) for(dx = 1; dx < cc - 1; ++dx) {
        id = dx * ld + dy;
        mxx_buf[id] = sum_kernel(jxx, dx, dy);
        myy_buf[id] = sum_kernel(jyy, dx, dy);
        mxy_buf[id] = sum_kernel(jxy, dx, dy);
        myx_buf[id] = sum_kernel(jyx, dx, dy);
    }
    FLA_Obj_free(&jxx);
    FLA_Obj_free(&jyy);
    FLA_Obj_free(&jxy);
    FLA_Obj_free(&jyx);

    // calculate R values
    double* r_buf = (double*) FLA_Obj_buffer_at_view(r), eig_fst, eig_snd;
    for(dy = 1; dy < rc - 1; ++dy) for(dx = 1; dx < cc - 1; ++dx) {
        id = dx * ld + dy;
        eig_vals(mxx_buf[id], mxy_buf[id], myx_buf[id], myy_buf[id], &eig_fst, &eig_snd);
        r_buf[id] = eig_fst * eig_snd - (eig_fst + eig_snd) * (eig_fst + eig_snd) * 0.05;
    }
    FLA_Obj_free(&mxx);
    FLA_Obj_free(&myy);
    FLA_Obj_free(&mxy);
    FLA_Obj_free(&myx);

    // normalize and filter R values
    double minima = DBL_MAX, maxima = DBL_MIN, v;
    for(dy = 0; dy < rc; ++dy) for(dx = 0; dx < cc; ++dx) {
        v = r_buf[dx * ld + dy];
        minima = (minima > v) ? v : minima;
        maxima = (maxima < v) ? v : maxima;
    }
    for(dy = 0; dy < rc; ++dy) for(dx = 0; dx < cc; ++dx) {
        v = r_buf[dx * ld + dy];
        v = (v - minima) / (maxima - minima);
        if(v > t) {
            stbds_arrput(corners.corners, dx);
            stbds_arrput(corners.corners, dy);
            ++(corners.corner_count);
        }
    }
    FLA_Obj_free(&r);

    FLA_Finalize();

    return corners;
}
