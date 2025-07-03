#include "mat.h"

#include <stddef.h>
#include <stdlib.h>
#include <FLAME.h>

#include "log.h"

struct __MAT_H__vi_Mat {
    FLA_Obj obj;
    size_t rows, cols;
    double data[];
};

vi_Mat
vi_Mat_init_zeros(size_t rows, size_t cols) {
    vi_Mat mat;
    mat = calloc(1, sizeof(*mat) + sizeof(double) * rows * cols);
    ASSERT(mat != NULL);
    mat->rows = rows;
    mat->cols = cols;
    FLA_Obj_create_without_buffer(FLA_DOUBLE, rows, cols, &(mat->obj));\
    FLA_Obj_attach_buffer(mat->data, cols, 1, &(mat->obj));\
    return mat;
}

vi_Mat
vi_Mat_init_zeros_like(vi_Mat mat) {
    return vi_Mat_init_zeros(mat->rows, mat->cols);
}

vi_Mat
vi_Mat_init_copy(vi_Mat mat) {
    vi_Mat copy = vi_Mat_init_zeros_like(mat);
    memcpy(copy->data, mat->data, sizeof(double) * mat->rows * mat->cols);
    return copy;
}

vi_Mat
vi_Mat_init_elem_count(size_t rows, size_t cols, size_t n, ...) {
    ASSERT(rows * cols == n);
    vi_Mat mat;
    mat = malloc(sizeof(*mat) + sizeof(double) * rows * cols);
    ASSERT(mat != NULL);
    mat->rows = rows;
    mat->cols = cols;
    FLA_Obj_create_without_buffer(FLA_DOUBLE, rows, cols, &(mat->obj));\
    FLA_Obj_attach_buffer(mat->data, cols, 1, &(mat->obj));\
    va_list args;
    va_start(args, n);
    vi_Mat_it(mat) *it_val = va_arg(args, double);
    return mat;
}

void
vi_Mat_free(vi_Mat mat) {
    FLA_Obj_free_without_buffer(&(mat->obj));
    free(mat);
}

void
vi_Mat_show(vi_Mat mat, char* name) {
    FLA_Obj_show(name, mat->obj, "%.2lf", "");
}

size_t
vi_Mat_rows(vi_Mat mat) {
    return mat->rows;
}

size_t
vi_Mat_cols(vi_Mat mat) {
    return mat->cols;
}

double*
vi_Mat_ref(vi_Mat mat, size_t row, size_t col) {
    ASSERT(row < mat->rows && col < mat->cols);
    return &(mat->data[col + row * mat->cols]);
}

double
vi_Mat_get(vi_Mat mat, size_t row, size_t col) {
    return *vi_Mat_ref(mat, row, col);
}

void
vi_Mat_set(vi_Mat mat, size_t row, size_t col, double val) {
    *vi_Mat_ref(mat, row, col) = val;
}

void
vi_Mat_transpose_square(vi_Mat mat) {
    ASSERT(mat->rows == mat->cols);
    double temp;
    vi_Mat_it(mat) {
        if(it_row <= it_col) continue;
        temp = vi_Mat_get(mat, it_col, it_row);
        vi_Mat_set(mat, it_col, it_row, *it_val);
        *it_val = temp;
    }
}

void
vi_Mat_scale(vi_Mat mat, double scalar) {
    FLA_Obj temp;
    FLA_Obj_create_constant(scalar, &temp);
    FLA_Scal(temp, mat->obj);
    FLA_Obj_free(&temp);
}

void
vi_Mat_scale_elem_wise(vi_Mat mat, vi_Mat scalar) {
    ASSERT(mat->rows == scalar->rows && mat->cols == scalar->cols);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, scalar->obj, mat->obj);
}
