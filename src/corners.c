#include "corners.h"
#include <FLAME.h>
#include "FLA.h"
#include "image.h"

long reflect(long x, long max) {
    if(x < 0) return -x;
    if (x >= max) return 2 * max - x - 2;
    return x;
}

void convolve_reflect(FLA_Obj i, FLA_Obj j, FLA_Obj k) {
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, i, &j);
    long N, M;
    N = (long) FLA_Obj_length(i);
    M = (long) FLA_Obj_width(i);

    double* i_buf,* k_buf,* j_buf;
    i_buf = (double*) FLA_Obj_buffer_at_view(i);
    j_buf = (double*) FLA_Obj_buffer_at_view(j);
    k_buf = (double*) FLA_Obj_buffer_at_view(k);

    double sum;
    long dx, dy, kx, ky, ix, iy;
    for(dy = 0; dy < N; ++dy) for(dx = 0, sum = 0.0; dx < M; ++dx) {
        for(ky = -1; ky <= 1; ++ky) for(kx = -1; kx <= 1; ++kx) {
            iy = reflect(dy + ky, N);
            ix = reflect(dx + kx, M);
            sum += i_buf[ix + iy * M] * k_buf[(ky + 1) * 3 + (kx + 1)];
        }
        j_buf[dx + dy * M] = sum;
    }
}

vi_HarrisCorners
vi_HarrisCorners_from_ImageIntensity(vi_ImageIntensity img, double t) {
    vi_HarrisCorners corners = { .corners = NULL, .corner_count = 0 };
    FLA_Init();
    FLA_Obj mat = vi_ImageIntensity_to_Obj(img);

    // instantiate kernel
    FLA_Obj temp;
    FLA_Obj_create_constant(1.0 / 32.0, &temp);
    FLA_OBJ_INIT(k_x, 3, 3, 
         3.0,  10.0,  3.0,
         0.0,   0.0,  0.0,
        -3.0, -10.0, -3.0);
    FLA_Scal(temp, k_x);
    FLA_Obj_free(&temp);
    
    FLA_Obj k_y;
    FLA_Obj_create_conf_to(FLA_NO_TRANSPOSE, k_x, &k_y);
    FLA_Copy(k_x, k_y);    
    FLA_Transpose(k_y);       

    FLA_Obj j_x, j_y;
    FLA_Obj_create(FLA_DOUBLE, FLA_Obj_length(mat), FLA_Obj_width(mat), 0, 0, &j_x);
    FLA_Obj_create(FLA_DOUBLE, FLA_Obj_length(mat), FLA_Obj_width(mat), 0, 0, &j_y);
    convolve_reflect(mat, j_x, k_x);
    convolve_reflect(mat, j_y, k_y);
    
    FLA_Obj_free(&j_x);
    FLA_Obj_free(&j_y);
    FLA_Obj_free(&k_x);
    FLA_Obj_free(&k_y);
    FLA_Obj_free(&mat);
    FLA_Finalize();
    return corners;
}
