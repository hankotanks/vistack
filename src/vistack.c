#include "vistack.h"

#include <FLAME.h>

void vi_start() {
    FLA_Init();
}

void vi_finish() {
    FLA_Finalize();
}
