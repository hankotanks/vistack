#include "image.h"
#include "plot.h"
#include <string.h>
#include <FLAME.h>
#include <glenv.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "log.h"

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
    vi_ImageRaw img = malloc(sizeof(struct __IMAGE_H__vi_ImageRaw) + (size_t) (w * h * c));
    ASSERT_LOG(img != NULL, "Failed to allocate memory for image [%s],", path);
    img->w = (size_t) w;
    img->h = (size_t) h;
    img->c = (size_t) c;
    memcpy((unsigned char*) img->buf, buf, (size_t) (w * h * c));
    return img; 
}

vi_ImageIntensity 
vi_ImageIntensity_from_ImageRaw(const vi_ImageRaw img) {
    vi_ImageIntensity img_i = malloc(sizeof(struct __IMAGE_H__vi_ImageIntensity) + (size_t) (img->w * img->h) * sizeof(double));
    ASSERT_LOG(img_i != NULL, "Failed to allocate memory for image intensity.");
    img_i->w = img->w;
    img_i->h = img->h;
    double* buf = (double*) img_i->buf;
    for(size_t i = 0, j; i < img->w * img->h; ++i) {
        for(j = 0; j < img->c; ++j) buf[i] += (double) img->buf[i * img->c + j];
        buf[i] /= (double) img->c * 255.0;
    }
    return img_i;
}

const double* 
vi_ImageIntensity_buffer(vi_ImageIntensity img) {
    return img->buf;
}

unsigned long
vi_ImageIntensity_rows(vi_ImageIntensity img) {
    return img->h;
}

unsigned long
vi_ImageIntensity_cols(vi_ImageIntensity img) {
    return img->w;
}

FLA_Obj
vi_ImageIntensity_to_Mat(vi_ImageIntensity img) {
    FLA_Obj mat;
    FLA_Obj_create(FLA_DOUBLE, img->h, img->w, 0, 0, &mat);
    memcpy(FLA_Obj_buffer_at_view(mat), img->buf, img->w * img->h * sizeof(double));
    return mat;
}

vi_ImageIntensity
vi_Mat_to_ImageIntensity(FLA_Obj mat) {
    size_t rc, cc;
    rc = (size_t) FLA_Obj_length(mat);
    cc = (size_t) FLA_Obj_width(mat); 
    vi_ImageIntensity img = malloc(sizeof(struct __IMAGE_H__vi_ImageIntensity) + sizeof(double) * rc * cc);
    img->h = rc;
    img->w = cc;
    memcpy((double*) img->buf, FLA_Obj_buffer_at_view(mat), sizeof(double) * img->w * img->h);
    return img;
}

struct plotter_data {
    GLuint shader;
    vi_ImageIntensity img;
    GLuint tex;
    GLuint VAO, VBO, EBO;
};

void
image_intensity_plotter_config(void* data) {
    // get pointer to the data
    struct plotter_data* plotter_data = (struct plotter_data*) data;
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
    vi_ImageIntensity img = plotter_data->img;
    float* buf = malloc(sizeof(float) * img->w * img->h);
    for(size_t i = 0; i < img->w * img->h; i++) buf[i] = (float) img->buf[i];
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (GLsizei) img->w, (GLsizei) img->h, 0, GL_RED, GL_FLOAT, buf);
    free(buf);
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, (GLint[]) { GL_RED, GL_RED, GL_RED, GL_ONE });
    glBindTexture(GL_TEXTURE_2D, 0);
}

void
image_intensity_plotter_render(const void* data) {
    const struct plotter_data* plotter_data = (const struct plotter_data*) data;
    glUseProgram(plotter_data->shader);
    glBindVertexArray(plotter_data->VAO);
    glBindTexture(GL_TEXTURE_2D, plotter_data->tex);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void
image_intensity_plotter_deinit(void* data) {
    struct plotter_data* plotter_data = (struct plotter_data*) data;
    glDeleteTextures(1, &(plotter_data->tex));
    glDeleteBuffers(1, &(plotter_data->VBO));
    glDeleteBuffers(1, &(plotter_data->EBO));
    glDeleteVertexArrays(1, &(plotter_data->VAO));
    glDeleteProgram(plotter_data->shader);
}

vi_Plotter
vi_ImageIntensity_plotter(vi_ImageIntensity img) {
    vi_Plotter layer = vi_Plotter_init(
        sizeof(struct plotter_data),
        image_intensity_plotter_config,
        image_intensity_plotter_render,
        image_intensity_plotter_deinit);
    ((struct plotter_data*) vi_Plotter_data(layer))->img = img;
    return layer;
}
