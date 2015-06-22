/*
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_NVENCODEAPI_H
#define AVCODEC_NVENCODEAPI_H

#include <cuda.h>

#define AV_NV_FUNC_TYPE(name)   ff_nvenc_##name##_func
#define AV_NV_FUNC_DECL(name)   AV_NV_FUNC_TYPE(name) name

#define NV_ENC_CAPS_SUPPORT_YUV444_ENCODE           (1 << 0)
#define NV_ENC_CAPS_WIDTH_MAX                       (1 << 1)
#define NV_ENC_CAPS_HEIGHT_MAX                      (1 << 2)
#define NV_ENC_CAPS_NUM_MAX_BFRAMES                 (1 << 3)
#define NV_ENC_PIC_FLAG_EOS                         (1 << 4)

#define NVENCAPI                                    CUDAAPI
#define NVENCAPI_VERSION                            5
#define NV_ENCODE_API_FUNCTION_LIST_VER             5
#define NV_ENC_BUFFER_FORMAT_NV12_PL                1
#define NV_ENC_BUFFER_FORMAT_YUV444_PL              2
#define NV_ENC_BUFFER_FORMAT_YV12_PL                3
#define NV_ENC_CAPS_PARAM_VER                       5
#define NV_ENC_CONFIG_VER                           5
#define NV_ENC_CREATE_BITSTREAM_BUFFER_VER          5
#define NV_ENC_CREATE_INPUT_BUFFER_VER              5
#define NV_ENC_DEVICE_TYPE_CUDA                     1
#define NV_ENC_ERR_NEED_MORE_INPUT                 -2
#define NV_ENC_INITIALIZE_PARAMS_VER                5
#define NV_ENC_LEVEL_AUTOSELECT                     0
#define NV_ENC_LEVEL_H264_1                        10
#define NV_ENC_LEVEL_H264_1b                        9
#define NV_ENC_LEVEL_H264_11                       11
#define NV_ENC_LEVEL_H264_12                       12
#define NV_ENC_LEVEL_H264_13                       13
#define NV_ENC_LEVEL_H264_2                        20
#define NV_ENC_LEVEL_H264_21                       21
#define NV_ENC_LEVEL_H264_22                       22
#define NV_ENC_LEVEL_H264_3                        30
#define NV_ENC_LEVEL_H264_31                       31
#define NV_ENC_LEVEL_H264_32                       32
#define NV_ENC_LEVEL_H264_4                        40
#define NV_ENC_LEVEL_H264_41                       41
#define NV_ENC_LEVEL_H264_42                       42
#define NV_ENC_LEVEL_H264_5                        50
#define NV_ENC_LEVEL_H264_51                       51
#define NV_ENC_LEVEL_HEVC_1                        30
#define NV_ENC_LEVEL_HEVC_2                        60
#define NV_ENC_LEVEL_HEVC_21                       63
#define NV_ENC_LEVEL_HEVC_3                        90
#define NV_ENC_LEVEL_HEVC_31                       93
#define NV_ENC_LEVEL_HEVC_4                       120
#define NV_ENC_LEVEL_HEVC_41                      123
#define NV_ENC_LEVEL_HEVC_5                       150
#define NV_ENC_LEVEL_HEVC_51                      153
#define NV_ENC_LEVEL_HEVC_52                      156
#define NV_ENC_LEVEL_HEVC_6                       180
#define NV_ENC_LEVEL_HEVC_61                      183
#define NV_ENC_LEVEL_HEVC_62                      186
#define NV_ENC_LOCK_BITSTREAM_VER                   5
#define NV_ENC_LOCK_INPUT_BUFFER_VER                5
#define NV_ENC_MEMORY_HEAP_SYSMEM_UNCACHED          1
#define NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER    5
#define NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME        1
#define NV_ENC_PARAMS_FRAME_FIELD_MODE_FIELD        2
#define NV_ENC_PIC_TYPE_B                           1
#define NV_ENC_PIC_TYPE_BI                          2
#define NV_ENC_PIC_TYPE_I                           3
#define NV_ENC_PIC_TYPE_IDR                         4
#define NV_ENC_PIC_TYPE_INTRA_REFRESH               5
#define NV_ENC_PIC_TYPE_P                           6
#define NV_ENC_PIC_STRUCT_FIELD_BOTTOM_TOP          1
#define NV_ENC_PIC_STRUCT_FIELD_TOP_BOTTOM          2
#define NV_ENC_PIC_STRUCT_FRAME                     3
#define NV_ENC_SEQUENCE_PARAM_PAYLOAD_VER           5
#define NV_ENC_PARAMS_RC_CBR                        1
#define NV_ENC_PARAMS_RC_CONSTQP                    2
#define NV_ENC_PARAMS_RC_VBR                        3
#define NV_ENC_PARAMS_RC_VBR_MINQP                  4
#define NV_ENC_PARAMS_RC_2_PASS_QUALITY             5
#define NV_ENC_PARAMS_RC_2_PASS_FRAMESIZE_CAP       6
#define NV_ENC_PARAMS_RC_2_PASS_VBR                 7
#define NV_ENC_PIC_PARAMS_VER                       5
#define NV_ENC_PRESET_CONFIG_VER                    5
#define NV_ENC_SUCCESS                              CUDA_SUCCESS
#define NV_ENC_TIER_HEVC_MAIN                       1
#define NV_ENC_TIER_HEVC_HIGH                       2

typedef int*        GUID;
typedef CUresult    NVENCSTATUS;
typedef int         NV_ENC_BUFFER_FORMAT;
typedef int         NV_ENC_CAPS;
typedef void*       NV_ENC_INPUT_PTR;
typedef void*       NV_ENC_OUTPUT_PTR;

typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncCreateBitstreamBuffer))     (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncCreateInputBuffer))         (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncDestroyBitstreamBuffer))    (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncDestroyEncoder))            (void* v1);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncDestroyInputBuffer))        (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncEncodePicture))             (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncGetEncodeCaps))             (void* v1, GUID  v2, void* v3, void* v4);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncGetEncodeGUIDCount))        (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncGetEncodeGUIDs))            (void* v1, GUID* v2, int   v3, void* v4);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncGetEncodePresetConfig))     (void* v1, GUID  v2, GUID  v3, void* v4);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncGetSequenceParams))         (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncInitializeEncoder))         (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncLockBitstream))             (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncLockInputBuffer))           (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncOpenEncodeSessionEx))       (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncUnlockBitstream))           (void* v1, void* v2);
typedef CUresult(NVENCAPI *AV_NV_FUNC_TYPE(nvEncUnlockInputBuffer))         (void* v1, void* v2);

static int NV_ENC_CODEC_H264_GUID[]                     = { 1, 1, 0, };
static int NV_ENC_CODEC_HEVC_GUID[]                     = { 1, 2, 0, };
static int NV_ENC_PRESET_DEFAULT_GUID[]                 = { 2, 1, 0, };
static int NV_ENC_PRESET_HP_GUID[]                      = { 2, 2, 0, };
static int NV_ENC_PRESET_HQ_GUID[]                      = { 2, 3, 0, };
static int NV_ENC_PRESET_BD_GUID[]                      = { 2, 4, 0, };
static int NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID[]     = { 3, 1, 0, };
static int NV_ENC_PRESET_LOW_LATENCY_HP_GUID[]          = { 3, 2, 0, };
static int NV_ENC_PRESET_LOW_LATENCY_HQ_GUID[]          = { 3, 3, 0, };
static int NV_ENC_PRESET_LOSSLESS_DEFAULT_GUID[]        = { 4, 1, 0, };
static int NV_ENC_PRESET_LOSSLESS_HP_GUID[]             = { 4, 2, 0, };
static int NV_ENC_H264_PROFILE_BASELINE_GUID[]          = { 5, 1, 0, };
static int NV_ENC_H264_PROFILE_MAIN_GUID[]              = { 5, 2, 0, };
static int NV_ENC_H264_PROFILE_HIGH_GUID[]              = { 5, 3, 0, };
static int NV_ENC_H264_PROFILE_HIGH_444_GUID[]          = { 5, 4, 0, };
static int NV_ENC_H264_PROFILE_CONSTRAINED_HIGH_GUID[]  = { 5, 5, 0, };
static int NV_ENC_HEVC_PROFILE_MAIN_GUID[]              = { 6, 1, 0, };

typedef struct
{
    int version;

    AV_NV_FUNC_DECL(nvEncCreateBitstreamBuffer);
    AV_NV_FUNC_DECL(nvEncCreateInputBuffer);
    AV_NV_FUNC_DECL(nvEncDestroyBitstreamBuffer);
    AV_NV_FUNC_DECL(nvEncDestroyEncoder);
    AV_NV_FUNC_DECL(nvEncDestroyInputBuffer);
    AV_NV_FUNC_DECL(nvEncEncodePicture);
    AV_NV_FUNC_DECL(nvEncGetEncodeCaps);
    AV_NV_FUNC_DECL(nvEncGetEncodeGUIDCount);
    AV_NV_FUNC_DECL(nvEncGetEncodeGUIDs);
    AV_NV_FUNC_DECL(nvEncGetEncodePresetConfig);
    AV_NV_FUNC_DECL(nvEncGetSequenceParams);
    AV_NV_FUNC_DECL(nvEncInitializeEncoder);
    AV_NV_FUNC_DECL(nvEncLockBitstream);
    AV_NV_FUNC_DECL(nvEncLockInputBuffer);
    AV_NV_FUNC_DECL(nvEncOpenEncodeSessionEx);
    AV_NV_FUNC_DECL(nvEncUnlockBitstream);
    AV_NV_FUNC_DECL(nvEncUnlockInputBuffer);
}
NV_ENCODE_API_FUNCTION_LIST;

typedef struct
{
    NV_ENC_OUTPUT_PTR bitstreamBuffer;
    int               memoryHeap;
    int               size;
    int               version;
}
NV_ENC_CREATE_BITSTREAM_BUFFER;

typedef struct
{
    void*            bufferDataPtr;
    int              bufferFmt;
    int              height;
    NV_ENC_INPUT_PTR inputBuffer;
    int              memoryHeap;
    int              pitch;
    int              version;
    int              width;
}
NV_ENC_CREATE_INPUT_BUFFER;

typedef struct
{
    void*            bufferDataPtr;
    NV_ENC_INPUT_PTR inputBuffer;
    int              pitch;
    int              version;
}
NV_ENC_LOCK_INPUT_BUFFER;

typedef struct
{
    void*             bitstreamBufferPtr;
    int               bitstreamSizeInBytes;
    NV_ENC_OUTPUT_PTR outputBitstream;
    int               outputDuration;
    int               outputTimeStamp;
    int               pictureType;
    int               version;
}
NV_ENC_LOCK_BITSTREAM;

typedef struct
{
    int         version;
    NV_ENC_CAPS capsToQuery;
}
NV_ENC_CAPS_PARAM;

typedef struct
{
    int   apiVersion;
    void* device;
    int   deviceType;
    int   version;
}
NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS;

typedef struct
{
    int rateControlMode;
    int enableMinQP;
    int enableMaxQP;

    union
    {
        struct
        {
            int averageBitRate;
            int maxBitRate;
            int vbvBufferSize;
        };

        struct
        {
            struct
            {
                int qpIntra;
                int qpInterP;
                int qpInterB;
            }
            constQP;

            struct
            {
                int qpIntra;
                int qpInterP;
                int qpInterB;
            }
            minQP;

            struct
            {
                int qpIntra;
                int qpInterP;
                int qpInterB;
            }
            maxQP;
        };
    };
}
NV_ENC_RC_PARAMS;

typedef struct
{
    int colourDescriptionPresentFlag;
    int colourMatrix;
    int colourPrimaries;
    int transferCharacteristics;
    int videoFullRangeFlag;
    int videoSignalTypePresentFlag;
}
NV_ENC_CONFIG_H264_VUI_PARAMETERS;

typedef struct
{
    int                               chromaFormatIDC;
    int                               disableSPSPPS;
    NV_ENC_CONFIG_H264_VUI_PARAMETERS h264VUIParameters;
    int                               idrPeriod;
    int                               level;
    int                               maxNumRefFrames;
    int                               repeatSPSPPS;
    int                               sliceMode;
    void*                             sliceModeData;
}
NV_ENC_CONFIG_H264;

typedef struct
{
    int   chromaFormatIDC;
    int   disableSPSPPS;
    int   idrPeriod;
    int   level;
    int   maxNumRefFramesInDPB;
    int   repeatSPSPPS;
    int   sliceMode;
    void* sliceModeData;
    int   tier;
}
NV_ENC_CONFIG_HEVC;

typedef struct
{
    int              frameIntervalP;
    int              frameFieldMode;
    int              gopLength;
    int              level;
    GUID             profileGUID;
    NV_ENC_RC_PARAMS rcParams;
    int              version;

    struct
    {
        union
        {
            NV_ENC_CONFIG_H264 h264Config;
            NV_ENC_CONFIG_HEVC hevcConfig;
        };
    }
    encodeCodecConfig;
}
NV_ENC_CONFIG;

typedef struct
{
    int           version;
    NV_ENC_CONFIG presetCfg;
}
NV_ENC_PRESET_CONFIG;

typedef struct
{
    int               bufferFmt;
    int               encodePicFlags;
    NV_ENC_INPUT_PTR  inputBuffer;
    int               inputHeight;
    int               inputWidth;
    int               inputTimeStamp;
    NV_ENC_OUTPUT_PTR outputBitstream;
    int               pictureStruct;
    int               version;

    struct
    {
        union
        {
            NV_ENC_CONFIG_H264 h264PicParams;
            NV_ENC_CONFIG_HEVC hevcPicParams;
        };
    }
    codecPicParams;
}
NV_ENC_PIC_PARAMS;

typedef struct
{
    int             darHeight;
    int             darWidth;
    NV_ENC_CONFIG*  encodeConfig;
    int             enableEncodeAsync;
    int             enablePTD;
    GUID            encodeGUID;
    int             encodeHeight;
    int             encodeWidth;
    int             frameRateDen;
    int             frameRateNum;
    GUID            presetGUID;
    int             version;
}
NV_ENC_INITIALIZE_PARAMS;

typedef struct
{
    int   inBufferSize;
    int*  outSPSPPSPayloadSize;
    void* spsppsBuffer;
    int   version;
}
NV_ENC_SEQUENCE_PARAM_PAYLOAD;

#endif /* AVCODEC_NVENCODEAPI_H */
