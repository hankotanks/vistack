#include "feature.h"

#include <float.h>
#include <glenv.h>
#include <stdbool.h>
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#include "log.h"
#include "plot.h"
#include "mat.h"
#include "image.h"

bool
out_of_bounds(vi_Mat in, size_t offset, size_t row, size_t col) {
    size_t rc, cc;
    rc = vi_Mat_rows(in);
    cc = vi_Mat_cols(in);
    return (bool)
        (row < offset) || (row >= rc - offset) ||
        (col < offset) || (col >= cc - offset);
}

vi_Mat
convolve(vi_Mat mat, vi_Mat kernel, size_t offset) {
    vi_Mat out = vi_Mat_init_zeros_like(mat);
    double sum;
    vi_Mat_it_with_prefix(out) {
        if(out_of_bounds(mat, offset, out_row, out_col)) continue;
        sum = 0.0;
        vi_Mat_it_with_prefix(kernel) sum += vi_Mat_get(mat, 
            out_row + kernel_row - offset, 
            out_col + kernel_col - offset
        ) * (*kernel_val);
        *out_val = sum;
    }
    return out;
}

vi_ImageData
vi_ImageData_init(vi_ImageIntensity image, vi_Mat kernel) {
    ASSERT(vi_Mat_rows(kernel) == vi_Mat_cols(kernel));
    vi_ImageData data = { 
        .image = vi_ImageIntensity_to_Mat(image),
        .offset = vi_Mat_rows(kernel) / 2
    };
    // create image gradients
    data.dx = convolve(data.image, kernel, data.offset);
    vi_Mat_show(kernel, "kx");
    vi_Mat_transpose_square(kernel);
    vi_Mat_show(kernel, "ky");
    data.dy = convolve(data.image, kernel, data.offset);
    vi_Mat_transpose_square(kernel);
    return data;
}

void
vi_ImageData_free(vi_ImageData data) {
    vi_Mat_free(data.image);
    vi_Mat_free(data.dx);
    vi_Mat_free(data.dy);
}

void vi_HarrisDetector(vi_Mat r, vi_Mat m[static 3]) {
    double mxx, mxy, myy;
    vi_Mat_it(r) {
        mxx = vi_Mat_get(m[0], it_row, it_col);
        mxy = vi_Mat_get(m[1], it_row, it_col);
        myy = vi_Mat_get(m[2], it_row, it_col);
        *it_val = mxx * myy - mxy * mxy - (mxx + myy) * (mxx + myy) * 0.05;
    }
}

double
adj_sum(vi_Mat mat, size_t offset, size_t row, size_t col) {
    double sum = 0.0;
    for(size_t dr = 0, dc; dr <= offset * 2; ++dr) for(dc = 0; dc <= offset * 2; ++dc) {
        if(dr == offset && dc == offset) continue;
        sum += vi_Mat_get(mat, row + dr - offset, col + dc - offset);
    }
    return sum;
}

bool
suppress_non_maxima(vi_Mat r, size_t offset, size_t row, size_t col) {
    size_t r0, c0, rf, cf;
    r0 = (row > offset) ? (row - offset) : 0;
    rf = (row + offset + 1) < vi_Mat_rows(r) ? (row + offset + 1) : vi_Mat_rows(r);
    cf = (col + offset + 1) < vi_Mat_cols(r) ? (col + offset + 1) : vi_Mat_cols(r);
    double fst = vi_Mat_get(r, row, col), snd;
    for(; r0 < rf; ++r0) for(c0 = (col > offset) ? (col - offset) : 0; c0 < cf; ++c0) {
        if(row == r0 && col == c0) continue;
        snd = vi_Mat_get(r, r0, c0);
        if(fst < snd) return false;
        if(fst == snd && (r0 < row || (r0 == row && c0 < col))) return false;
    }
    return true;
}

vi_CornerList
vi_CornerList_init(vi_ImageData data, vi_CornerDetector op, double t) {
    // compute gradient compositions
    vi_Mat dxx = vi_Mat_init_copy(data.dx);
    vi_Mat dxy = vi_Mat_init_copy(data.dx);
    vi_Mat dyy = vi_Mat_init_copy(data.dy);
    vi_Mat_scale_elem_wise(dxy, data.dy);
    vi_Mat_scale_elem_wise(dxx, data.dx);
    vi_Mat_scale_elem_wise(dyy, data.dy);
    // init structure matrices
    vi_Mat m[3];
    m[0] = vi_Mat_init_zeros_like(data.image); // mxx
    m[1] = vi_Mat_init_zeros_like(data.image); // mxy
    m[2] = vi_Mat_init_zeros_like(data.image); // myy
    { // compute structure matrices
        double sum;
        vi_Mat_it(data.image) {
            if(out_of_bounds(data.image, data.offset, it_row, it_col)) continue;
            sum = adj_sum(dxx, data.offset, it_row, it_col);
            vi_Mat_set(m[0], it_row, it_col, sum);
            sum = adj_sum(dxy, data.offset, it_row, it_col);
            vi_Mat_set(m[1], it_row, it_col, sum);
            sum = adj_sum(dyy, data.offset, it_row, it_col);
            vi_Mat_set(m[2], it_row, it_col, sum);
        }
    }
    // free composed gradients
    vi_Mat_free(dxx);
    vi_Mat_free(dxy);
    vi_Mat_free(dyy);
    // calculate R values
    vi_Mat r = vi_Mat_init_zeros_like(data.image);
    (op)(r, m);
    // free structure matrices
    vi_Mat_free(m[0]);
    vi_Mat_free(m[1]);
    vi_Mat_free(m[2]);
    // filter and normalize R values
    double minima = DBL_MAX, maxima = DBL_MIN;
    {
        vi_Mat_it(r) {
            if(*it_val < 0.0) continue;
            minima = (minima > *it_val) ? *it_val : minima;
            maxima = (maxima < *it_val) ? *it_val : maxima;
        }
    }
    // assemble corners list
    vi_CornerList corners = { .x = NULL, .y = NULL };
    {
        vi_Mat_it(r) {
            if((*it_val - minima) / (maxima - minima) < t) continue;
            if(suppress_non_maxima(r, data.offset, it_row, it_col)) continue;
            stbds_arrput(corners.x, it_col);
            stbds_arrput(corners.y, it_row);
        }
    }
    return corners;
}

void
vi_CornerList_free(vi_CornerList corners) {
    stbds_arrfree(corners.x);
    stbds_arrfree(corners.y);
}


struct plotter_data {
    GLuint shader;
    size_t corner_count;
    float* corners;
    GLuint VAO, VBO, EBO;
};

void
corners_plotter_config(void* data) {
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
corners_plotter_render(const void* data) {
    const struct plotter_data* plotter_data = (const struct plotter_data*) data;
    glEnable(GL_PROGRAM_POINT_SIZE);
    glUseProgram(plotter_data->shader);
    glBindVertexArray(plotter_data->VAO);
    glDrawArrays(GL_POINTS, 0, (GLsizei) plotter_data->corner_count);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

void
corners_plotter_deinit(void* data) {
    struct plotter_data* plotter_data = (struct plotter_data*) data;
    free(plotter_data->corners);
    glDeleteBuffers(1, &(plotter_data->VBO));
    glDeleteVertexArrays(1, &(plotter_data->VAO));
    glDeleteProgram(plotter_data->shader);
}

vi_Plotter
vi_CornerList_plot(vi_ImageData data, vi_CornerList corners) {
    long corner_count = stbds_arrlen(corners.x);
    ASSERT(corner_count >= 0 && corner_count == stbds_arrlen(corners.y));
    vi_Plotter layer = vi_Plotter_init(sizeof(struct plotter_data),
        corners_plotter_config,
        corners_plotter_render,
        corners_plotter_deinit);
    struct plotter_data* plotter_data = ((struct plotter_data*) vi_Plotter_data(layer));
    plotter_data->corner_count = (size_t) corner_count;
    plotter_data->corners = malloc(sizeof(float) * 2 * (size_t) plotter_data->corner_count);
    ASSERT(plotter_data->corners != NULL);
    for(size_t i = 0; i < plotter_data->corner_count; ++i) {
        plotter_data->corners[i * 2] = 2.f * ((float) corners.x[i] / \
            (float) vi_Mat_cols(data.image)) - 1.f;
        plotter_data->corners[i * 2 + 1] = 1.f - 2.f * \
            ((float) corners.y[i] / (float) vi_Mat_rows(data.image));
    }
    return layer;
}
