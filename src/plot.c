#include "plot.h"
#include <FLAME.h>
#include <glenv.h>
#include <stb_ds.h>
#include "log.h"

vi_Plotter
vi_Plotter_init(
    size_t data_size, 
    vi_Plotter_config config,
    vi_Plotter_render render,
    vi_Plotter_deinit deinit
) {
    vi_Plotter layer;
    layer = malloc(sizeof(*layer) + data_size);
    ASSERT_LOG(layer != NULL, "Failed to allocated vi_Plotter.");
    layer->config = config;
    layer->render = render;
    layer->deinit = deinit;
    layer->data_size = data_size;
    return layer;
}

void*
vi_Plotter_data(vi_Plotter layer) {
    return layer->data;
}

struct __PLOT_H__vi_Plot {
    vi_Plotter* layers;
};

vi_Plot
vi_Plot_init() {
    vi_Plot plot = malloc(sizeof(struct __PLOT_H__vi_Plot));
    ASSERT_LOG(plot != NULL, "Failed to allocate vi_Plot.");
    plot->layers = NULL;
    return plot;
}

void
vi_Plot_free(vi_Plot plot) {
    long layer_count = stbds_arrlen(plot->layers);
    ASSERT_LOG(layer_count >= 0, "Failed to free vi_Plot.");
    for(size_t i = 0; i < (size_t) layer_count; ++i) free(plot->layers[i]);
    stbds_arrfree(plot->layers);
    free(plot);
}

void
vi_Plot_add_layer(vi_Plot plot, vi_Plotter layer) {
    stbds_arrput(plot->layers, layer);
}

void
vi_Plot_show(vi_Plot plot) {
    // windows initialization
    RGFW_window* win = RGFW_createWindow("demo", RGFW_RECT(0, 0, 800, 600), RGFW_windowCenter);
    glenv_init(win);
    const GLenum status = glewInit();
    ASSERT_LOG(status == GLEW_OK, "Failed to initialize GLEW:\n        %s", glewGetErrorString(status));
    // initialize render passes
    long layer_count = stbds_arrlen(plot->layers);
    ASSERT_LOG(layer_count >= 0, "Failed to show vi_Plot.");
    for(size_t i = 0; i < (size_t) layer_count; ++i)
        (plot->layers[i]->config)((void*) plot->layers[i]->data);
    // event loop
    while (!RGFW_window_shouldClose(win)) {
        glenv_new_frame();
        glClearColor(0.1f, 0.18f, 0.24f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        // perform render passes
        for(size_t i = 0; i < (size_t) layer_count; ++i)
            (plot->layers[i]->render)((const void*) plot->layers[i]->data);
        // execute frame
        glenv_render(NK_ANTI_ALIASING_ON);
    }
    // clean up
    for(size_t i = 0; i < (size_t) layer_count; ++i)
        (plot->layers[i]->deinit)((void*) plot->layers[i]->data);
    // deinit glenv and window
    glenv_deinit();
    RGFW_window_close(win);
}
