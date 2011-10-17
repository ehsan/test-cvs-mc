/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vpx_config.h"
#include "vp8/common/loopfilter.h"

prototype_loopfilter(vp8_mbloop_filter_vertical_edge_mmx);
prototype_loopfilter(vp8_mbloop_filter_horizontal_edge_mmx);
prototype_loopfilter(vp8_loop_filter_vertical_edge_mmx);
prototype_loopfilter(vp8_loop_filter_horizontal_edge_mmx);

prototype_loopfilter(vp8_loop_filter_vertical_edge_sse2);
prototype_loopfilter(vp8_loop_filter_horizontal_edge_sse2);
prototype_loopfilter(vp8_mbloop_filter_vertical_edge_sse2);
prototype_loopfilter(vp8_mbloop_filter_horizontal_edge_sse2);

extern loop_filter_uvfunction vp8_loop_filter_horizontal_edge_uv_sse2;
extern loop_filter_uvfunction vp8_loop_filter_vertical_edge_uv_sse2;
extern loop_filter_uvfunction vp8_mbloop_filter_horizontal_edge_uv_sse2;
extern loop_filter_uvfunction vp8_mbloop_filter_vertical_edge_uv_sse2;

#if HAVE_MMX
/* Horizontal MB filtering */
void vp8_loop_filter_mbh_mmx(unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr,
                             int y_stride, int uv_stride, loop_filter_info *lfi)
{
    vp8_mbloop_filter_horizontal_edge_mmx(y_ptr, y_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_mbloop_filter_horizontal_edge_mmx(u_ptr, uv_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 1);

    if (v_ptr)
        vp8_mbloop_filter_horizontal_edge_mmx(v_ptr, uv_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 1);
}


/* Vertical MB Filtering */
void vp8_loop_filter_mbv_mmx(unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr,
                             int y_stride, int uv_stride, loop_filter_info *lfi)
{
    vp8_mbloop_filter_vertical_edge_mmx(y_ptr, y_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_mbloop_filter_vertical_edge_mmx(u_ptr, uv_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 1);

    if (v_ptr)
        vp8_mbloop_filter_vertical_edge_mmx(v_ptr, uv_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 1);
}


/* Horizontal B Filtering */
void vp8_loop_filter_bh_mmx(unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr,
                            int y_stride, int uv_stride, loop_filter_info *lfi)
{
    vp8_loop_filter_horizontal_edge_mmx(y_ptr + 4 * y_stride, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_horizontal_edge_mmx(y_ptr + 8 * y_stride, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_horizontal_edge_mmx(y_ptr + 12 * y_stride, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_loop_filter_horizontal_edge_mmx(u_ptr + 4 * uv_stride, uv_stride, lfi->blim, lfi->lim, lfi->hev_thr, 1);

    if (v_ptr)
        vp8_loop_filter_horizontal_edge_mmx(v_ptr + 4 * uv_stride, uv_stride, lfi->blim, lfi->lim, lfi->hev_thr, 1);
}


void vp8_loop_filter_bhs_mmx(unsigned char *y_ptr, int y_stride, const unsigned char *blimit)
{
    vp8_loop_filter_simple_horizontal_edge_mmx(y_ptr + 4 * y_stride, y_stride, blimit);
    vp8_loop_filter_simple_horizontal_edge_mmx(y_ptr + 8 * y_stride, y_stride, blimit);
    vp8_loop_filter_simple_horizontal_edge_mmx(y_ptr + 12 * y_stride, y_stride, blimit);
}


/* Vertical B Filtering */
void vp8_loop_filter_bv_mmx(unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr,
                            int y_stride, int uv_stride, loop_filter_info *lfi)
{
    vp8_loop_filter_vertical_edge_mmx(y_ptr + 4, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_vertical_edge_mmx(y_ptr + 8, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_vertical_edge_mmx(y_ptr + 12, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_loop_filter_vertical_edge_mmx(u_ptr + 4, uv_stride, lfi->blim, lfi->lim, lfi->hev_thr, 1);

    if (v_ptr)
        vp8_loop_filter_vertical_edge_mmx(v_ptr + 4, uv_stride, lfi->blim, lfi->lim, lfi->hev_thr, 1);
}


void vp8_loop_filter_bvs_mmx(unsigned char *y_ptr, int y_stride, const unsigned char *blimit)
{
    vp8_loop_filter_simple_vertical_edge_mmx(y_ptr + 4, y_stride, blimit);
    vp8_loop_filter_simple_vertical_edge_mmx(y_ptr + 8, y_stride, blimit);
    vp8_loop_filter_simple_vertical_edge_mmx(y_ptr + 12, y_stride, blimit);
}
#endif


/* Horizontal MB filtering */
#if HAVE_SSE2
void vp8_loop_filter_mbh_sse2(unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr,
                              int y_stride, int uv_stride, loop_filter_info *lfi)
{
    vp8_mbloop_filter_horizontal_edge_sse2(y_ptr, y_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_mbloop_filter_horizontal_edge_uv_sse2(u_ptr, uv_stride, lfi->mblim, lfi->lim, lfi->hev_thr, v_ptr);
}


/* Vertical MB Filtering */
void vp8_loop_filter_mbv_sse2(unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr,
                              int y_stride, int uv_stride, loop_filter_info *lfi)
{
    vp8_mbloop_filter_vertical_edge_sse2(y_ptr, y_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_mbloop_filter_vertical_edge_uv_sse2(u_ptr, uv_stride, lfi->mblim, lfi->lim, lfi->hev_thr, v_ptr);
}


/* Horizontal B Filtering */
void vp8_loop_filter_bh_sse2(unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr,
                             int y_stride, int uv_stride, loop_filter_info *lfi)
{
    vp8_loop_filter_horizontal_edge_sse2(y_ptr + 4 * y_stride, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_horizontal_edge_sse2(y_ptr + 8 * y_stride, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_horizontal_edge_sse2(y_ptr + 12 * y_stride, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_loop_filter_horizontal_edge_uv_sse2(u_ptr + 4 * uv_stride, uv_stride, lfi->blim, lfi->lim, lfi->hev_thr, v_ptr + 4 * uv_stride);
}


void vp8_loop_filter_bhs_sse2(unsigned char *y_ptr, int y_stride, const unsigned char *blimit)
{
    vp8_loop_filter_simple_horizontal_edge_sse2(y_ptr + 4 * y_stride, y_stride, blimit);
    vp8_loop_filter_simple_horizontal_edge_sse2(y_ptr + 8 * y_stride, y_stride, blimit);
    vp8_loop_filter_simple_horizontal_edge_sse2(y_ptr + 12 * y_stride, y_stride, blimit);
}


/* Vertical B Filtering */
void vp8_loop_filter_bv_sse2(unsigned char *y_ptr, unsigned char *u_ptr, unsigned char *v_ptr,
                             int y_stride, int uv_stride, loop_filter_info *lfi)
{
    vp8_loop_filter_vertical_edge_sse2(y_ptr + 4, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_vertical_edge_sse2(y_ptr + 8, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_vertical_edge_sse2(y_ptr + 12, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_loop_filter_vertical_edge_uv_sse2(u_ptr + 4, uv_stride, lfi->blim, lfi->lim, lfi->hev_thr, v_ptr + 4);
}


void vp8_loop_filter_bvs_sse2(unsigned char *y_ptr, int y_stride, const unsigned char *blimit)
{
    vp8_loop_filter_simple_vertical_edge_sse2(y_ptr + 4, y_stride, blimit);
    vp8_loop_filter_simple_vertical_edge_sse2(y_ptr + 8, y_stride, blimit);
    vp8_loop_filter_simple_vertical_edge_sse2(y_ptr + 12, y_stride, blimit);
}

#endif
