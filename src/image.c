#include "image.h"

#include <string.h>
#include <glenv.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "log.h"
#include "mat.h"
#include "plot.h"

struct __IMAGE_H__vi_ImageRaw {
    size_t w, h, c; // width, height, channel count
    const unsigned char buf[];
};

struct __IMAGE_H__vi_ImageIntensity {
    size_t w, h;
    const double buf[];
};

vi_ImageRaw 
vi_ImageRaw_load(const char* path) {
    int w, h, c;
    unsigned char* buf = stbi_load(path, &w, &h, &c, 0);
    ASSERT_LOG(buf != NULL, "Failed to open image [%s].", path);
    vi_ImageRaw image = malloc(sizeof(struct __IMAGE_H__vi_ImageRaw) + (size_t) (w * h * c));
    ASSERT_LOG(image != NULL, "Failed to allocate memory for image [%s],", path);
    image->w = (size_t) w;
    image->h = (size_t) h;
    image->c = (size_t) c;
    memcpy((unsigned char*) image->buf, buf, (size_t) (w * h * c));
    return image; 
}

vi_ImageIntensity 
vi_ImageIntensity_from_ImageRaw(const vi_ImageRaw image_raw) {
    vi_ImageIntensity image;
    image = calloc(1, sizeof(*image) + (size_t) (image_raw->w * image_raw->h) * sizeof(double));
    ASSERT_LOG(image != NULL, "Failed to allocate memory for image intensity.");
    image->w = image_raw->w;
    image->h = image_raw->h;
    double* buf = (double*) image->buf;
    for(size_t i = 0, j; i < image_raw->w * image_raw->h; ++i) {
        for(j = 0; j < image_raw->c; ++j) buf[i] += (double) image_raw->buf[i * image_raw->c + j];
        buf[i] /= (double) image_raw->c * 255.0;
    }
    return image;
}

const double* 
vi_ImageIntensity_buffer(vi_ImageIntensity image) {
    return image->buf;
}

unsigned long
vi_ImageIntensity_rows(vi_ImageIntensity image) {
    return image->h;
}

unsigned long
vi_ImageIntensity_cols(vi_ImageIntensity image) {
    return image->w;
}

vi_Mat
vi_ImageIntensity_to_Mat(vi_ImageIntensity image) {
    vi_Mat mat = vi_Mat_init_zeros(image->h, image->w);
    vi_Mat_it(mat) *it_val = image->buf[it_row * image->w + it_col];
    return mat;
}

vi_ImageIntensity
vi_Mat_to_ImageIntensity(vi_Mat mat) {
    vi_ImageIntensity image;
    image = malloc(sizeof(*image) + sizeof(double) * vi_Mat_rows(mat) * vi_Mat_cols(mat));
    image->h = vi_Mat_rows(mat);
    image->w = vi_Mat_cols(mat);
    vi_Mat_it(mat) ((double*) image->buf)[it_col + it_row * vi_Mat_cols(mat)] = *it_val;
    return image;
}

struct corners_plotter_data {
    GLuint shader;
    vi_ImageIntensity image;
    GLuint tex;
    GLuint VAO, VBO, EBO;
};

void
image_intensity_plotter_config(void* data) {
    // get pointer to the data
    struct corners_plotter_data* plotter_data = (struct corners_plotter_data*) data;
    // fragment and vertex shader sources
    const char* vert_src =
        "#version 330 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "layout(location = 1) in vec2 aTex;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "    TexCoord = aTex;\n"
        "}\n";
    const char* frag_src =
        "#version 330 core\n"
        "in vec2 TexCoord;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D tex;\n"
        "void main() {\n"
        "    float v = texture(tex, TexCoord).r;\n"
        "    FragColor = vec4(v, v, v, 1.0);\n"
        "}\n";
    // create shaders
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vert_src, NULL);
    glCompileShader(vert);
    GLint success;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    ASSERT_LOG(success, "Failed to compile vertex shader.");
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_src, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    ASSERT_LOG(success == GL_TRUE, "Failed to compile fragment shader.");
    // create program
    plotter_data->shader = glCreateProgram();
    ASSERT_LOG(plotter_data->shader != 0, "Failed to initialize shader program.");
    glAttachShader(plotter_data->shader, vert);
    glAttachShader(plotter_data->shader, frag);
    glLinkProgram(plotter_data->shader);
    glGetProgramiv(plotter_data->shader, GL_LINK_STATUS, &success);
    ASSERT_LOG(success == GL_TRUE, "Failed to link shader program.");
    glDeleteShader(vert);
    glDeleteShader(frag);
    // declare quad geometry
    const float quad_vertices[] = { 
        -1.f, -1.f, 0.f, 1.f,
         1.f, -1.f, 1.f, 1.f,
         1.f,  1.f, 1.f, 0.f,
        -1.f,  1.f, 0.f, 0.f
    };
    const unsigned int quad_indices[] = { 0, 1, 2, 2, 3, 0 };
    // initialize buffers
    glGenVertexArrays(1, &(plotter_data->VAO));
    glGenBuffers(1, &(plotter_data->VBO));
    glGenBuffers(1, &(plotter_data->EBO));
    glBindVertexArray(plotter_data->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, plotter_data->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plotter_data->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, NULL);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*) (sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    // texture initialization
    glGenTextures(1, &(plotter_data->tex));
    glBindTexture(GL_TEXTURE_2D, plotter_data->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    vi_ImageIntensity image = plotter_data->image;
    float* buf = malloc(sizeof(float) * image->w * image->h);
    for(size_t i = 0; i < image->w * image->h; i++) buf[i] = (float) image->buf[i];
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (GLsizei) image->w, (GLsizei) image->h, 0, GL_RED, GL_FLOAT, buf);
    free(buf);
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, (GLint[]) { GL_RED, GL_RED, GL_RED, GL_ONE });
    glBindTexture(GL_TEXTURE_2D, 0);
}

void
image_intensity_plotter_render(const void* data) {
    const struct corners_plotter_data* plotter_data = (const struct corners_plotter_data*) data;
    glUseProgram(plotter_data->shader);
    glBindVertexArray(plotter_data->VAO);
    glBindTexture(GL_TEXTURE_2D, plotter_data->tex);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void
image_intensity_plotter_deinit(void* data) {
    struct corners_plotter_data* plotter_data = (struct corners_plotter_data*) data;
    glDeleteTextures(1, &(plotter_data->tex));
    glDeleteBuffers(1, &(plotter_data->VBO));
    glDeleteBuffers(1, &(plotter_data->EBO));
    glDeleteVertexArrays(1, &(plotter_data->VAO));
    glDeleteProgram(plotter_data->shader);
}

vi_Plotter
vi_ImageIntensity_plotter(vi_ImageIntensity image) {
    vi_Plotter layer = vi_Plotter_init(
        sizeof(struct corners_plotter_data),
        image_intensity_plotter_config,
        image_intensity_plotter_render,
        image_intensity_plotter_deinit);
    ((struct corners_plotter_data*) vi_Plotter_data(layer))->image = image;
    return layer;
}
