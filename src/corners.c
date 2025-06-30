#include "corners.h"
#include <stddef.h>
#include <stdbool.h>
#include <float.h>
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#include <glenv.h>
#include <FLAME.h>
#include "FLA.h"
#include "log.h"
#include "image.h"
#include "plot.h"


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
    for(; dy < yf; ++dy) for(dx = (x > s) ? x - s : 0; dx < xf; ++dx) {
        if(x == dx && y == dy) continue;
        rab = FLA_OBJ_GET(r, dx, dy);
        if(rab > rxy) return false;
        if(rab == rxy && (dy < y || (dy == y && dx < x))) return false;
    }
    return true;
}

const size_t*
vi_harris_corners_compute(vi_ImageIntensity img, double t, size_t s) {
    FLA_Init();
    FLA_Obj mat = vi_ImageIntensity_to_Mat(img);

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

    size_t* corners = NULL;
    for(dy = 0; dy < FLA_OBJ_H(r); ++dy) for(dx = 0; dx < FLA_OBJ_W(r); ++dx) {
        v = FLA_OBJ_GET(r, dx, dy);
        v = (v - minima) / (maxima - minima);
        if(v > t && local_maxima_test(r, dx, dy, s)) {
            stbds_arrput(corners, dx);
            stbds_arrput(corners, dy);
        }
    }
    FLA_Obj_free(&r);

    FLA_Finalize();

    return corners;
}

struct plotter_data {
    GLuint shader;
    size_t corner_count;
    float* corners;
    GLuint VAO, VBO, EBO;
};

void
harris_corners_plotter_config(void* data) {
    struct plotter_data* plotter_data = (struct plotter_data*) data;
    // shaders
    const char* vert_src =
        "#version 330 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "    gl_PointSize = 5.0;\n"  // adjust point size as needed
        "}\n";
    const char* frag_src =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // red points\n"
        "}\n";
    // vertex shader
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vert_src, NULL);
    glCompileShader(vert);
    GLint success;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    ASSERT_LOG(success == GL_TRUE, "Failed to compile pts vertex shader.");
    // fragment shader
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_src, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    ASSERT_LOG(success == GL_TRUE, "Failed to compile pts fragment shader.");
    // create shader program and link
    plotter_data->shader = glCreateProgram();
    glAttachShader(plotter_data->shader, vert);
    glAttachShader(plotter_data->shader, frag);
    glLinkProgram(plotter_data->shader);
    glGetProgramiv(plotter_data->shader, GL_LINK_STATUS, &success);
    ASSERT_LOG(success == GL_TRUE, "Failed to link pts shader program.");
    glDeleteShader(vert);
    glDeleteShader(frag);
    // buffers
    glGenVertexArrays(1, &(plotter_data->VAO));
    glGenBuffers(1, &(plotter_data->VBO));
    glBindVertexArray(plotter_data->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, plotter_data->VBO);
    glBufferData(GL_ARRAY_BUFFER, 
        (GLsizeiptr) (sizeof(float) * plotter_data->corner_count * 2), 
        plotter_data->corners, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
}

void
harris_corners_plotter_render(const void* data) {
    const struct plotter_data* plotter_data = (const struct plotter_data*) data;
    glEnable(GL_PROGRAM_POINT_SIZE);
    glUseProgram(plotter_data->shader);
    glBindVertexArray(plotter_data->VAO);
    glDrawArrays(GL_POINTS, 0, (GLsizei) plotter_data->corner_count);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

void
harris_corners_plotter_deinit(void* data) {
    struct plotter_data* plotter_data = (struct plotter_data*) data;
    stbds_arrfree(plotter_data->corners);
    glDeleteBuffers(1, &(plotter_data->VBO));
    glDeleteVertexArrays(1, &(plotter_data->VAO));
    glDeleteProgram(plotter_data->shader);
}

vi_Plotter
vi_harris_corners_plotter(vi_ImageIntensity img, const size_t* corners) {
    vi_Plotter layer = vi_Plotter_init(
        sizeof(struct plotter_data),
        harris_corners_plotter_config,
        harris_corners_plotter_render,
        harris_corners_plotter_deinit);
    struct plotter_data* plotter_data = ((struct plotter_data*) vi_Plotter_data(layer));
    long corner_count = stbds_arrlen(corners);
    ASSERT_LOG(corner_count >= 0, "Failed to construct vi_Plotter for Harris corners.");
    plotter_data->corner_count = (size_t) corner_count;
    plotter_data->corners = malloc(sizeof(float) * 2 * (size_t) plotter_data->corner_count);
    ASSERT_LOG(plotter_data->corners != NULL, "Failed to allocate space for vi_Plotter for Harris corners.");
    for (size_t i = 0; i < (size_t) plotter_data->corner_count; ++i) {
        plotter_data->corners[i * 2] = 2.f * ((float) corners[i * 2] / \
            (float) vi_ImageIntensity_cols(img)) - 1.f;
        plotter_data->corners[i * 2 + 1] = 1.f - 2.f * \
            ((float) corners[i * 2 + 1] / (float) vi_ImageIntensity_rows(img));
    }
    return layer;
}
