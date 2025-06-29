#include "corners.h"
#include <FLAME.h>
#include "FLA.h"
#include "image.h"

vi_HarrisCorners
vi_HarrisCorners_from_ImageIntensity(vi_ImageIntensity img, float t) {
    vi_HarrisCorners corners = { .corners = NULL, .corner_count = 0 };
    FLA_Init();
    FLA_Obj mat = vi_ImageIntensity_to_Obj(img);
    FLA_OBJ_INIT(kernel, 3, 3, 
         3.f,  10.f,  3.f,
         0.f,   0.f,  0.f,
        -3.f, -10.f, -3.f);
    return corners;
}
