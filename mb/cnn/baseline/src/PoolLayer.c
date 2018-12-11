/* 
 * PoolLayer.c
 * Francesco Conti <f.conti@unibo.it>
 *
 * Copyright (C) 2015 ETH Zurich, University of Bologna
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include "PoolLayer.h"

#define _nf   nfeat_tile
#define _h    height_tile
#define _w    width_tile
#define _oh   layer->out_height
#define _ow   layer->out_width
#define _ps   layer->pool_stride

#define X(k,i,j) x[((k*layer->height)+i)*layer->width+j]
#define Y(k,i,j) y[((k*layer->out_height)+i)*layer->out_width+j]

/**
 *  Allocates a new PoolLayer data structure and its output feature maps.
 *
 *  @return a pointer to the new PoolLayer data structure.
 *
 *  @param n_feat
 *      the number of input feature maps.
 *  @param pool_stride
 *      the pooling factor.
 *  @param height
 *      the height of the input feature maps.
 *  @param width
 *      the width of the input feature maps.
 *  @param out_height
 *      the height of the output feature maps.
 *  @param out_width
 *      the width of the output feature maps.
 *  @param *x
 *      a *mandatory* pointer to the input feature maps.
 *  @param *y
 *      an *optional* pointer to the already-allocated output feature maps. If
 *      NULL, PoolLayer_new() will allocate y automatically.
 */
PoolLayer *PoolLayer_new(
#ifdef CCN_NOALLOC
    PoolLayer *layer,
#endif /* CCN_NOALLOC */
    data_t *x,
    data_t *y,
    data_t *loc_x0,
    data_t *loc_x1,
    data_t *loc_y0,
    data_t *loc_y1,
    int n_feat,
    int pool_stride,
    int height,
    int width,
    int tiling_max_nfeat,
    int tiling_max_height,
    int tiling_max_width,
    int parallel_type
) {

#ifndef CCN_NOALLOC 
    // build PoolLayer
    PoolLayer *layer;
    layer = ccn_malloc(sizeof(PoolLayer));
#endif /* CCN_NOALLOC */

    layer->n_feat      = n_feat;
    layer->pool_stride = pool_stride;
    layer->height      = height;
    layer->width       = width;
    layer->out_height  = height/pool_stride + height%pool_stride;
    layer->out_width   = width /pool_stride + width %pool_stride;
    layer->x           = x;
    layer->y           = y;

#ifndef CCN_CACHE
    layer->loc_x0 = loc_x0;
    layer->loc_y0 = loc_y0;
    layer->loc_x1 = loc_x1;
    layer->loc_y1 = loc_y1;
#endif /* ifndef CCN_CACHE */

    layer->tiling_max_nfeat  = tiling_max_nfeat;
    layer->tiling_max_height = tiling_max_height;
    layer->tiling_max_width  = tiling_max_width;

    layer->parallel_type = parallel_type;

    return layer;

}

void PoolLayer_delete(PoolLayer *layer) {
    free(layer);
}

static void PoolLayer_tile_loop(PoolLayer *layer, int nfeat_tile, int height_tile, int width_tile, int ii, int jj, int kk, int *doublebuf) {

    int i,j,k,i1,j1;
    data_t max, xtmp;

    data_t *_x;
    data_t *_y;
    data_t *l2_x, *l2_y;
    int sum;

    // if(*doublebuf) {
        _x = layer->loc_x0;
        _y = layer->loc_y0;
    // }
    // else {
    //     _x = layer->loc_x1;
    //     _y = layer->loc_y1;
    // }

    l2_x = layer->x + (ii*_h+jj)*_w+kk;
    l2_y = layer->y + (ii*_oh+jj)*_ow+kk;

#ifndef CCN_CACHE
    // DMA-in
    // #pragma omp master
    {
        ccn_memcpy(_x, l2_x, sizeof(data_t)*_nf*_h*_w);
    }
    // #pragma omp barrier
#else /* ifdef CCN_CACHE */
    _x = l2_x;
    _y = l2_y;
#endif /* ifdef CCN_CACHE */

#ifdef INTERM_CHECKSUM
    // #pragma omp master
    {
        int sum = 0;
        for(k=0; k<_nf; k++) {
            for(i=0; i<_oh; i++) {
                for(j=0; j<_ow; j++) {
                    for(i1=0; i1<_ps; i1++) {
                        for(j1=0; j1<_ps; j1++) {
                            sum += _x[((k*_h)+(i*_ps+i1))*_w+(j*_ps+j1)];
                        }
                    }
                }
            }
        }
        printf("[PoolLayer] in : %d\n", sum);
    }
    // #pragma omp barrier
#endif

    if(layer->parallel_type == PARALLEL_FEAT) {
        #pragma omp parallel for
        for(k=0; k<_nf; k++) {
            for(i=0; i<_oh; i++) {
                for(j=0; j<_ow; j++) {
                    max = -DATA_T_MAX;
                    for(i1=0; i1<_ps; i1++) {
                        for(j1=0; j1<_ps; j1++) {
                            xtmp = _x[((k*_h)+(i*_ps+i1))*_w+(j*_ps+j1)];
                            if(xtmp > max)
                                max = xtmp;
                        }
                    }
                    _y[((k*_oh)+i)*_ow+j] = max;
                }
            }
        }
    }
    else {
        for(k=0; k<_nf; k++) {
            #pragma omp parallel for
            for(i=0; i<_oh; i++) {
                for(j=0; j<_ow; j++) {
                    max = -DATA_T_MAX;
                    for(i1=0; i1<_ps; i1++) {
                        for(j1=0; j1<_ps; j1++) {
                            xtmp = _x[((k*_h)+(i*_ps+i1))*_w+(j*_ps+j1)];
                            if(xtmp > max)
                                max = xtmp;
                        }
                    }
                    _y[((k*_oh)+i)*_ow+j] = max;
                }
            }
        }
    }

#ifdef INTERM_CHECKSUM
    // #pragma omp master
    {
        int sum = 0;
        for(k=0; k<_nf; k++) {
            for(i=0; i<_oh; i++) {
                for(j=0; j<_ow; j++) {
                    sum += _y[((k*_oh)+i)*_ow+j];
                }
            }
        }
        printf("[PoolLayer] out : %d\n", sum);
    }
    // #pragma omp barrier
#endif

#ifndef CCN_CACHE
    // DMA-out
    // #pragma omp master
    {
        ccn_memcpy_async(l2_y, _y, sizeof(data_t)*_nf*_oh*_ow);
    }
    // #pragma omp barrier
#endif /* ifndef CCN_CACHE */

    // *doublebuf = (*doublebuf == 0) ? 1 : 0;

}

/**
 *  Executes the given PoolLayer, i.e. computes its outputs given the inputs
 *  defined in the data structure.
 *  The PoolLayer reduces the size of the feature maps by max-pooling.
 *
 *  @param *layer
 *      a pointer to the PoolLayer data structure to execute.
 */

void PoolLayer_exec(PoolLayer *layer) {

    int i,j, ii,jj,kk;
    int max_nfeat_tile = layer->tiling_max_nfeat;
    int max_height_tile = layer->tiling_max_height;
    int max_width_tile = layer->tiling_max_width;

    int nfeat_int  = layer->n_feat;
    int height_int = layer->height;
    int width_int  = layer->width;
    int nfeat_tile;
    int doublebuf = 0;

#ifdef CCN_TILING
    // normal nfeat
    for(ii=0; nfeat_int>=max_nfeat_tile; ii+=max_nfeat_tile) {
        int height_tile;
        nfeat_tile = max_nfeat_tile;
        // normal height
        for(jj=0; height_int>=max_height_tile; jj+=max_height_tile) {
            int width_tile;
            height_tile = max_height_tile;
            // normal width
            for(kk=0; width_int>=max_width_tile; kk+=max_width_tile) {
                width_tile = max_width_tile;
                PoolLayer_tile_loop(layer, nfeat_tile, height_tile, width_tile, ii, jj, kk, &doublebuf);
                width_int -= width_tile;
            }
            // last width
            if (width_int > 0) {
                width_tile = width_int;
                kk += width_tile;
                PoolLayer_tile_loop(layer, nfeat_tile, height_tile, width_tile, ii, jj, kk, &doublebuf);
            }
            height_int -= height_tile;
            width_int = layer->width;
        }
        // last height
        if (height_int > 0) {
            int width_tile;
            height_tile = height_int;
            for(kk=0; width_int>=max_width_tile; kk+=max_width_tile) {
                width_tile = max_width_tile;
                PoolLayer_tile_loop(layer, nfeat_tile, height_tile, width_tile, ii, jj, kk, &doublebuf);
                width_int -= width_tile;
            }
            if (width_int > 0) {
                width_tile = width_int;
                PoolLayer_tile_loop(layer, nfeat_tile, height_tile, width_tile, ii, jj, kk, &doublebuf);
            }
            width_int = layer->width;
        }
        nfeat_int -= nfeat_tile;
        height_int = layer->height;
    }
    // last nfeat
    if (nfeat_int > 0) {
        int height_tile;
        nfeat_tile = nfeat_int;
        // normal height
        for(jj=0; height_int > max_height_tile-1; jj+=max_height_tile) {
            unsigned int sptr;
            int width_tile;
            height_tile = max_height_tile;
            // normal width
            for(kk=0; width_int>=max_width_tile; kk+=max_width_tile) {
                width_tile = max_width_tile;
                PoolLayer_tile_loop(layer, nfeat_tile, height_tile, width_tile, ii, jj, kk, &doublebuf);
                width_int -= width_tile;
            }
            // last width
            if (width_int > 0) {
                width_tile = width_int;
                PoolLayer_tile_loop(layer, nfeat_tile, height_tile, width_tile, ii, jj, kk, &doublebuf);
            }
            height_int -= height_tile;
            width_int = layer->width;
        }
        // last height
        if (height_int > 0) {
            int width_tile;
            height_tile = height_int;
            for(kk=0; width_int>=max_width_tile; kk+=max_width_tile) {
                width_tile = max_width_tile;
                PoolLayer_tile_loop(layer, nfeat_tile, height_tile, width_tile, ii, jj, kk, &doublebuf);
                width_int -= width_tile;
            }
            if (width_int > 0) {
                width_tile = width_int;
                PoolLayer_tile_loop(layer, nfeat_tile, height_tile, width_tile, ii, jj, kk, &doublebuf);
            }
            width_int = layer->width;
        }
        height_int = layer->height;
    }
#else /* ifndef CCN_TILING */
        PoolLayer_tile_loop(layer, nfeat_int, height_int, width_int, 0, 0, 0, &doublebuf);
#endif /* ifndef CCN_TILING */

}

