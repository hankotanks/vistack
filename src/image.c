#include "image.h"
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
    unsigned char* buf = stbi_load(path, &w, &h, &c, 3);
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
        buf[i] /= (double) img->c * 255.f;
    }
    return img_i;
}

const double* 
vi_ImageIntensity_buffer(vi_ImageIntensity img) {
    return img->buf;
}

void 
vi_ImageIntensity_show(vi_ImageIntensity img) {
    // windows initialization
    RGFW_window* win = RGFW_createWindow("demo", RGFW_RECT(0, 0, 800, 600), RGFW_windowCenter);
    glenv_init(win);
    const GLenum status = glewInit();
    ASSERT_LOG(status == GLEW_OK, "Failed to initialize GLEW:\n        %s", glewGetErrorString(status));
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
        "    double v = texture(tex, TexCoord).r;\n"
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
    ASSERT_LOG(success, "Failed to compile fragment shader.");
    // create program
    GLuint shader = glCreateProgram();
    ASSERT_LOG(shader != 0, "Failed to initialize shader program.");
    glAttachShader(shader, vert);
    glAttachShader(shader, frag);
    glLinkProgram(shader);
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    ASSERT_LOG(success, "Failed to link shader program.");
    glDeleteShader(vert);
    glDeleteShader(frag);
    // declare quad geometry
    const double quad_vertices[] = { 
        -1.f, -1.f, 0.f, 1.f,
         1.f, -1.f, 1.f, 1.f,
         1.f,  1.f, 1.f, 0.f,
        -1.f,  1.f, 0.f, 0.f
    };
    const unsigned int quad_indices[] = { 0, 1, 2, 2, 3, 0 };
    // initialize buffers
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(double) * 4, NULL);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(double) * 4, (void*) (sizeof(double) * 2));
    glEnableVertexAttribArray(1);
    // texture initialization
    GLuint img_tex_id;
    glGenTextures(1, &img_tex_id);
    glBindTexture(GL_TEXTURE_2D, img_tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (GLsizei) img->w, (GLsizei) img->h, 0, GL_RED, GL_FLOAT, img->buf);
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, (GLint[]) { GL_RED, GL_RED, GL_RED, GL_ONE });
    glBindTexture(GL_TEXTURE_2D, 0);
    // event loop
    while (!RGFW_window_shouldClose(win)) {
        glenv_new_frame();
        glClearColor(0.1f, 0.18f, 0.24f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader);
        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_2D, img_tex_id);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glenv_render(NK_ANTI_ALIASING_ON);
    }
    // clean up
    glDeleteTextures(1, &img_tex_id);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shader);
    glenv_deinit();
    RGFW_window_close(win);
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
vi_ImageIntensity_to_Obj(vi_ImageIntensity img) {
    FLA_Obj mat;
    FLA_Obj_create(FLA_DOUBLE, img->h, img->w, 0, 0, &mat);
    memcpy(FLA_Obj_buffer_at_view(mat), img->buf, img->w * img->h * sizeof(double));
    return mat;
}
