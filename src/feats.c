#include "feats.h"
#include <stddef.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#include <glenv.h>
#include <FLAME.h>
#include "FLA.h"
#include "log.h"
#include "image.h"
#include "plot.h"

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

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

vi_HarrisCorners
vi_HarrisCorners_compute(vi_ImageIntensity img, double t, size_t s) {
    vi_HarrisCorners corners = { .corners = NULL, .count = 0, 
        .rows = vi_ImageIntensity_rows(img),
        .cols = vi_ImageIntensity_cols(img) 
    };

    // initialize libflame environment
    FLA_Init();
    FLA_Obj mat = vi_ImageIntensity_to_Mat(img);

    // instantiate kernel
    FLA_Obj kx, ky;
    FLA_OBJ_INIT(kx, 3, 3, 
         3.0, 0.0,  -3.0,
        10.0, 0.0, -10.0,
         3.0, 0.0,  -3.0);
    FLA_OBJ_SCALE(kx, 1.0 / 32.0);
    FLA_OBJ_DUPLICATE(kx, ky);
    FLA_Transpose(ky);

    // declare gradients
    FLA_Obj jxx, jxy, jyy;
    FLA_OBJ_LIKE(mat, jxx);
    FLA_OBJ_LIKE(mat, jxy);
    FLA_OBJ_LIKE(mat, jyy);
    convolve(mat, jxx, kx);
    convolve(mat, jyy, ky);
    FLA_Obj_free(&mat);
    FLA_Obj_free(&kx);
    FLA_Obj_free(&ky);
    FLA_Copy(jxx, jxy);
    // copy gradients to vi_HarrisCorners
    size_t buf_size = sizeof(double) * FLA_OBJ_W(jxx) * FLA_OBJ_H(jxx);
    corners.jx = malloc(buf_size);
    corners.jy = malloc(buf_size);
    ASSERT_LOG(corners.jx != NULL && corners.jy != NULL, "Failed to allocate vi_HarrisCorners.");
    memcpy(corners.jx, FLA_Obj_buffer_at_view(jxx), buf_size);
    memcpy(corners.jy, FLA_Obj_buffer_at_view(jyy), buf_size);
    // compute jxx, jxy, jyy from jx, jy
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jyy, jxy);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jxx, jxx);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jyy, jyy);
    // declare M structure matrices
    FLA_Obj mxx, mxy, myy;
    FLA_OBJ_LIKE(jxx, mxx);
    FLA_OBJ_LIKE(jxy, mxy);
    FLA_OBJ_LIKE(jyy, myy);
    
    // instantiate R matrix
    FLA_Obj r;
    FLA_OBJ_DUPLICATE(mxx, r);

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
            ++(corners.count);
        }
    }
    FLA_Obj_free(&r);
    // deinit libflame environment
    FLA_Finalize();

    return corners;
}

void
vi_HarrisCorners_free(vi_HarrisCorners corners) {
    free(corners.jx);
    free(corners.jy);
    stbds_arrfree(corners.corners);
}

//
//
//
//
//
//
//

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
    free(plotter_data->corners);
    glDeleteBuffers(1, &(plotter_data->VBO));
    glDeleteVertexArrays(1, &(plotter_data->VAO));
    glDeleteProgram(plotter_data->shader);
}

vi_Plotter
vi_HarrisCorners_plotter(vi_HarrisCorners corners) {
    vi_Plotter layer = vi_Plotter_init(
        sizeof(struct plotter_data),
        harris_corners_plotter_config,
        harris_corners_plotter_render,
        harris_corners_plotter_deinit);
    struct plotter_data* plotter_data = ((struct plotter_data*) vi_Plotter_data(layer));
    plotter_data->corner_count = corners.count;
    plotter_data->corners = malloc(sizeof(float) * 2 * (size_t) plotter_data->corner_count);
    ASSERT_LOG(plotter_data->corners != NULL, "Failed to allocate space for vi_Plotter for Harris corners.");
    for (size_t i = 0; i < (size_t) plotter_data->corner_count; ++i) {
        plotter_data->corners[i * 2] = 2.f * ((float) corners.corners[i * 2] / \
            (float) corners.cols) - 1.f;
        plotter_data->corners[i * 2 + 1] = 1.f - 2.f * \
            ((float) corners.corners[i * 2 + 1] / (float) corners.rows);
    }
    return layer;
}

struct __FEATS_H__vi_ImageDescriptor {
    size_t count;
    unsigned char desc[];
};

void
desc_compute_patch(FLA_Obj ang, FLA_Obj mag, size_t x, size_t y, double* hist) {
    size_t i, j, k;
    for(i = 0; i < 4; ++i) for(j = 0; j < 4; ++j) {
        k = (size_t) (FLA_OBJ_GET(ang, x + i, y + j) / 45.0) % 8;
        hist[k] += FLA_OBJ_GET(mag, x + i, y + j);
    } 
}

void
desc_compute(FLA_Obj ang, FLA_Obj mag, size_t x, size_t y, double desc[128]) {
    // does not perform bounds checking
    for(size_t i = 0, j, k = 0; i < 4; ++i) for(j = 0; j < 4; ++j) 
        desc_compute_patch(ang, mag, x + i * 4, y + j * 4, &(desc[(k += 8) - 8]));
}

void
desc_quantize(double in[128], unsigned char out[128]) {
    double norm = 0.0;
    size_t i;
    for(i = 0; i < 128; ++i) norm += in[i] * in[i];
    norm = sqrt(norm);
    if(norm > 1e-8) for(i = 0; i < 128; ++i) 
        out[i] = (unsigned char) (in[i] / norm * 255.0 + 0.5);
    else memset(out, 0, 128);
}

vi_ImageDescriptor
vi_ImageDescriptor_from_HarrisCorners(vi_HarrisCorners in, vi_HarrisCorners* out) {
    vi_ImageDescriptor desc;
    desc = calloc(1, sizeof(*desc) + in.count * 128);
    ASSERT_LOG(desc != NULL, "Failed to allocate vi_ImageDescriptor.");

    // compute angle and magnitude
    FLA_Init();
    FLA_Obj jx, jy;
    FLA_OBJ_INIT_FROM_BUFFER(jx, in.rows, in.cols, in.jx);
    FLA_OBJ_INIT_FROM_BUFFER(jy, in.rows, in.cols, in.jy);
    FLA_Obj ang, mag;
    FLA_OBJ_DUPLICATE(jx, ang);
    FLA_OBJ_DUPLICATE(jy, mag);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jx, mag);
    FLA_Scal_elemwise(FLA_NO_TRANSPOSE, jy, ang);
    // compute ang and final mag in tandem
    size_t x, y;
    for(x = 0; x < in.cols; ++x) for(y = 0; y < in.rows; ++y) {
        FLA_OBJ_GET(mag, x, y) += FLA_OBJ_GET(ang, x, y);
        FLA_OBJ_GET(mag, x, y) = sqrt(FLA_OBJ_GET(mag, x, y));
        FLA_OBJ_GET(ang, x, y) = atan2(FLA_OBJ_GET(jy, x, y), FLA_OBJ_GET(jx, x, y));
        FLA_OBJ_GET(ang, x, y) += (FLA_OBJ_GET(ang, x, y) < 0.0) ? M_PI * 2.0 : 0.0;
        FLA_OBJ_GET(ang, x, y) *= 180.0 / M_PI;
    }
    FLA_Obj_free_without_buffer(&jx);
    FLA_Obj_free_without_buffer(&jy);

    size_t temp[in.count];
    double curr[128];

    // build descriptors for each corner point
    size_t i;
    for(i = 0; i < in.count; ++i) {
        memset(curr, 0, sizeof(double) * 128);
        x = in.corners[i * 2];
        y = in.corners[i * 2 + 1];
        if(x < 8 || y < 8 || (x + 8) >= in.cols || (y + 8) >= in.rows) continue;
        temp[desc->count] = i;
        desc_compute(ang, mag, x, y, curr);
        desc_quantize(curr, &(desc->desc[((desc->count)++) * 128]));
    }
    FLA_Obj_free(&ang);
    FLA_Obj_free(&mag);

    // construct output corners
    out->corners = NULL;
    out->count = desc->count;
    out->rows = in.rows;
    out->cols = in.cols;
    size_t buf = sizeof(double) * in.rows * in.cols;
    out->jx = malloc(buf);
    out->jy = malloc(buf);
    ASSERT_LOG(out->jx != NULL && out->jy != NULL, "Failed to allocate vi_HarrisCorners.");
    memcpy(in.jx, out->jx, buf);
    memcpy(in.jy, out->jy, buf);
    for(i = 0; i < desc->count; ++i) {
        stbds_arrput(out->corners, in.corners[temp[i] * 2]);
        stbds_arrput(out->corners, in.corners[temp[i] * 2 + 1]);
    }

    // deinit libflame environment
    FLA_Finalize();

    return desc;
}

void
vi_ImageDescriptor_free(vi_ImageDescriptor desc) {
    free(desc);
}

void
vi_ImageDescriptor_dump(vi_ImageDescriptor desc) {
    for(size_t i = 0, j, k; i < desc->count; ++i) {
        printf("[%zu] descriptor\n", i);
        for(j = 0; j < 8; ++j) {
            for(k = 0; k < 16; ++k) printf("%02d ", (int) desc->desc[i * 128 + j * 16 + k]);
            printf("\n");
        }
        printf("\n");
    }
}

struct __FEATS_H__vi_ImageMatches {
    size_t count, matches[];
};

vi_ImageMatches
vi_ImageMatches_compute(vi_ImageDescriptor a, vi_ImageDescriptor b, double threshold) {
    vi_ImageMatches matches;
    matches = malloc(sizeof(*matches) + sizeof(size_t) * MIN(a->count, b->count) * 2);

    double s, t, fst, snd;
    for(size_t i = 0, j, k, m; i < a->count; ++i) {
        fst = DBL_MAX;
        snd = DBL_MAX;
        for(j = 0; j < b->count; ++j) {
            s = 0.0;
            for(k = 0; k < 128; ++k) {
                t = (double) a->desc[i * 128 + k] - (double) b->desc[j * 128 + k];
                s += t * t;
            }

            if(s < fst) {
                snd = fst;
                fst = s;
                m = j;
            } else if(s < snd) snd = s;
        }

        if(s < threshold * threshold * snd) {
            matches->matches[matches->count * 2] = i;
            matches->matches[matches->count * 2 + 1] = m;
            ++(matches->count);
        }
    }

    return matches;
}

void
vi_ImageMatches_free(vi_ImageMatches matches) {
    free(matches);
}

void
vi_ImageMatches_dump(vi_ImageMatches matches, vi_HarrisCorners a,vi_HarrisCorners b) {
    for(size_t i = 0, ax, ay, bx, by; i < matches->count; ++i) {
        ax = a.corners[matches->matches[i * 2] * 2];
        ay = a.corners[matches->matches[i * 2] * 2 + 1];
        bx = b.corners[matches->matches[i * 2 + 1] * 2];
        by = b.corners[matches->matches[i * 2 + 1] * 2 + 1];
        printf("[%zu, %zu] <-> [%zu, %zu]\n", ax, ay, bx, by);
    }
}
