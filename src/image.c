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
img_render_preamble(vi_ImageIntensity img, GLuint* img_tex, GLuint buf[static 3], GLuint* shader) {
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
    *shader = glCreateProgram();
    ASSERT_LOG(*shader != 0, "Failed to initialize shader program.");
    glAttachShader(*shader, vert);
    glAttachShader(*shader, frag);
    glLinkProgram(*shader);
    glGetProgramiv(*shader, GL_LINK_STATUS, &success);
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
    glGenVertexArrays(1, buf);
    glGenBuffers(1, buf + 1);
    glGenBuffers(1, buf + 2);
    glBindVertexArray(*buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, NULL);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*) (sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    // texture initialization
    glGenTextures(1, img_tex);
    glBindTexture(GL_TEXTURE_2D, *img_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    float* img_buf_f = malloc(sizeof(float) * img->w * img->h);
    for(size_t i = 0; i < img->w * img->h; i++) img_buf_f[i] = (float) img->buf[i];
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (GLsizei) img->w, (GLsizei) img->h, 0, GL_RED, GL_FLOAT, img_buf_f);
    free(img_buf_f);
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, (GLint[]) { GL_RED, GL_RED, GL_RED, GL_ONE });
    glBindTexture(GL_TEXTURE_2D, 0);
}

void
img_render(GLuint img_tex, GLuint buf[static 3], GLuint shader) {
    glUseProgram(shader);
    glBindVertexArray(buf[0]);
    glBindTexture(GL_TEXTURE_2D, img_tex);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void
img_render_clean_up(GLuint img_tex, GLuint buf[static 3], GLuint shader) {
    glDeleteTextures(1, &img_tex);
    glDeleteBuffers(1, buf + 1);
    glDeleteBuffers(1, buf + 2);
    glDeleteVertexArrays(1, buf);
    glDeleteProgram(shader);
}

void
pts_render_preamble(vi_ImageIntensity img, size_t* corners, size_t count, GLuint buf[static 3], GLuint* shader) {
    float* corners_ndc = malloc(sizeof(float) * 2 * count);
    for (size_t i = 0; i < count; i++) {
        corners_ndc[i * 2 + 1] = 2.f * ((float) corners[i * 2]   / (float) img->w) - 1.f;
        corners_ndc[i * 2] = 1.f - 2.f * ((float) corners[i * 2 + 1] / (float) img->h);
    }

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

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vert_src, NULL);
    glCompileShader(vert);
    GLint success;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    ASSERT_LOG(success == GL_TRUE, "Failed to compile pts vertex shader.");

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_src, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    ASSERT_LOG(success == GL_TRUE, "Failed to compile pts fragment shader.");

    *shader = glCreateProgram();
    glAttachShader(*shader, vert);
    glAttachShader(*shader, frag);
    glLinkProgram(*shader);
    glGetProgramiv(*shader, GL_LINK_STATUS, &success);
    ASSERT_LOG(success == GL_TRUE, "Failed to link pts shader program.");
    glDeleteShader(vert);
    glDeleteShader(frag);

    glGenVertexArrays(1, buf);
    glGenBuffers(1, buf + 1);
    glBindVertexArray(buf[0]);
    glBindBuffer(GL_ARRAY_BUFFER, buf[1]);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (sizeof(float) * count * 2), corners_ndc, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    free(corners_ndc);
}

void
pts_render(size_t count, GLuint buf[static 3], GLuint shader) {
    glEnable(GL_PROGRAM_POINT_SIZE);
    glUseProgram(shader);
    glBindVertexArray(buf[0]);
    glDrawArrays(GL_POINTS, 0, (GLsizei) count);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

void
pts_render_clean_up(GLuint buf[static 3], GLuint shader) {
    glDeleteBuffers(1, buf + 1);
    glDeleteVertexArrays(1, buf);
    glDeleteProgram(shader);
}

void 
vi_ImageIntensity_show(vi_ImageIntensity img, size_t* corners, size_t count) {
    // windows initialization
    RGFW_window* win = RGFW_createWindow("demo", RGFW_RECT(0, 0, 800, 600), RGFW_windowCenter);
    glenv_init(win);
    const GLenum status = glewInit();
    ASSERT_LOG(status == GLEW_OK, "Failed to initialize GLEW:\n        %s", glewGetErrorString(status));
    // initialize render passes
    GLuint img_tex, img_shader, img_buf[3];
    img_render_preamble(img, &img_tex, img_buf, &img_shader);
    GLuint pts_shader, pts_buf[3];
    pts_render_preamble(img, corners, count, pts_buf, &pts_shader);
    // event loop
    while (!RGFW_window_shouldClose(win)) {
        glenv_new_frame();
        glClearColor(0.1f, 0.18f, 0.24f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        // perform render passes
        img_render(img_tex, img_buf, img_shader);
        pts_render(count, pts_buf, pts_shader);
        // execute frame
        glenv_render(NK_ANTI_ALIASING_ON);
    }
    // clean up
    img_render_clean_up(img_tex, img_buf, img_shader);
    pts_render_clean_up(pts_buf, pts_shader);
    // deinit glenv and window
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

vi_ImageIntensity
vi_ImageIntensity_from_Obj(FLA_Obj mat) {
    size_t rc, cc;
    rc = (size_t) FLA_Obj_length(mat);
    cc = (size_t) FLA_Obj_width(mat); 
    vi_ImageIntensity img = malloc(sizeof(struct __IMAGE_H__vi_ImageIntensity) + sizeof(double) * rc * cc);
    img->h = rc;
    img->w = cc;
    memcpy((double*) img->buf, FLA_Obj_buffer_at_view(mat), sizeof(double) * img->w * img->h);
    return img;
}
