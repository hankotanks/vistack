#ifndef __MAT_H__
#define __MAT_H__

#include <stddef.h>

typedef struct __MAT_H__vi_Mat* vi_Mat;

vi_Mat
vi_Mat_init_zeros(size_t rows, size_t cols);
vi_Mat
vi_Mat_init_zeros_like(vi_Mat mat);
vi_Mat
vi_Mat_init_copy(vi_Mat mat);
vi_Mat
vi_Mat_init_elem_count(size_t rows, size_t cols, size_t n, ...);
void
vi_Mat_free(vi_Mat mat);

//
//

#define vi_Mat_init(rows, cols, ...) vi_Mat_init_elem_count(rows, cols, rows * cols, __VA_ARGS__)

//
//

void
vi_Mat_show(vi_Mat mat, char* name);

//
//

size_t
vi_Mat_rows(vi_Mat mat);
size_t
vi_Mat_cols(vi_Mat mat);

double*
vi_Mat_ref(vi_Mat mat, size_t row, size_t col);
double
vi_Mat_get(vi_Mat mat, size_t row, size_t col);
void
vi_Mat_set(vi_Mat mat, size_t row, size_t col, double val);

//
//

#define vi_Mat_it_with_prefix(mat) \
    double* mat##_val = vi_Mat_ref(mat, 0, 0); \
    for(size_t mat##_row = 0, mat##_col; mat##_row < vi_Mat_rows(mat); ++mat##_row) \
    for(mat##_col = 0; mat##_col < vi_Mat_cols(mat); ++(mat##_col), ++(mat##_val))
#define vi_Mat_it(mat) \
    double* it_val = vi_Mat_ref(mat, 0, 0); \
    for(size_t it_row = 0, it_col; it_row < vi_Mat_rows(mat); ++it_row) \
    for(it_col = 0; it_col < vi_Mat_cols(mat); ++(it_col), ++(it_val))
//
//

void
vi_Mat_transpose_square(vi_Mat mat);
void
vi_Mat_scale(vi_Mat mat, double scalar);
void
vi_Mat_scale_elem_wise(vi_Mat mat, vi_Mat scalar);

#endif // __MAT_H__
