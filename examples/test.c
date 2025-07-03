#include <FLAME.h>

int main(void) {
    dim_t m = 2, k = 3, n = 2;

    double A_buf[6] = {
        1.0, 4.0,
        2.0, 5.0,
        3.0, 6.0
    };

    double B_buf[6] = {
        7.0,  9.0, 11.0,
        8.0, 10.0, 12.0
    };

    FLA_Init();

    FLA_Obj A, B, C;
    FLA_Obj_create_without_buffer(FLA_DOUBLE, m, k, &A);
    FLA_Obj_attach_buffer(A_buf, 1, m, &A);
    FLA_Obj_create_without_buffer(FLA_DOUBLE, k, n, &B);
    FLA_Obj_attach_buffer(B_buf, 1, k, &B);
    FLA_Obj_create(FLA_DOUBLE, m, n, 0, 0, &C);

    FLA_Gemm(FLA_NO_TRANSPOSE, FLA_NO_TRANSPOSE, FLA_ONE, A, B, FLA_ZERO, C);
    FLA_Obj_show("C = A * B", C, "%9.2f", "");
    
    FLA_Obj_free_without_buffer(&A);
    FLA_Obj_free_without_buffer(&B);
    FLA_Obj_free(&C);

    FLA_Finalize();

    return 0;
}
