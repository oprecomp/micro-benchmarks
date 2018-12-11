/*
 * ConvPoolLayer.c
 * Francesco Conti <f.conti@unibo.it>
 *
 * Copyright (C) 2016 ETH Zurich, University of Bologna
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include "linalg.h"
#include "tiling.h"
#include "ConvPoolLayer.h"
#ifdef CCN_HWCE_ACCEL
  #include "hwce.h"
#endif
#ifdef CCN_ENCRYPT
  #include "encryption.h"
#endif

#ifdef IPC_LINALG
#include "perf_monitor.h"
#endif

#ifndef NULL
    #define NULL ((void *) 0)
#endif

#ifndef PULP_CHIP
    #define PULP_CHIP -1
#endif

unsigned int dmaId = -1;

#ifdef CCN_TILING_LESSTIME
    #define _conv_tiling_init(); \
        unsigned char (*tile_grid_nof)[layer->ntile_nif][layer->ntile_h][layer->ntile_w] = (unsigned char (*)[layer->ntile_nif][layer->ntile_h][layer->ntile_w]) layer->tile_grid_nof; \
        unsigned char (*tile_grid_nif)[layer->ntile_nif][layer->ntile_h][layer->ntile_w] = (unsigned char (*)[layer->ntile_nif][layer->ntile_h][layer->ntile_w]) layer->tile_grid_nif; \
        unsigned char (*tile_grid_h)  [layer->ntile_nif][layer->ntile_h][layer->ntile_w] = (unsigned char (*)[layer->ntile_nif][layer->ntile_h][layer->ntile_w]) layer->tile_grid_h; \
        unsigned char (*tile_grid_w)  [layer->ntile_nif][layer->ntile_h][layer->ntile_w] = (unsigned char (*)[layer->ntile_nif][layer->ntile_h][layer->ntile_w]) layer->tile_grid_w; \
        int _fs = layer->filter_size; \
        int _nof = tile_grid_nof[aa][bb][ii][jj]; \
        int _nif = tile_grid_nif[aa][bb][ii][jj]; \
        int _h   = tile_grid_h  [aa][bb][ii][jj]; \
        int _w   = tile_grid_w  [aa][bb][ii][jj]; \
        int _oh  = _h-_fs+1; \
        int _ow  = _w-_fs+1;
#else /* ~CCN_TILING_LESSTIME */
    #define _conv_tiling_init(); \
        int _fs = layer->filter_size; \
        int _nof = (aa < layer->ntile_full_nof) ? layer->tiling_max_nof    : layer->tlast_nof; \
        int _nif = (bb < layer->ntile_full_nif) ? layer->tiling_max_nif    : layer->tlast_nif; \
        int _h   = (ii < layer->ntile_full_h  ) ? layer->tiling_max_height : layer->tlast_h; \
        int _w   = (jj < layer->ntile_full_w  ) ? layer->tiling_max_width  : layer->tlast_w; \
        int _oh = _h-_fs+1; \
        int _ow = _w-_fs+1;
#endif /* ~CCN_TILING_LESSTIME */

#define _conv_notiling_init(); \
    int _fs  = layer->filter_size; \
    int _nof = layer->n_out_feat; \
    int _nif = layer->n_in_feat; \
    int _h   = layer->height; \
    int _w   = layer->width; \
    int _oh  = _h-_fs+1; \
    int _ow  = _w-_fs+1;

/**
 *  Allocates a new ConvPoolLayer data structure and its fields (weight, bias,
 *  output feature maps).
 *
 *  @return a pointer to the new ConvPoolLayer data structure.
 *
 *  @param n_out_feat
 *      the number of output feature maps.
 *  @param n_in_feat
 *      the number of input feature maps.
 *  @param filter_size
 *      the size of the filters.
 *  @param height
 *      the height of the input feature maps.
 *  @param width
 *      the width of the input feature maps.
 *  @param activation
 *      1 if activation is tanh, 0 if no activation.
 *  @param *x
 *      a *mandatory* pointer to the input feature maps.
 *  @param *y
 *      an *optional* pointer to the already-allocated output feature maps. If
 *      NULL, ConvPoolLayer_new() will allocate y automatically.
 */
ConvPoolLayer *ConvPoolLayer_new(
#ifdef CCN_NOALLOC
    ConvPoolLayer *layer,
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
    data_t *loc_y3,
    data_t *loc_w0,
    data_t *loc_w1,
    int n_out_feat,
    int n_in_feat,
    int height,
    int width,
    int filter_size,
    int activation,
    int pool_size,
    int parallel_type,
    int tiling_max_nof,
    int tiling_max_nif,
    int tiling_max_height,
    int tiling_max_width,
    unsigned qf
) {

#ifndef CCN_NOALLOC
    // build ConvPoolLayer
    ConvPoolLayer *layer;
    layer = ccn_malloc(sizeof(ConvPoolLayer));
#endif /* ifndef CCN_NOALLOC */

    layer->name          = name;
    layer->n_in_feat     = n_in_feat;
    layer->n_out_feat    = n_out_feat;
    layer->filter_size   = filter_size;
    layer->height        = height;
    layer->width         = width;
    layer->activation    = activation;
    layer->parallel_type = parallel_type;
    layer->w             = w;
    layer->b             = b;
    layer->x             = x;
    layer->y             = y;
    layer->qf            = qf;
    layer->pool_size     = pool_size;

#ifndef CCN_CACHE
    layer->loc_x0 = loc_x0;
    layer->loc_y0 = loc_y0;
    layer->loc_x1 = loc_x1;
    layer->loc_y1 = loc_y1;
    layer->loc_y2 = loc_y2;
    layer->loc_y_tmp = loc_y3;
    layer->loc_w0 = loc_w0;
    layer->loc_w1 = loc_w1;

    layer->loc_b     = (data_t *) ccn_malloc(sizeof(data_t)*tiling_max_nof);
#endif /* ifndef CCN_CACHE */

    layer->tiling_max_nof    = tiling_max_nof;
    layer->tiling_max_nif    = tiling_max_nif;
    layer->tiling_max_height = tiling_max_height;
    layer->tiling_max_width  = tiling_max_width;

#ifdef CCN_TILING
    // define and record the number of tiles
    int ntile_nof = (n_out_feat % tiling_max_nof   ) ? n_out_feat / tiling_max_nof    + 1 : n_out_feat / tiling_max_nof;
    int ntile_nif = (n_in_feat  % tiling_max_nif   ) ? n_in_feat  / tiling_max_nif    + 1 : n_in_feat  / tiling_max_nif;

    // a little more complicated for H and W tiles: the last tile counts for (tiling_max_height-_fs+1)+height%(tiling_max_height-_fs+1),
    // then every other tile only for tiling_max_height-_fs+1
    int ntile_h;
    int ntile_w;
    int tlast_h;
    int tlast_w;
    if(height <= tiling_max_height) {
        ntile_h = 1;
    }
    else {
        // e.g. tiling_max_height = 7, height = 14, _fs = 5
        // e.g. tlast = 3+2 = 5
        tlast_h = (tiling_max_height-layer->filter_size+1) + height%(tiling_max_height-layer->filter_size+1);
        // e.g. ntile = 1 + 9/3 = 1+3 = 4
        ntile_h = 1 + (height-tlast_h) / (tiling_max_height-layer->filter_size+1);
    }
    if(width <= tiling_max_width) {
        ntile_w = 1;
    }
    else {
        // e.g. tiling_max_width = 20, width = 32, _fs = 5
        // e.g. tlast = 16+0 = 16
        tlast_w = (tiling_max_width-layer->filter_size+1) + width%(tiling_max_width-layer->filter_size+1);
        // e.g. ntile = 1 + 16/16 = 1+1 = 2
        ntile_w = 1 + (width-tlast_w) / (tiling_max_width-layer->filter_size+1);
    }

    layer->ntile_nof = ntile_nof;
    layer->ntile_nif = ntile_nif;
    layer->ntile_h   = ntile_h;
    layer->ntile_w   = ntile_w;

#ifdef CCN_TILING_LESSMEM
    layer->tlast_nof = n_out_feat % tiling_max_nof;
    layer->tlast_nif = n_in_feat  % tiling_max_nif;
    layer->tlast_h   = tlast_h;
    layer->tlast_w   = tlast_w;

    layer->ntile_full_nof = ntile_nof;
    layer->ntile_full_nif = ntile_nif;
    layer->ntile_full_h   = ntile_h;
    layer->ntile_full_w   = ntile_w;
#else /* ~CCN_TILING_LESSMEM */
    // allocate the tile grid in a flat fashion
    layer->tile_grid_nof = ccn_malloc(sizeof(unsigned char)*(ntile_nof+NB_PIPE_STAGE-1)*ntile_nif*ntile_h*ntile_w);
    layer->tile_grid_nif = ccn_malloc(sizeof(unsigned char)*(ntile_nof+NB_PIPE_STAGE-1)*ntile_nif*ntile_h*ntile_w);
    layer->tile_grid_h   = ccn_malloc(sizeof(unsigned char)*(ntile_nof+NB_PIPE_STAGE-1)*ntile_nif*ntile_h*ntile_w);
    layer->tile_grid_w   = ccn_malloc(sizeof(unsigned char)*(ntile_nof+NB_PIPE_STAGE-1)*ntile_nif*ntile_h*ntile_w);

    // cast the tile grid to a 4-dimensional array
    unsigned char (*tile_grid_nof)[ntile_nif][ntile_h][ntile_w] = layer->tile_grid_nof;
    unsigned char (*tile_grid_nif)[ntile_nif][ntile_h][ntile_w] = layer->tile_grid_nif;
    unsigned char (*tile_grid_h)  [ntile_nif][ntile_h][ntile_w] = layer->tile_grid_h;
    unsigned char (*tile_grid_w)  [ntile_nif][ntile_h][ntile_w] = layer->tile_grid_w;
#endif /* ~CCN_TILING_LESSMEM */

    // fill in the tile grid
    int aa, bb, ii, jj;
    for(aa=0; aa<layer->ntile_nof; aa++) {
        for(bb=0; bb<layer->ntile_nif; bb++) {
            for(ii=0; ii<layer->ntile_h; ii++) {
                for(jj=0; jj<layer->ntile_w; jj++) {

#ifdef CCN_TILING_LESSTIME
                    if(jj*(tiling_max_width-layer->filter_size+1) > width-tiling_max_width) {
                        tile_grid_w[aa][bb][ii][jj] = (unsigned char) tlast_w;
                    }
                    else {
                        tile_grid_w[aa][bb][ii][jj] = (unsigned char) tiling_max_width;
                    }

                    if(ii*(tiling_max_height-layer->filter_size+1) > height-tiling_max_height) {
                        tile_grid_h[aa][bb][ii][jj] = (unsigned char) tlast_h;
                    }
                    else {
                        tile_grid_h[aa][bb][ii][jj] = (unsigned char) tiling_max_height;
                    }

                    if(bb*tiling_max_nif > n_in_feat-tiling_max_nif) {
                        tile_grid_nif[aa][bb][ii][jj] = (unsigned char) n_in_feat % tiling_max_nif;
                    }
                    else {
                        tile_grid_nif[aa][bb][ii][jj] = (unsigned char) tiling_max_nif;
                    }

                    if(aa*tiling_max_nof > n_out_feat-tiling_max_nof) {
                        tile_grid_nof[aa][bb][ii][jj] = (unsigned char) n_out_feat % tiling_max_nof;
                    }
                    else {
                        tile_grid_nof[aa][bb][ii][jj] = (unsigned char) tiling_max_nof;
                    }
#else /* ~CCN_TILING_LESSTIME */
                    if(jj*(tiling_max_width-layer->filter_size+1) > width-tiling_max_width) {
                        layer->ntile_full_w = jj;
                    }

                    if(ii*(tiling_max_height-layer->filter_size+1) > height-tiling_max_height) {
                        layer->ntile_full_h = ii;
                    }

                    if(bb*tiling_max_nif > n_in_feat-tiling_max_nif) {
                        layer->ntile_full_nif = bb;
                    }

                    if(aa*tiling_max_nof > n_out_feat-tiling_max_nof) {
                        layer->ntile_full_nof = aa;
                    }
#endif /* ~CCN_TILING_LESSTIME */

                }
            }
        }
    }
#ifdef CCN_TILING_LESSTIME
    for(aa=layer->ntile_nof; aa<layer->ntile_nof+NB_PIPE_STAGE-1; aa++) {
        for(bb=0; bb<layer->ntile_nif; bb++) {
            for(ii=0; ii<layer->ntile_h; ii++) {
                for(jj=0; jj<layer->ntile_w; jj++) {
                    tile_grid_w  [aa][bb][ii][jj] = 0;
                    tile_grid_h  [aa][bb][ii][jj] = 0;
                    tile_grid_h  [aa][bb][ii][jj] = 0;
                    tile_grid_nif[aa][bb][ii][jj] = 0;
                    tile_grid_nif[aa][bb][ii][jj] = 0;
                    tile_grid_nof[aa][bb][ii][jj] = 0;
                    tile_grid_nof[aa][bb][ii][jj] = 0;
                }
            }
        }
    }
#endif /* CCN_TILING_LESSTIME */
#else /* ~CCN_TILING */
    // no tile grid
    int ntile_nof = n_out_feat;
    int ntile_nif = n_in_feat;
    int ntile_h   = height;
    int ntile_w   = width;
    layer->ntile_nof = ntile_nof;
    layer->ntile_nif = ntile_nif;
    layer->ntile_h   = ntile_h;
    layer->ntile_w   = ntile_w;
#endif /* ~CCN_TILING */

#ifdef TILING_DEBUG
    printf("[ConvPoolLayer %s] NOF grid:\n", layer->name);
    for(aa=0; aa<layer->ntile_nof; aa++) {
        for(bb=0; bb<layer->ntile_nif; bb++) {
            printf("aa=%d bb=%d\n", aa, bb);
            for(ii=0; ii<layer->ntile_h; ii++) {
                printf("  ");
                for(jj=0; jj<layer->ntile_w; jj++) {
                    printf("%d ", tile_grid_nof[aa][bb][ii][jj]);
                }
                printf("\n");
            }
        }
    }
    printf("[ConvPoolLayer %s] NIF grid:\n", layer->name);
    for(aa=0; aa<layer->ntile_nof; aa++) {
        for(bb=0; bb<layer->ntile_nif; bb++) {
            printf("aa=%d bb=%d\n", aa, bb);
            for(ii=0; ii<layer->ntile_h; ii++) {
                printf("  ");
                for(jj=0; jj<layer->ntile_w; jj++) {
                    printf("%d ", tile_grid_nif[aa][bb][ii][jj]);
                }
                printf("\n");
            }
        }
    }
    printf("[ConvPoolLayer %s] H grid:\n", layer->name);
    for(aa=0; aa<layer->ntile_nof; aa++) {
        for(bb=0; bb<layer->ntile_nif; bb++) {
            printf("aa=%d bb=%d\n", aa, bb);
            for(ii=0; ii<layer->ntile_h; ii++) {
                printf("  ");
                for(jj=0; jj<layer->ntile_w; jj++) {
                    printf("%d ", tile_grid_h[aa][bb][ii][jj]);
                }
                printf("\n");
            }
        }
    }
    printf("[ConvPoolLayer %s] W grid:\n", layer->name);
    for(aa=0; aa<layer->ntile_nof; aa++) {
        for(bb=0; bb<layer->ntile_nif; bb++) {
            printf("aa=%d bb=%d\n", aa, bb);
            for(ii=0; ii<layer->ntile_h; ii++) {
                printf("  ");
                for(jj=0; jj<layer->ntile_w; jj++) {
                    printf("%d ", tile_grid_w[aa][bb][ii][jj]);
                }
                printf("\n");
            }
        }
    }
#endif /* TILING_DEBUG */

#ifdef CCN_PULP_HWCE
#if PULP_CHIP==CHIP_FULMINE
    hwce_enable();
#endif /* ~CHIP_FULMINE */
#endif /* ~CCN_PULP_HWCE */

#ifdef CCN_HWCE_ACCEL
    hwce_config_init();
#endif

    return layer;

}

void ConvPoolLayer_delete(
    ConvPoolLayer *layer
) {
#ifndef CCN_CACHE
    ccn_free(layer->loc_w0);
    ccn_free(layer->loc_w1);
    ccn_free(layer->loc_b);
#endif /* ~CCN_CACHE */
#ifdef CCN_TILING
    ccn_free(layer->tile_grid_nof);
    ccn_free(layer->tile_grid_nif);
    ccn_free(layer->tile_grid_h);
    ccn_free(layer->tile_grid_w);
#endif /* ~CCN_TILING */
    ccn_free(layer);
}

static void ConvPoolLayer_pipe_fe(
    ConvPoolLayer *layer,
    int aa,
    int bb,
    int ii,
    int jj
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
        _conv_tiling_init()

        // x strides are h-fs+1, w-fs+1 due to tile overlap
        data_t *l2_x = ccn_get_tile_3d(
            layer->x,
            bb, ii, jj,
            layer->tiling_max_nif, layer->tiling_max_height, layer->tiling_max_width,
            layer->height, layer->width,
            0, _fs-1, _fs-1
        );

        data_t *l2_y = ccn_get_tile_3d(
            layer->y,
            aa, ii, jj,
            layer->tiling_max_nof, layer->tiling_max_height-_fs+1, layer->tiling_max_width-_fs+1,
            layer->height-_fs+1, layer->width-_fs+1,
            0, 0, 0
        );

#if PULP_CHIP == CHIP_MIA || PULP_CHIP == CHIP_PULP3 || PULP_CHIP == CHIP_FULMINE || PULP_CHIP == CHIP_HONEY
        data_t *l2_W = ccn_get_tile_2d(
            layer->w,
            aa, bb,
            layer->tiling_max_nof*MULTIPLE8(_fs*_fs), layer->tiling_max_nif*MULTIPLE8(_fs*_fs),
            layer->n_in_feat
        );
#else /* ~CHIP_MIA && ~CHIP_PULP3 && ~CHIP_FULMINE && ~CHIP_HONEY */
        data_t *l2_W = ccn_get_tile_2d(
            layer->w,
            aa, bb,
            layer->tiling_max_nof*_fs*_fs, layer->tiling_max_nif*_fs*_fs,
            layer->n_in_feat
        );
#endif /* ~CHIP_MIA && ~CHIP_PULP3 && ~CHIP_FULMINE && ~CHIP_HONEY */

#ifdef CCN_TILING_3D
        /* with no additional assumptions, the tiling grid is three-dimensional */
        // X tile copy-in
        ccn_memcpy_async_3d(
            layer->loc_x_fe, // pointers
            l2_x,
            _nif, // sizes
            _h,
            _w*sizeof(data_t),
            _h, // local strides
            _w*sizeof(data_t),
            layer->height, // remote strides
            layer->width*sizeof(data_t)
        );
#endif /* CCN_TILING_3D */
#ifdef CCN_TILING_2D
        /* Assuming that tiles are internally contiguous in the j feature map dimension,
           the tiling grid is two-dimensional.
           Moreover, _w=layer->width */
        // X tile copy-in
        ccn_memcpy_async_2d(
            layer->loc_x_fe, // pointers
            l2_x,
            _nif, // sizes
            _h*_w*sizeof(data_t),
            _h*_w*sizeof(data_t), // local strides
            layer->height*layer->width*sizeof(data_t) // remote strides
        );
#endif /* CCN_TILING_2D */
#ifdef CCN_TILING_1D
        /* Assuming that tiles are internally contiguous in the i,j feature map dimensions,
           the tiling grid is one-dimensional.
           Moreover, _h=layer->height,_w=layer->width */
        // X tile copy-in
        ccn_memcpy_async(
            layer->loc_x_fe, // pointers
            l2_x,
            _nif*_h*_w*sizeof(data_t)
        );
#endif /* CCN_TILING_1D */

        // W copy-in
#if PULP_CHIP == CHIP_MIA || PULP_CHIP == CHIP_PULP3 || PULP_CHIP == CHIP_FULMINE || PULP_CHIP == CHIP_HONEY
        for(int a=0; a<_nof; a++) {
            for(int b=0; b<_nif; b++) {
                ccn_memcpy_async(
                    layer->loc_w_fe + a*_nif*MULTIPLE4(_fs*_fs) + b*MULTIPLE4(_fs*_fs),
                    l2_W + a*layer->n_in_feat*MULTIPLE8(_fs*_fs) + b*MULTIPLE8(_fs*_fs),
                    sizeof(data_t)*_fs*_fs
                );
            }
        }
#else /* ~CHIP_MIA && ~CHIP_PULP3 */
        if(layer->parallel_type != PARALLEL_HWCE) {
            for(int a=0; a<_nof; a++) {
                for(int b=0; b<_nif; b++) {
                    ccn_memcpy_async(
                        layer->loc_w_fe + a*_nif*_fs*_fs + b*_fs*_fs,
                        l2_W + a*layer->n_in_feat*_fs*_fs + b*_fs*_fs,
                        sizeof(data_t)*_fs*_fs
                    );
                }
            }
        }
        else {
            for(int a=0; a<_nof; a++) {
                for(int b=0; b<_nif; b++) {
                    ccn_memcpy_async(
                        layer->loc_w_fe + a*_nif*MULTIPLE4(_fs*_fs) + b*MULTIPLE4(_fs*_fs),
                        l2_W + a*layer->n_in_feat*_fs*_fs + b*_fs*_fs,
                        sizeof(data_t)*_fs*_fs
                    );
                }
            }
        }
#endif /* ~CHIP_MIA && ~CHIP_PULP3 */

#ifdef FAKEDMA
        // ccn_memcpy_wait();
#endif /* FAKEDMA */

#ifdef FETCH_CHECKSUM
        int32_t sum_x = 0;
        int32_t sum_W = 0;
        int32_t sum_y = 0;
        for(int i=0; i<_nif*_h*_w; i++) {
            sum_x += layer->loc_x_fe[i];
        }
        if(layer->parallel_type == PARALLEL_HWCE) {
            for(int a=0; a<_nof; a++) {
                for(int b=0; b<_nif; b++) {
                    for(int i=0; i<_fs*_fs; i++) {
                        sum_W += layer->loc_w_fe[a*_nif*MULTIPLE4(_fs*_fs) + b*MULTIPLE4(_fs*_fs) + i];
                    }
                }
            }
        }
        else {
            for(int i=0; i<_nof*_nif*_fs*_fs; i++) {
                sum_W += layer->loc_w_fe[i];
            }
        }
        printf("[ConvPoolLayer %s] Fetch checksum %d,%d,%d,%d: x=%d W=%d\n", layer->name, aa, bb, ii, jj, sum_x, sum_W);
#endif /* FETCH_CHECKSUM */

    }
#ifdef FETCH_PROFILE
    perf_stop();
    int t0 = perf_get_cycles();
    printf("[ConvPoolLayer %s] Fetch profiling: %d\n", layer->name, t0);
#endif /* FETCH_PROFILE */

}

static void ConvPoolLayer_pipe_ex(
    ConvPoolLayer *layer,
    int aa,
    int bb,
    int ii,
    int jj
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
    _conv_tiling_init()
#else /* ~CCN_TILING */
    _conv_notiling_init()
#endif /* ~CCN_TILING */

#ifndef CCN_CACHE

    data_t *_x = layer->loc_x_ex;

// #define USE_TMP_BUFFER
#ifdef USE_TMP_BUFFER
    data_t *_y;
    data_t *_y2 = layer->loc_y_ex;
    if(bb == layer->ntile_nif-1) {
        _y  = layer->loc_y_tmp;
    }
    else {
        _y  = layer->loc_y_ex;
    }
#else /* USE_TMP_BUFFER */
    data_t *_y  = layer->loc_y_ex;
#endif /* USE_TMP_BUFFER */

    data_t *_W = layer->loc_w_ex;

#ifndef CCN_DOUBLEBUF
    // wait for the end of the fetch stage if not doing double buffering
    // ccn_memcpy_wait();
    // #pragma omp barrier
#endif /* ~CCN_DOUBLEBUF */

#else /* CCN_CACHE */

    data_t *_x = ccn_get_tile_3d(
        layer->x,
        bb, ii, jj,
        layer->tiling_max_nif, layer->tiling_max_height, layer->tiling_max_width,
        layer->height, layer->width,
        0, _fs-1, _fs-1
    );

    data_t *_y = ccn_get_tile_3d(
        layer->y,
        aa, ii, jj,
        layer->tiling_max_nof, layer->tiling_max_height-_fs+1, layer->tiling_max_width-_fs+1,
        layer->height-_fs+1, layer->width-_fs+1,
        0, 0, 0
    );

    // we assume weights to be contiguous
    data_t *_W = ccn_get_tile_2d(
        layer->w,
        aa, bb,
        layer->tiling_max_nof, layer->tiling_max_nif,
        layer->n_in_feat*_fs*_fs
    )

#endif /* CCN_CACHE */

    // loop over output features
    for(int a=0; a<_nof; a++) {

        // #pragma omp barrier
        // y_a[i,j] = b_a for every output feature a, pixel (i,j)
        if(bb == 0) {
            data_t _b = layer->b[aa*layer->tiling_max_nof+a];
            // #pragma omp parallel for
            for(int i=0; i<_oh*_ow; i++) {
                _y[a*_oh*_ow+i] = _b;
            }
        }

        // convolution "core"
#ifndef NOCOMPUTATION
#ifndef CCN_CACHE
#ifdef CCN_PULP_HWCE
        if(layer->parallel_type == PARALLEL_HWCE)
            linalg_2dconv_hwce(_W, _x, _y, _h, _w, _fs, a, _nif, layer->parallel_type, layer->qf);
        else
            linalg_2dconv(_W, _x, _y, _h, _w, _fs, a, _nif, layer->parallel_type, layer->qf);
#else /* ~CCN_PULP_HWCE */
        linalg_2dconv(_W, _x, _y, _h, _w, _fs, a, _nif, layer->parallel_type, layer->qf);
#endif /* ~CCN_PULP_HWCE */
#else /* CCN_CACHE */
        linalg_2dconv(_W, _x, _y, layer->height, layer->width, _fs, a, layer->n_in_feat, layer->parallel_type, layer->qf);
#endif /* CCN_CACHE */
#endif /* NOCOMPUTATION */

#ifdef DETAILED_DEBUG
        #pragma omp master
        {
            for(int i=0; i<_oh; i++) {
                for(int j=0; j<_ow; j++) {
                    printf("(%d,%d): %04x\n", i, j, _y[a*_oh*_ow+i*_ow+j] & 0xffff);
                }
            }
        }
        #pragma omp barrier
#endif /* DETAILED_DEBUG */

        if(bb == layer->ntile_nif-1) {

            int _ps = layer->pool_size;
            int _oph, _opw;
            if(_ps == 2) {
                _oph = _oh >> 1;
                _opw = _ow >> 1;
            }
            else if(_ps == 4) {
                _oph = _oh >> 2;
                _opw = _ow >> 2;
            }
            else {
                _oph = _oh / _ps;
                _opw = _ow / _ps;
            }

            // #pragma omp parallel \
            //         firstprivate(_y2,_ps)
            // {

                if(layer->activation == ACTIVATION_TANH) {
                    // #pragma omp for \
                    //         collapse(2)
                    for (int i=0; i<_oh; i++) {
                        for (int j=0; j<_ow; j++) {
                            _y[a*_oh*_ow+i*_ow+j] = ccn_tanh(_y[a*_oh*_ow+i*_ow+j]);
                        }
                    }
                }
                else if(layer->activation == ACTIVATION_RELU) {
                    // #pragma omp for \
                    //         collapse(2)
                    for (int i=0; i<_oh; i++) {
                        for (int j=0; j<_ow; j++) {
                            _y[a*_oh*_ow+i*_ow+j] = ccn_relu(_y[a*_oh*_ow+i*_ow+j]);
                        }
                    }
                }

                // #pragma omp for \
                //             collapse(2)
                for(int i=0; i<_oph; i++) {
                    for(int j=0; j<_opw; j++) {
                        data_t max = -DATA_T_MAX;
                        for(int i1=0; i1<_ps; i1++) {
                            for(int j1=0; j1<_ps; j1++) {
                                data_t xtmp = _y[((a*_oh)+(i*_ps+i1))*_ow+(j*_ps+j1)];
                                if(xtmp > max)
                                    max = xtmp;
                            }
                        }
  #ifdef USE_TMP_BUFFER
                        _y2[a*_oph*_opw+i*_opw+j] = max;
  #else /* USE_TMP_BUFFER */
                        _y[a*_oph*_opw+i*_opw+j] = max;
  #endif /* USE_TMP_BUFFER */
                    }
                }

            // }

        }

#ifdef CCN_ENCRYPT
#ifdef CCN_ENCRYPT_HWCRYPT
        hwcrypt_enable();
        ccn_encrypt_aes_xts_hwcrypt_config();
        ccn_encrypt_aes_xts_hwcrypt(_y, _y, _nof*_oh*_ow >> 1); // number of 32-bit words
#ifdef CCN_HWCE_ACCEL
        hwce_enable();
#endif /* CCN_HWCE_ACCEL */
#else /* ~CCN_ENCRYPT_HWCRYPT */
        ccn_encrypt_aes_xts(_y, _y, _nof*_oh*_ow*sizeof(data_t)); // number of bytes
#endif /* ~CCN_ENCRYPT_HWCRYPT */
#endif /* CCN_ENCRYPT */

#ifdef INTERM_CHECKSUM
        // #pragma omp barrier
        // #pragma omp master
        {
            int i, sum=0;
            printf("[ConvPoolLayer %s] Intermediate checksum tile %d,%d,%d,%d, a=%d: ", layer->name, aa,bb,ii,jj, print_flag);
            sum=0;
            data_t *xt = _x + a*_nif*_h*_w;
            for(i=0; i<_nif*_h*_w; i++){
                sum+=xt[i]; // FIXME: it should be _x, not xt. but if i do it here => WRONG CHECKSUM (BIG MISTERY)
            }
            printf("xsum=%d, ", sum);
            sum=0;
            data_t *wt = _W + a*_nif*MULTIPLE4(_fs*_fs);
            for(i=0; i<_nif*_fs*_fs; i++) {
                sum+=wt[i];
            }
            printf("wsum=%d, ", sum);
            sum=0;
            data_t *yt = _y + a*_oh*_ow;
            for(i=0; i<_oh*_ow; i++) {
                sum+=yt[i];
            }
            print_flag++;
            printf("ysum=%d\n", sum);
            printf("    xptr=%08x, wptr=%08x, yptr=%08x\n", _x, _W, _y);
        }
        // #pragma omp barrier
#endif

#ifdef CCN_CACHE
        _y += (layer->height-_fs+1)*(layer->width-_fs+1);
        _W += layer->n_in_feat*_fs*_fs;
#endif /* CCN_CACHE */

    } /* for(int a=a_start; a<_nof; a++) */

#ifdef TILE_CHECKSUM
    // #pragma omp barrier
    // #pragma omp master
    {
        int _oph, _opw;
        int _ps = layer->pool_size;
        data_t *_y2 = layer->loc_y_ex;
        if(_ps == 2) {
            _oph = _oh >> 1;
            _opw = _ow >> 1;
        }
        else if(_ps == 4) {
            _oph = _oh >> 2;
            _opw = _ow >> 2;
        }
        else {
            _oph = _oh / _ps;
            _opw = _ow / _ps;
        }
        int i, sum=0;
        printf("[ConvPoolLayer %s] Tile checksum %d,%d,%d,%d: ", layer->name, aa,bb,ii,jj);
        sum=0;
        for(i=0; i<_nif*_h*_w; i++){
            sum+=_x[i];
        }
        printf("xsum=%d, ", sum);
        sum=0;
        for(i=0; i<_nof*_nif*_fs*_fs; i++) {
            sum+=_W[i];
        }
        printf("wsum=%d, ", sum);
        sum=0;
        for(i=0; i<_nof*_oh*_ow; i++) {
            sum+=_y[i];
        }
        // print_flag++;
        printf("ysum=%d\n", sum);
        sum=0;
        for(i=0; i<_nof*_oph*_opw; i++) {
            sum+=_y2[i];
        }
        // print_flag++;
        printf("y2sum=%d\n", sum);
        printf("    xptr=%08x, wptr=%08x, yptr=%08x\n", _x, _W, _y);
    }
    // #pragma omp barrier
#endif
    }
#ifdef EXECUTE_PROFILE
    perf_stop();
    int t0 = perf_get_cycles();
    printf("[ConvPoolLayer %s] Execute profiling: %d\n", layer->name, t0);
#endif /* EXECUTE_PROFILE */

}

static void ConvPoolLayer_pipe_wb(
    ConvPoolLayer *layer,
    int aa,
    int bb,
    int ii,
    int jj
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
        _conv_tiling_init();

        data_t *l2_y;
        // if last bb tile, then l2_y points to the max-pooled output
        // if(bb != layer->ntile_nif-1) {
        //     l2_y = ccn_get_tile_3d(
        //         layer->y,
        //         aa, ii, jj,
        //         layer->tiling_max_nof, layer->tiling_max_height-_fs+1, layer->tiling_max_width-_fs+1,
        //         layer->height-_fs+1, layer->width-_fs+1,
        //         0, 0, 0
        //     );
        // }
        // else {
        int _tph, _tpw;
        int _ph, _pw;
        int _oph, _opw;
        if(bb == layer->ntile_nif-1) {
            int _ps = layer->pool_size;
            if(_ps == 2) {
                _tph = (layer->tiling_max_height-_fs+1) >> 1;
                _tpw = (layer->tiling_max_width-_fs +1) >> 1;
                _ph = (layer->height-_fs+1) >> 1;
                _pw = (layer->width-_fs +1) >> 1;
                _oph = _oh >> 1;
                _opw = _ow >> 1;
            }
            else if(_ps == 4) {
                _tph = (layer->tiling_max_height-_fs+1) >> 2;
                _tpw = (layer->tiling_max_width-_fs +1) >> 2;
                _ph = (layer->height-_fs+1) >> 2;
                _pw = (layer->width-_fs +1) >> 2;
                _oph = _oh >> 2;
                _opw = _ow >> 2;
            }
            else {
                _tph = (layer->tiling_max_height-_fs+1) / _ps;
                _tpw = (layer->tiling_max_width-_fs +1) / _ps;
                _ph = (layer->height-_fs+1) / _ps;
                _pw = (layer->width-_fs +1) / _ps;
                _oph = _oh / _ps;
                _opw = _ow / _ps;
            }
            l2_y = ccn_get_tile_3d(
                layer->y,
                aa, ii, jj,
                layer->tiling_max_nof, _tph, _tpw,
                _ph, _pw,
                0, 0, 0
            );

#ifdef WRITEBACK_CHECKSUM
            int32_t sum = 0;
            for(int i=0; i<_nof*_oh*_ow; i++) {
                sum += layer->loc_y_tmp[i];
            }
            printf("[ConvLayer %s] Writeback checksum %d,%d,%d,%d: %d\n", layer->name, aa, bb, ii, jj, sum);
            sum = 0;
            for(int i=0; i<_nof*_oph*_opw; i++) {
                sum += layer->loc_y_wb[i];
            }
            // printf("[ConvPoolLayer %s] Writeback checksum %d,%d,%d,%d: %d\n", layer->name, aa, bb, ii, jj, sum);
#endif /* WRITEBACK_CHECKSUM */

#ifdef WRITEBACK_DEBUG
            printf("[ConvPoolLayer %s] Writeback debug CONV %d,%d,%d,%d:\n", layer->name, aa, bb, ii, jj);
            for(int i=0; i<_nof; i++) {
                for(int j=0; j<_oh; j++) {
                    for(int k=0; k<_ow; k++) {
                        printf("  (%d,%d,%d): %04x\n", i,j,k, layer->loc_y_tmp[i*_oh*_ow+j*_ow+k] & 0xffff);
                    }
                }
            }
            printf("[ConvPoolLayer %s] Writeback debug POOL %d,%d,%d,%d:\n", layer->name, aa, bb, ii, jj);
            for(int i=0; i<_nof; i++) {
                for(int j=0; j<_oph; j++) {
                    for(int k=0; k<_opw; k++) {
                        printf("  (%d,%d,%d): %04x\n", i,j,k, layer->loc_y_wb[i*_oph*_opw+j*_opw+k] & 0xffff);
                    }
                }
            }
#endif /* WRITEBACK_DEBUG */

#ifdef CCN_TILING_3D
            /* with no additional assumptions, the tiling grid is three-dimensional */
            // Y tile copy-out
            ccn_memcpy_async_3d(
                l2_y, // pointers
                layer->loc_y_wb,
                _nof, // sizes
                _oph,
                _opw*sizeof(data_t),
                _ph, // remote strides
                _pw*sizeof(data_t),
                _oph, // local strides
                _opw*sizeof(data_t)
            );
#endif /* CCN_TILING_3D */
#ifdef CCN_TILING_2D
            /* Assuming that tiles are internally contiguous in the j feature map dimension,
               the tiling grid is two-dimensional.
               Moreover, _w=layer->width */
            // Y tile copy-in
            ccn_memcpy_async_2d(
                l2_y, // pointers
                layer->loc_y_wb,
                _nof, // sizes
                _oph*_opw*sizeof(data_t),
                _ph*_pw*sizeof(data_t), // local strides
                _oph*_opw*sizeof(data_t) // remote strides
            );
#endif /* CCN_TILING_2D */
#ifdef CCN_TILING_1D
            /* Assuming that tiles are internally contiguous in the i,j feature map dimensions,
               the tiling grid is one-dimensional.
               Moreover, _h=layer->height,_w=layer->width */
            // Y tile copy-in
            ccn_memcpy_async(
                l2_y, // pointers
                layer->loc_y_wb,
                _nof*_oph*_opw*sizeof(data_t)
            );
#endif /* CCN_TILING_1D */
        }

#ifdef FAKEDMA
        // ccn_memcpy_wait();
#endif /* FAKEDMA */

    }
#ifdef WRITEBACK_DEBUG
    #pragma omp barrier
#endif
#ifdef WRITEBACK_PROFILE
    perf_stop();
    int t0 = perf_get_cycles();
    printf("[ConvPoolLayer %s] Writeback profiling: %d\n", layer->name, t0);
#endif /* WRITEBACK_PROFILE */
}

/**
 *  Executes the given ConvPoolLayer, i.e. computes its outputs given the inputs
 *  defined in the data structure.
 *  The ConvPoolLayer computes the output of a convolutional network layer with
 *  3d inputs and outputs (an array of 2d feature maps).
 *
 *  @param *layer
 *      a pointer to the ConvPoolLayer data structure to execute.
 */
void ConvPoolLayer_exec(ConvPoolLayer *layer) {

    // ConvPoolLayer_exec is now organized as a pipeline with the following stages
    //   fetch      (fe) : DMA in of a tile
    //   execute    (ex) : execution of layer
    //   write-back (wb) : DMA out of a tile
    // all indeces have a fetch, execute and write-back version

    int aa_pipe,bb_pipe,ii_pipe,jj_pipe;

    int aa_fe = -1, bb_fe = -1, ii_fe = -1, jj_fe = -1;
    int aa_ex = -1, bb_ex = -1, ii_ex = -1, jj_ex = -1;
    int aa_wb = -1, bb_wb = -1, ii_wb = -1, jj_wb = -1;

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
    memset(layer->loc_w0, 0, sizeof(data_t)*layer->tiling_max_nof*layer->tiling_max_nif*MULTIPLE4(layer->filter_size*layer->filter_size));
    memset(layer->loc_w1, 0, sizeof(data_t)*layer->tiling_max_nof*layer->tiling_max_nif*MULTIPLE4(layer->filter_size*layer->filter_size));

#ifdef CCN_TILING
    for(aa_pipe=0; aa_pipe<layer->ntile_nof+NB_PIPE_STAGE-1; aa_pipe++) {

        for(ii_pipe=0; ii_pipe<layer->ntile_h; ii_pipe++) {

            for(jj_pipe=0; jj_pipe<layer->ntile_w; jj_pipe++) {

                for(bb_pipe=0; bb_pipe<layer->ntile_nif; bb_pipe++) {

                    // update state of fe indeces
                    if(jj_pipe<layer->ntile_w) {
                        jj_fe = jj_pipe;
                        ii_fe = ii_pipe;
                        bb_fe = bb_pipe;
                        aa_fe = aa_pipe;
                    }
                    else {
                        jj_fe = -1;
                        ii_fe = -1;
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
                    printf("[ConvPoolLayer %s pipe] aa=%d bb=%d ii=%d jj=%d\n", layer->name, aa_pipe, bb_pipe, ii_pipe, jj_pipe);
                    printf("  fe: aa=%d bb=%d ii=%d jj=%d\n", aa_fe, bb_fe, ii_fe, jj_fe);
                    printf("  ex: aa=%d bb=%d ii=%d jj=%d\n", aa_ex, bb_ex, ii_ex, jj_ex);
                    printf("  wb: aa=%d bb=%d ii=%d jj=%d\n", aa_wb, bb_wb, ii_wb, jj_wb);
                    printf("  doublebuf states: %d %d %d\n", doublebuf_state_x_fe, doublebuf_state_y_fe, doublebuf_state_y_wb);
                    printf("\n");
#endif PIPE_DEBUG
#ifdef PIPE_PROFILE
                    reset_timer();
                    start_timer();
#endif /* PIPE_PROFILE */
#ifndef DISABLE_OPENMP
                    #pragma omp parallel num_threads(3)
#endif
                    {
                        // fetch stage
#ifndef DISABLE_OPENMP
                        if(omp_get_thread_num() == THREAD_FE)
#endif
                            ConvPoolLayer_pipe_fe(layer, aa_fe, bb_fe, ii_fe, jj_fe);

                        // execute stage
#ifndef DISABLE_OPENMP
                        if(omp_get_thread_num() == THREAD_EX)
#endif
                            ConvPoolLayer_pipe_ex(layer, aa_ex, bb_ex, ii_ex, jj_ex);

                        // write-back stage
#ifndef DISABLE_OPENMP
                        if(omp_get_thread_num() == THREAD_WB)
#endif
                            ConvPoolLayer_pipe_wb(layer, aa_wb, bb_wb, ii_wb, jj_wb);
                    }
#ifdef PIPE_PROFILE
                    stop_timer();
                    int t0 = get_time();
                    reset_timer();
                    printf("[ConvPoolLayer %s] Pipe profiling: %d\n", layer->name, t0);
#endif /* PIPE_PROFILE */

                    // update state of ex,wb indeces
                    jj_wb = jj_ex;
                    jj_ex = jj_fe;
                    ii_wb = ii_ex;
                    ii_ex = ii_fe;
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
                    if (doublebuf_state_y_fe == 0) {
                        doublebuf_state_y_fe = 1;
                    }
                    else if (doublebuf_state_y_fe == 1) {
                        doublebuf_state_y_fe = 2;
                    }
                    else {
                        doublebuf_state_y_fe = 0;
                    }
                    if (doublebuf_state_y_wb == 0) {
                        doublebuf_state_y_wb = 1;
                    }
                    else if (doublebuf_state_y_wb == 1) {
                        doublebuf_state_y_wb = 2;
                    }
                    else {
                        doublebuf_state_y_wb = 0;
                    }
#endif /* CCN_DOUBLEBUF */
#endif /* ~CCN_CACHE */

                }

            }

        }

    }
#else /* ~CCN_TILING */
    // fetch stage
    ConvPoolLayer_pipe_fe(layer, 0, 0, 0, 0);

    // execute stage
    ConvPoolLayer_pipe_ex(layer, 0, 0, 0, 0);

    // write-back stage
    ConvPoolLayer_pipe_wb(layer, 0, 0, 0, 0);
#endif /* CCN_TILING */

}
