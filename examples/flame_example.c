#include <FLAME.h>

int main(void) {
    double* buffer;
    int m, rs, cs;
    FLA_Obj A;
    FLA_Init();
    get_matrix_info(&buffer, &m, &rs, &cs);
    FLA_Obj_create_without_buffer(FLA_DOUBLE, m, m, &A);
    FLA_Obj_attach_buffer(buffer, rs, cs, &A);
    FLA_Chol(FLA_LOWER_TRIANGULAR, A);
    FLA_Obj_free_without_buffer(&A);
    free_matrix( buffer );
    FLA_Finalize();
    return 0;
}