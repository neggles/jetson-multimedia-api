/*
 * Copyright (c) 2016-2022, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef V4L2_BACKEND_TEST_H
#define V4L2_BACKEND_TEST_H

#include "NvVideoDecoder.h"
#include "NvEglRenderer.h"
#include "NvJpegEncoder.h"
#include <queue>
#include <utility>
#include <map>
#include <fstream>
#include <pthread.h>
#include <semaphore.h>
#include "nvosd.h"

#ifdef ENABLE_TRT
#include "trt_inference.h"
#define    TRT_MODEL        GOOGLENET_SINGLE_CLASS
#endif

using namespace std;

#define MAX_BUFFERS  32
#define OSD_BUF_NUM 100

enum slice_type_e
{
    SLICE_TYPE_P = 0,
    SLICE_TYPE_B = 1,
    SLICE_TYPE_I = 2,
};

enum nal_type_e
{
    NAL_UNIT_EXTERNAL                 = 0,
    NAL_UNIT_CODED_SLICE              = 1,
    NAL_UNIT_CODED_SLICE_DATAPART_A   = 2,
    NAL_UNIT_CODED_SLICE_DATAPART_B   = 3,
    NAL_UNIT_CODED_SLICE_DATAPART_C   = 4,
    NAL_UNIT_CODED_SLICE_IDR          = 5,
    NAL_UNIT_SEI                      = 6,
    NAL_UNIT_SPS                      = 7,
    NAL_UNIT_PPS                      = 8,
    NAL_UNIT_ACCESS_UNIT_DELIMITER    = 9,
    NAL_UNIT_END_OF_SEQUENCE          = 10,
    NAL_UNIT_END_OF_STREAM            = 11,
    NAL_UNIT_FILLER_DATA              = 12,
    NAL_UNIT_SUBSET_SPS               = 15,
    NAL_UNIT_CODED_SLICE_PREFIX       = 14,
    NAL_UNIT_CODED_SLICE_SCALABLE     = 20,
    NAL_UNIT_CODED_SLICE_IDR_SCALABLE = 21
};

typedef struct
{
    uint32_t window_height;
    uint32_t window_width;
} display_resolution_t;

typedef struct
{
    uint32_t window_height;
    uint32_t window_width;
    uint32_t window_x;
    uint32_t window_y;
    uint32_t crop_height;
    uint32_t crop_width;
    uint32_t crop_x;
    uint32_t crop_y;
    float    alpha;
}window_t;

class TRT_Context;

#define WINDOW_NUM 4
#define CHANNEL_NUM 4

#define PARSER_DECODER_VIC_RENDER 0
#define PARSER   1
#define PARSER_DECODER 2
#define PARSER_DECODER_VIC 3

typedef struct
{
    uint64_t timestamp;
    struct timeval input_time;
    struct timeval output_time;
    slice_type_e slice_type;
    nal_type_e nal_type;
    uint32_t ref;
} frame_info_t;

typedef struct
{
    NvOSD_RectParams *g_rect;
    int  g_rect_num;
} frame_bbox;

typedef struct
{
    int                fd;
    int                channel;
    frame_bbox         *bbox;
} Shared_Buffer;


struct backend_context;

#ifdef ENABLE_TRT
typedef struct
{
    TRT_Context        tctx;
    pthread_mutex_t    trt_lock;
    pthread_cond_t     trt_cond;
    pthread_t          trt_handle;
    std::queue<Shared_Buffer> *trt_buf_queue;

    pthread_mutex_t    osd_lock;
    std::queue<frame_bbox*> *osd_queue;
    sem_t              result_ready_sem;
    uint32_t           *parray;
    uint32_t           rect_count;

    struct backend_context *ctx;
} trt_context;
#endif

typedef struct backend_context
{
    uint32_t channel;
    uint32_t dec_status; //0 stream off, 1 stream on
    NvVideoDecoder *dec;
    uint32_t decoder_pixfmt;
    int extra_cap_plane_buffer;
    int numCapBuffers;
    int dmabuff_fd[MAX_BUFFERS];

    NvEglRenderer *renderer;

    EGLImageKHR egl_image;

    char *in_file_path;
    std::ifstream *in_file;

    char *out_file_path;
    std::ofstream *out_file;

    bool fullscreen;
    bool do_stat;

    //0, not test, 1 parser, 2 parser+decoder, 3 parser+decode + VIC
    uint32_t cpu_occupation_option;
    uint32_t window_height;
    uint32_t window_width;
    uint32_t window_x;
    uint32_t window_y;

    float fps;

    bool disable_dpb;

    bool input_nalu;

    map< uint64_t, frame_info_t* > *frame_info_map;
    window_t window[WINDOW_NUM];

    pthread_mutex_t fps_lock;
    pthread_cond_t fps_cond;
    pthread_t dec_capture_loop;
    pthread_t dec_feed_handle;
    pthread_t render_feed_handle;
    std::queue<Shared_Buffer> *render_buf_queue;
    int render_fd;

    pthread_mutex_t render_lock; // for dec & render
    pthread_cond_t render_cond;

    //VIC need wait until dec get run
    bool got_error;
    bool got_eos;
    int stop_render;
    void *nvosd_context;
#ifdef ENABLE_TRT
    trt_context *trt_ctx;
    int         trt_fd;
#endif
} context_t;

typedef struct
{
    uint32_t channel_num;
    char *in_file_path[CHANNEL_NUM];
#ifdef ENABLE_TRT
    string deployfile;
    string modelfile;
#endif
} global_cfg;

int parse_csv_args(context_t * ctx,
#ifdef ENABLE_TRT
    TRT_Context *trt_ctx,
#endif
    int argc, char *argv[]);
void parse_global(global_cfg* cfg, int argc, char ***argv);
void print_help(void);
#endif
