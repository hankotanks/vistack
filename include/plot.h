#ifndef __PLOT_H__
#define __PLOT_H__

#include <stddef.h>

typedef void (*vi_Plotter_config)(void*);
typedef void (*vi_Plotter_render)(const void*);
typedef void (*vi_Plotter_deinit)(void*);

typedef struct {
    void (*config)(void* data);
    void (*render)(const void* data);
    void (*deinit)(void* data);
    size_t data_size;
    unsigned char data[];
}* vi_Plotter;

vi_Plotter
vi_Plotter_init(size_t data_size, 
    vi_Plotter_config config,
    vi_Plotter_render render, 
    vi_Plotter_deinit deinit);
void*
vi_Plotter_data(vi_Plotter layer);

typedef struct __PLOT_H__vi_Plot* vi_Plot;

vi_Plot
vi_Plot_init(const char* title, size_t rows, size_t cols);
void
vi_Plot_free(vi_Plot plot);
void
vi_Plot_add_layer(vi_Plot plot, size_t row, size_t col, size_t row_span, size_t col_span, vi_Plotter layer);
void
vi_Plot_show(vi_Plot plot);

#endif // __PLOT_H__
