/* 
 * huffman.c
 * Francesco Conti <f.conti@unibo.it>
 *
 * Code derivative from http://huffman.sourceforge.net
 * Copyright (C) 2003  Douglas Ryan Richardson
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include "huffman.h"

int huffman_decode(
    const unsigned char *encoded_buf,
    unsigned int        encoded_buf_len,
    unsigned char       **decoded_buf,
    unsigned int        *decoded_buf_len
) {
    huffman_node *root, *p;
    unsigned int data_count;
    unsigned int i = 0;
    unsigned char *buf;
    unsigned int bufcur = 0;

    /* Ensure the arguments are valid. */
    if(!decoded_buf || !decoded_buf_len)
        return 1;

    /* Read the Huffman code table. */
    root = build_huffman_tree(encoded_buf, encoded_buf_len, &i, &data_count);
    if(!root)
        return 1;

    buf = (unsigned char*)malloc(data_count);

    /* Decode the memory. */
    p = root;
    for(; i < encoded_buf_len && data_count > 0; ++i) 
    {
        unsigned char byte = encoded_buf[i];
        unsigned char mask = 1;
        while(data_count > 0 && mask)
        {
            p = byte & mask ? p->one : p->zero;
            mask <<= 1;

            if(p->isLeaf)
            {
                buf[bufcur++] = p->symbol;
                p = root;
                --data_count;
            }
        }
    }

    free_huffman_tree(root);
    *decoded_buf= buf;
    *decoded_buf_len = bufcur;
    return 0;
}