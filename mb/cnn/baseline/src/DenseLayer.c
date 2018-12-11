/* 
 * DenseLayer.c
 * Francesco Conti <f.conti@unibo.it>
 *
 * Copyright (C) 2015 ETH Zurich, University of Bologna
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include "linalg.h"
#include "tiling.h"
#include "DenseLayer.h"

#ifdef CCN_TILING_LESSTIME
    #define _dense_tiling_init(); \
        unsigned char (*tile_grid_non)[layer->ntile_nin] = (unsigned char (*)[layer->ntile_nin]) layer->tile_grid_non; \
        unsigned char (*tile_grid_nin)[layer->ntile_nin] = (unsigned char (*)[layer->ntile_nin]) layer->tile_grid_nin; \
        int _non = tile_grid_non[aa][bb]; \
        int _nin = tile_grid_nin[aa][bb];
#else /* ~CCN_TILING_LESSTIME */
    #define _dense_tiling_init(); \
        int _non = (aa < layer->ntile_full_non) ? layer->tiling_max_non    : layer->tlast_non; \
        int _nin = (bb < layer->ntile_full_nin) ? layer->tiling_max_nin    : layer->tlast_nin;
#endif /* ~CCN_TILING_LESSTIME */

#define _dense_notiling_init(); \
    int _non = layer->n_out_neurons; \
    int _nin = layer->n_in_neurons;

/**
 *  Allocates a new DenseLayer data structure and its fields (weight, bias,
 *  output feature maps).
 *
 *  @return a pointer to the new DenseLayer data structure.
 *
 *  @param n_in_neurons
 *      the number of input feature maps.
 *  @param n_out_neurons
 *      the number of output feature maps.
 *  @param input_height
 *      the height of the input feature maps.
 *  @param input_width
 *      the width of the input feature maps.
 *  @param output_height
 *      the height of the output feature maps.
 *  @param output_width
 *      the width of the output feature maps.
 *  @param activation
 *      1 if activation is tanh, 0 if no activation.
 *  @param *x
 *      a *mandatory* pointer to the input feature maps.
 *  @param *y
 *      an *optional* pointer to the already-allocated output feature maps. If
 *      NULL, DenseLayer_new() will allocate y automatically.
 */
DenseLayer *DenseLayer_new(
#ifdef CCN_NOALLOC
    DenseLayer *layer,
#endif /* CCN_NOALLOC */
    const char *name,
    data_t *w,
    data_t *b,
    data_t *x,
    data_t *y,
    data_t *loc_x0,
    data_t *loc_x1,
    data_t *loc_y0,
    data_t *loc_y1,
    data_t *loc_y2,
    data_t *loc_w0,
    data_t *loc_w1,
    data_t *loc_b,
    int n_out_neurons,
    int n_in_neurons,
    int activation,
    int tiling_max_non,
    int tiling_max_nin,
    unsigned qf
) {

#ifndef CCN_NOALLOC
    // build DenseLayer
    DenseLayer *layer;
    layer = ccn_malloc(sizeof(DenseLayer));
#endif /* ifndef CCN_NOALLOC */

    layer->name          = name;
    layer->n_in_neurons  = n_in_neurons;
    layer->n_out_neurons = n_out_neurons;
    layer->activation    = activation;
    layer->w             = w;
    layer->b             = b;
    layer->x             = x;
    layer->y             = y;
    layer->qf            = qf;

#ifndef CCN_CACHE
    layer->loc_x0 = loc_x0;
    layer->loc_y0 = loc_y0;
    layer->loc_x1 = loc_x1;
    layer->loc_y1 = loc_y1;
    layer->loc_y2 = loc_y2;
    layer->loc_w0 = loc_w0;
    layer->loc_w1 = loc_w1;
    layer->loc_b  = loc_b;
#endif /* ifndef CCN_CACHE */

    layer->tiling_max_non    = tiling_max_non;
    layer->tiling_max_nin    = tiling_max_nin;

#ifdef CCN_TILING
    // define and record the number of tiles
    int ntile_non = (n_out_neurons % tiling_max_non   ) ? n_out_neurons / tiling_max_non    + 1 : n_out_neurons / tiling_max_non;
    int ntile_nin = (n_in_neurons  % tiling_max_nin   ) ? n_in_neurons  / tiling_max_nin    + 1 : n_in_neurons  / tiling_max_nin;

    layer->ntile_non = ntile_non;
    layer->ntile_nin = ntile_nin;

#ifdef CCN_TILING_LESSMEM
    layer->tlast_non = n_out_neurons % tiling_max_non;
    layer->tlast_nin = n_in_neurons  % tiling_max_nin;

    layer->ntile_full_non = ntile_non;
    layer->ntile_full_nin = ntile_nin;
#else /* ~CCN_TILING_LESSMEM */
    // allocate the tile grid in a flat fashion
    layer->tile_grid_non = ccn_malloc(sizeof(unsigned char)*(ntile_non+NB_PIPE_STAGE-1)*ntile_nin);
    layer->tile_grid_nin = ccn_malloc(sizeof(unsigned char)*(ntile_non+NB_PIPE_STAGE-1)*ntile_nin);

    // cast the tile grid to a 4-dimensional array
    unsigned char (*tile_grid_non)[ntile_nin] = layer->tile_grid_non;
    unsigned char (*tile_grid_nin)[ntile_nin] = layer->tile_grid_nin;
#endif /* ~CCN_TILING_LESSMEM */

    // fill in the tile grid
    int aa, bb;
    for(aa=0; aa<layer->ntile_non; aa++) {
        for(bb=0; bb<layer->ntile_nin; bb++) {

#ifdef CCN_TILING_LESSTIME
            if(bb*tiling_max_nin > n_in_neurons-tiling_max_nin) {
                tile_grid_nin[aa][bb] = (unsigned char) n_in_neurons % tiling_max_nin;
            }
            else {
                tile_grid_nin[aa][bb] = (unsigned char) tiling_max_nin;
            }

            if(aa*tiling_max_non > n_out_neurons-tiling_max_non) {
                tile_grid_non[aa][bb] = (unsigned char) n_out_neurons % tiling_max_non;
            }
            else {
                tile_grid_non[aa][bb] = (unsigned char) tiling_max_non;
            }
#else /* ~CCN_TILING_LESSTIME */
            if(bb*tiling_max_nin > n_in_neurons-tiling_max_nin) {
                layer->ntile_full_nin = bb;
            }

            if(aa*tiling_max_non > n_out_neurons-tiling_max_non) {
                layer->ntile_full_non = aa;
            }
#endif /* ~CCN_TILING_LESSTIME */

        }
    }
#ifdef CCN_TILING_LESSTIME
    for(aa=layer->ntile_non; aa<layer->ntile_non+NB_PIPE_STAGE-1; aa++) {
        for(bb=0; bb<layer->ntile_nin; bb++) {
            tile_grid_nin[aa][bb] = tiling_max_nin;
            tile_grid_non[aa][bb] = tiling_max_non;
        }
    }
#endif /* CCN_TILING_LESSTIME */
#else /* ~CCN_TILING */
    // no tile grid
    int ntile_non = n_out_neurons;
    int ntile_nin = n_in_neurons;
    layer->ntile_non = ntile_non;
    layer->ntile_nin = ntile_nin;
#endif /* ~CCN_TILING */

#ifdef TILING_DEBUG
    printf("[DenseLayer %s] NOn grid:\n", layer->name);
    for(aa=0; aa<layer->ntile_non; aa++) {
        printf("  ");
        for(bb=0; bb<layer->ntile_nin; bb++) {
            printf("%d ", tile_grid_non[aa][bb]);
        }
        printf("\n");
    }
    printf("[DenseLayer %s] NIn grid:\n", layer->name);
    for(aa=0; aa<layer->ntile_non; aa++) {
        printf("  ");
        for(bb=0; bb<layer->ntile_nin; bb++) {
            printf("%d ", tile_grid_nin[aa][bb]);
        }
        printf("\n");
    }
#endif /* TILING_DEBUG */

    return layer;

}

void DenseLayer_delete(
    DenseLayer *layer
) {
#ifndef CCN_CACHE
    ccn_free(layer->loc_w0);
    ccn_free(layer->loc_w1);
    ccn_free(layer->loc_b);
#endif /* ~CCN_CACHE */
#ifdef CCN_TILING
    ccn_free(layer->tile_grid_non);
    ccn_free(layer->tile_grid_nin);
#endif /* ~CCN_TILING */
    ccn_free(layer);
}

static void DenseLayer_pipe_fe(
    DenseLayer *layer,
    int aa,
    int bb
) {

#ifdef CCN_CACHE
    return;
#endif

    // if aa is -1, it means that this is the last tile (and bb, ii, jj also = -1)
    if(aa==-1)
        return;

#ifdef FETCH_PROFILE
    perf_enable_all();
    perf_reset();
    perf_start();
#endif /* FETCH_PROFILE */
    {
        _dense_tiling_init()

        data_t *l2_x = ccn_get_tile_1d(
            layer->x,
            bb,
            layer->tiling_max_nin
        );
        data_t *l2_y = ccn_get_tile_1d(
            layer->y,
            aa,
            layer->tiling_max_non
        );

        data_t *l2_W = ccn_get_tile_2d(
            layer->w,
            bb, aa,
            layer->tiling_max_nin, layer->tiling_max_non,
            layer->n_out_neurons
        );

        // X tile copy-in
        ccn_memcpy_async(
            layer->loc_x_fe, // pointers
            l2_x,
            _nin*sizeof(data_t)
        );
        // W copy-in (check misalignment)
        ccn_memcpy_async_2d(
            layer->loc_w_fe, // pointers
            l2_W,
            _nin, // sizes
            _non*sizeof(data_t),
            _non*sizeof(data_t), // local strides
            layer->n_out_neurons*sizeof(data_t) // remote strides
        );
        // b copy-in
        if(bb==0) {
            ccn_memcpy_async(
               layer->loc_b,
               &layer->b[aa*layer->tiling_max_non],
               _non*sizeof(data_t)
           );
        }

#ifdef FETCH_CHECKSUM
        int32_t sum_x = 0;
        int32_t sum_W = 0;
        int32_t sum_y = 0;
        for(int i=0; i<_nin; i++) {
            sum_x += layer->loc_x_fe[i];
        }
        for(int i=0; i<_non*_nin; i++) {
            sum_W += layer->loc_w_fe[i];
        }
        for(int i=0; i<_non; i++) {
            sum_y += layer->loc_y_fe[i];
        }
        printf("[DenseLayer %s] Fetch checksum %d,%d: x=%d W=%d y=%d\n", layer->name, aa, bb, sum_x, sum_W, sum_y);
#endif /* FETCH_CHECKSUM */

    }
#ifdef FETCH_PROFILE
    perf_stop();
    int t0 = perf_get_cycles();
    printf("[DenseLayer %s] Fetch profiling: %d\n", layer->name, t0);
#endif /* FETCH_PROFILE */

}

static void DenseLayer_pipe_ex(
    DenseLayer *layer,
    int aa,
    int bb
) {

    // if aa is -1, it means that this is the first tile (and bb, ii, jj also = -1)
    if(aa==-1)
        return;

#ifdef EXECUTE_PROFILE
    perf_enable_all();
    perf_reset();
    perf_start();
#endif /* EXECUTE_PROFILE */
    // #pragma omp single nowait
    {
#ifdef INTERM_CHECKSUM
    int print_flag = 0;
#endif

#ifdef CCN_TILING
    _dense_tiling_init()
#else /* ~CCN_TILING */
    _dense_notiling_init()
#endif /* ~CCN_TILING */

#ifndef CCN_CACHE

    data_t *_x = layer->loc_x_ex;
    data_t *_y = layer->loc_y_ex;
    data_t *_W = layer->loc_w_ex;
    data_t *_b = layer->loc_b;

#ifndef CCN_DOUBLEBUF
    // wait for the end of the fetch stage if not doing double buffering
    // ccn_memcpy_wait();
    // #pragma omp barrier
#endif /* ~CCN_DOUBLEBUF */

#else /* CCN_CACHE */

    data_t *_x = ccn_get_tile_1d(
        layer->x,
        bb,
        layer->tiling_max_nin
    );
    
    data_t *_y = ccn_get_tile_1d(
        layer->y,
        aa,
        layer->tiling_max_non
    );

    data_t *_W = ccn_get_tile_2d(
        layer->w,
        aa, bb,
        layer->tiling_max_non, layer->tiling_max_nin,
        layer->n_in_neurons*layer->n_out_neurons
    );

#endif /* CCN_CACHE */

    // biasing y
    if(bb==0) {
        for(int a=0; a<_non; a++) {
            _y[a] = _b[a];
        }
    }

    // matrix x vector product
    linalg_mvprod(_W, 0, _x, _y, _nin, _non, layer->qf);
    // plp_matmul_i16(_W, _x, _y, _nin, _non, 1);

    // if(bb == layer->ntile_nin-1) {
    //     printf("EX DEB %d %d\n", aa, bb);
    //     for(int a=0; a<_non; a++) {
    //         char *s = fixed2string(_y[a], 13, 5);
    //         printf(" %d: %04x %s\n", a, _y[a], s);
    //         free(s);
    //     }
    // }

    // activation
    if(layer->activation == ACTIVATION_TANH) {
        for(int a=0; a<_non; a++) {
            _y[a] = ccn_tanh(_y[a]);
        }
    }
    else if(layer->activation == ACTIVATION_RELU) {
        for(int a=0; a<_non; a++) {
            _y[a] = (_y[a] < 0) ? 0 : _y[a];
        }
    }

#ifdef TILE_CHECKSUM
    {
        int i, sum=0;
        printf("[DenseLayer %s] Tile checksum %d,%d: ", layer->name, aa,bb);
        sum=0;
        for(i=0; i<_nin; i++){
            sum+=_x[i];
        }
        printf("xsum=%d, ", sum);
        sum=0;
        for(i=0; i<_non*_nin; i++) {
            sum+=_W[i];
        }
        printf("wsum=%d, ", sum);
        sum=0;
        for(i=0; i<_non; i++) {
            sum+=_y[i];
        }

        printf("ysum=%d\n", sum);
        printf("    xptr=%08x, wptr=%08x, yptr=%08x\n", _x, _W, _y);
    }
#endif
    }
#ifdef EXECUTE_PROFILE
    perf_stop();
    int t0 = perf_get_cycles();
    printf("[DenseLayer %s] Execute profiling: %d\n", layer->name, t0);
#endif /* EXECUTE_PROFILE */

}



static void DenseLayer_pipe_wb(
    DenseLayer *layer,
    int aa,
    int bb
) {

#ifdef CCN_CACHE
    return;
#endif

    // if aa is -1, it means that this is the first tile (and bb, ii, jj also = -1)
    if(aa==-1)
        return;

#ifdef WRITEBACK_PROFILE
    perf_enable_all();
    perf_reset();
    perf_start();
#endif /* WRITEBACK_PROFILE */
    // #pragma omp single
    {
        _dense_tiling_init();

        data_t *l2_y = ccn_get_tile_1d(
            layer->y,
            aa,
            layer->tiling_max_non
        );

#ifdef WRITEBACK_CHECKSUM
        int32_t sum = 0;
        for(int i=0; i<_non; i++) {
            sum += layer->loc_y_wb[i];
        }
        printf("[DenseLayer %s] Writeback checksum %d,%d: %d\n", layer->name, aa, bb, sum);
#endif /* WRITEBACK_CHECKSUM */

#ifdef WRITEBACK_DEBUG
        printf("[DenseLayer %s] Writeback debug %d,%d:\n", layer->name, aa, bb);
        for(int i=0; i<_non; i++) {
            printf("  (%d): %04x\n", i, layer->loc_y_wb[i] & 0xffff);
        }
#endif /* WRITEBACK_DEBUG */

        // Y tile copy-out
        if(bb == layer->ntile_nin-1) {
            ccn_memcpy_async(//
                l2_y, // pointers
                layer->loc_y_wb,
                _non*sizeof(data_t)
            );
        }

    }
#ifdef WRITEBACK_DEBUG
    #pragma omp barrier
#endif
#ifdef WRITEBACK_PROFILE
    perf_stop();
    int t0 = perf_get_cycles();
    printf("[DenseLayer %s] Writeback profiling: %d\n", layer->name, t0);
#endif /* WRITEBACK_PROFILE */
}




/**
 *  Executes the given DenseLayer, i.e. computes its outputs given the inputs
 *  defined in the data structure.
 *  The DenseLayer computes the output of a densely connected neural network
 *  layer with 3d inputs and outputs (an array of 2d feature maps).
 *
 *  @param *layer
 *      a pointer to the DenseLayer data structure to execute.
 */
void DenseLayer_exec(DenseLayer *layer) {

    // DenseLayer_exec is now organized as a pipeline with the following stages
    //   fetch      (fe) : DMA in of a tile
    //   execute    (ex) : execution of layer
    //   write-back (wb) : DMA out of a tile
    // all indeces have a fetch, execute and write-back version

    int aa_pipe,bb_pipe;

    int aa_fe = -1, bb_fe = -1;
    int aa_ex = -1, bb_ex = -1;
    int aa_wb = -1, bb_wb = -1;

#ifdef CCN_DOUBLEBUF
    // initialize double buffering in a known state
    int doublebuf_state_x_fe = 0;
    int doublebuf_state_y_fe = 0;
    int doublebuf_state_y_wb = 0;
#endif /* CCN_DOUBLEBUF */

#ifndef CCN_CACHE
    // initialize state of fe local buffer pointers
    layer->loc_x_fe = layer->loc_x0;
    layer->loc_w_fe = layer->loc_w0;
    layer->loc_y_fe = layer->loc_y0;
#endif /* ~CCN_CACHE */

    // reset the weights!
    memset(layer->loc_w0, 0, sizeof(data_t)*layer->tiling_max_non*layer->tiling_max_nin);
    memset(layer->loc_w1, 0, sizeof(data_t)*layer->tiling_max_non*layer->tiling_max_nin);

#ifdef CCN_TILING
    for(aa_pipe=0; aa_pipe<layer->ntile_non+NB_PIPE_STAGE-1; aa_pipe++) {

        for(bb_pipe=0; bb_pipe<layer->ntile_nin; bb_pipe++) {

            // update state of fe indeces
            if(bb_pipe<layer->ntile_nin) {
                bb_fe = bb_pipe;
                aa_fe = aa_pipe;
            }
            else {
                bb_fe = -1;
                aa_fe = -1;
            }

#ifndef CCN_CACHE
#ifdef CCN_DOUBLEBUF
            // update state of fe local buffer pointers
            if (doublebuf_state_x_fe == 0) {
                layer->loc_x_fe = layer->loc_x0;
            }
            else {
                layer->loc_x_fe = layer->loc_x1;
            }
            if (doublebuf_state_x_fe == 0) {
                layer->loc_w_fe = layer->loc_w0;
            }
            else {
                layer->loc_w_fe = layer->loc_w1;
            }
            if (doublebuf_state_y_fe == 0) {
                layer->loc_y_fe = layer->loc_y0;
            }
            else if (doublebuf_state_y_fe == 1) {
                layer->loc_y_fe = layer->loc_y1;
            }
            else {
                layer->loc_y_fe = layer->loc_y2;
            }
#endif /* CCN_DOUBLEBUF */
#endif /* ~CCN_CACHE */

#ifdef PIPE_DEBUG
            printf("[DenseLayer %s pipe] aa=%d bb=%d\n", layer->name, aa_pipe, bb_pipe);
            printf("  fe: aa=%d bb=%d\n", aa_fe, bb_fe);
            printf("  ex: aa=%d bb=%d\n", aa_ex, bb_ex);
            printf("  wb: aa=%d bb=%d\n", aa_wb, bb_wb);
            printf("  doublebuf states: %d %d %d\n", doublebuf_state_x_fe, doublebuf_state_y_fe, doublebuf_state_y_wb);
            printf("\n");
#endif PIPE_DEBUG
#ifdef PIPE_PROFILE
            reset_timer();
            start_timer();
#endif /* PIPE_PROFILE */
// #ifndef DISABLE_OPENMP
//             #pragma omp parallel num_threads(3)
// #endif
            {
                // fetch stage
// #ifndef DISABLE_OPENMP
//                 if(omp_get_thread_num() == THREAD_FE)
// #endif
                    DenseLayer_pipe_fe(layer, aa_fe, bb_fe);
                // execute stage
// #ifndef DISABLE_OPENMP
//                 if(omp_get_thread_num() == THREAD_EX)
// #endif
                    DenseLayer_pipe_ex(layer, aa_ex, bb_ex);

                // write-back stage
// #ifndef DISABLE_OPENMP
//                 if(omp_get_thread_num() == THREAD_WB)
// #endif
                    DenseLayer_pipe_wb(layer, aa_wb, bb_wb);
                }
#ifdef PIPE_PROFILE
                stop_timer();
                int t0 = get_time();
                reset_timer();
                printf("[DenseLayer %s] Pipe profiling: %d\n", layer->name, t0);
#endif /* PIPE_PROFILE */

                // update state of ex,wb indeces
                bb_wb = bb_ex;
                bb_ex = bb_fe;
                aa_wb = aa_ex;
                aa_ex = aa_fe;

                // update state of ex,wb local buffers
                layer->loc_x_ex = layer->loc_x_fe;
                layer->loc_w_ex = layer->loc_w_fe;
                layer->loc_y_wb = layer->loc_y_ex;
                layer->loc_y_ex = layer->loc_y_fe;

#ifndef CCN_CACHE
#ifdef CCN_DOUBLEBUF
                // switch double buffering state
                if (doublebuf_state_x_fe == 0) {
                    doublebuf_state_x_fe = 1;
                }
                else {
                    doublebuf_state_x_fe = 0;
                }
                if (bb_pipe==layer->ntile_nin-1 && doublebuf_state_y_fe == 0) {
                    doublebuf_state_y_fe = 1;
                }
                else if (bb_pipe==layer->ntile_nin-1 && doublebuf_state_y_fe == 1) {
                    doublebuf_state_y_fe = 0;
                }
                if (bb_pipe==layer->ntile_nin-1 && doublebuf_state_y_wb == 0) {
                    doublebuf_state_y_wb = 1;
                }
                else if (bb_pipe==layer->ntile_nin-1 && doublebuf_state_y_wb == 1) {
                    doublebuf_state_y_wb = 0;
                }
#endif /* CCN_DOUBLEBUF */
#endif /* ~CCN_CACHE */

        }

    }
#else /* ~CCN_TILING */
    // fetch stage
    DenseLayer_pipe_fe(layer, 0, 0);

    // execute stage
    DenseLayer_pipe_ex(layer, 0, 0);

    // write-back stage
    DenseLayer_pipe_wb(layer, 0, 0);
#endif /* CCN_TILING */

}
