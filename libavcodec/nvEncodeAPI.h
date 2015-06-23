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

#include "nvencwrapper_api.h"

#define NV_ENC_CAPS_SUPPORT_YUV444_ENCODE           NVW_ENC_CAPS_SUPPORT_YUV444_ENCODE
#define NV_ENC_CAPS_WIDTH_MAX                       NVW_ENC_CAPS_WIDTH_MAX
#define NV_ENC_CAPS_HEIGHT_MAX                      NVW_ENC_CAPS_HEIGHT_MAX
#define NV_ENC_CAPS_NUM_MAX_BFRAMES                 NVW_ENC_CAPS_NUM_MAX_BFRAMES
#define NV_ENC_PIC_FLAG_EOS                         NVW_ENC_PIC_FLAG_EOS
#define NVENCAPI                                    NVWENCAPI
#define NVENCAPI_VERSION                            NVWENCAPI_VERSION
#define NV_ENCODE_API_FUNCTION_LIST_VER             NVW_ENCODE_API_FUNCTION_LIST_VER
#define NV_ENC_BUFFER_FORMAT_NV12_PL                NVW_ENC_BUFFER_FORMAT_NV12_PL
#define NV_ENC_BUFFER_FORMAT_YUV444_PL              NVW_ENC_BUFFER_FORMAT_YUV444_PL
#define NV_ENC_BUFFER_FORMAT_YV12_PL                NVW_ENC_BUFFER_FORMAT_YV12_PL
#define NV_ENC_CAPS_PARAM_VER                       NVW_ENC_CAPS_PARAM_VER
#define NV_ENC_CONFIG_VER                           NVW_ENC_CONFIG_VER
#define NV_ENC_CREATE_BITSTREAM_BUFFER_VER          NVW_ENC_CREATE_BITSTREAM_BUFFER_VER
#define NV_ENC_CREATE_INPUT_BUFFER_VER              NVW_ENC_CREATE_INPUT_BUFFER_VER
#define NV_ENC_DEVICE_TYPE_CUDA                     NVW_ENC_DEVICE_TYPE_CUDA
#define NV_ENC_ERR_NEED_MORE_INPUT                  NVW_ENC_ERR_NEED_MORE_INPUT
#define NV_ENC_INITIALIZE_PARAMS_VER                NVW_ENC_INITIALIZE_PARAMS_VER
#define NV_ENC_LEVEL_AUTOSELECT                     NVW_ENC_LEVEL_AUTOSELECT
#define NV_ENC_LEVEL_H264_1                         NVW_ENC_LEVEL_H264_1
#define NV_ENC_LEVEL_H264_1b                        NVW_ENC_LEVEL_H264_1b
#define NV_ENC_LEVEL_H264_11                        NVW_ENC_LEVEL_H264_11
#define NV_ENC_LEVEL_H264_12                        NVW_ENC_LEVEL_H264_12
#define NV_ENC_LEVEL_H264_13                        NVW_ENC_LEVEL_H264_13
#define NV_ENC_LEVEL_H264_2                         NVW_ENC_LEVEL_H264_2
#define NV_ENC_LEVEL_H264_21                        NVW_ENC_LEVEL_H264_21
#define NV_ENC_LEVEL_H264_22                        NVW_ENC_LEVEL_H264_22
#define NV_ENC_LEVEL_H264_3                         NVW_ENC_LEVEL_H264_3
#define NV_ENC_LEVEL_H264_31                        NVW_ENC_LEVEL_H264_31
#define NV_ENC_LEVEL_H264_32                        NVW_ENC_LEVEL_H264_32
#define NV_ENC_LEVEL_H264_4                         NVW_ENC_LEVEL_H264_4
#define NV_ENC_LEVEL_H264_41                        NVW_ENC_LEVEL_H264_41
#define NV_ENC_LEVEL_H264_42                        NVW_ENC_LEVEL_H264_42
#define NV_ENC_LEVEL_H264_5                         NVW_ENC_LEVEL_H264_5
#define NV_ENC_LEVEL_H264_51                        NVW_ENC_LEVEL_H264_51
#define NV_ENC_LEVEL_HEVC_1                         NVW_ENC_LEVEL_HEVC_1
#define NV_ENC_LEVEL_HEVC_2                         NVW_ENC_LEVEL_HEVC_2
#define NV_ENC_LEVEL_HEVC_21                        NVW_ENC_LEVEL_HEVC_21
#define NV_ENC_LEVEL_HEVC_3                         NVW_ENC_LEVEL_HEVC_3
#define NV_ENC_LEVEL_HEVC_31                        NVW_ENC_LEVEL_HEVC_31
#define NV_ENC_LEVEL_HEVC_4                         NVW_ENC_LEVEL_HEVC_4
#define NV_ENC_LEVEL_HEVC_41                        NVW_ENC_LEVEL_HEVC_41
#define NV_ENC_LEVEL_HEVC_5                         NVW_ENC_LEVEL_HEVC_5
#define NV_ENC_LEVEL_HEVC_51                        NVW_ENC_LEVEL_HEVC_51
#define NV_ENC_LEVEL_HEVC_52                        NVW_ENC_LEVEL_HEVC_52
#define NV_ENC_LEVEL_HEVC_6                         NVW_ENC_LEVEL_HEVC_6
#define NV_ENC_LEVEL_HEVC_61                        NVW_ENC_LEVEL_HEVC_61
#define NV_ENC_LEVEL_HEVC_62                        NVW_ENC_LEVEL_HEVC_62
#define NV_ENC_LOCK_BITSTREAM_VER                   NVW_ENC_LOCK_BITSTREAM_VER
#define NV_ENC_LOCK_INPUT_BUFFER_VER                NVW_ENC_LOCK_INPUT_BUFFER_VER
#define NV_ENC_MEMORY_HEAP_SYSMEM_UNCACHED          NVW_ENC_MEMORY_HEAP_SYSMEM_UNCACHED
#define NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER    NVW_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER
#define NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME        NVW_ENC_PARAMS_FRAME_FIELD_MODE_FRAME
#define NV_ENC_PARAMS_FRAME_FIELD_MODE_FIELD        NVW_ENC_PARAMS_FRAME_FIELD_MODE_FIELD
#define NV_ENC_PIC_TYPE_B                           NVW_ENC_PIC_TYPE_B
#define NV_ENC_PIC_TYPE_BI                          NVW_ENC_PIC_TYPE_BI
#define NV_ENC_PIC_TYPE_I                           NVW_ENC_PIC_TYPE_I
#define NV_ENC_PIC_TYPE_IDR                         NVW_ENC_PIC_TYPE_IDR
#define NV_ENC_PIC_TYPE_INTRA_REFRESH               NVW_ENC_PIC_TYPE_INTRA_REFRESH
#define NV_ENC_PIC_TYPE_P                           NVW_ENC_PIC_TYPE_P
#define NV_ENC_PIC_STRUCT_FIELD_BOTTOM_TOP          NVW_ENC_PIC_STRUCT_FIELD_BOTTOM_TOP
#define NV_ENC_PIC_STRUCT_FIELD_TOP_BOTTOM          NVW_ENC_PIC_STRUCT_FIELD_TOP_BOTTOM
#define NV_ENC_PIC_STRUCT_FRAME                     NVW_ENC_PIC_STRUCT_FRAME
#define NV_ENC_SEQUENCE_PARAM_PAYLOAD_VER           NVW_ENC_SEQUENCE_PARAM_PAYLOAD_VER
#define NV_ENC_PARAMS_RC_CBR                        NVW_ENC_PARAMS_RC_CBR
#define NV_ENC_PARAMS_RC_CONSTQP                    NVW_ENC_PARAMS_RC_CONSTQP
#define NV_ENC_PARAMS_RC_VBR                        NVW_ENC_PARAMS_RC_VBR
#define NV_ENC_PARAMS_RC_VBR_MINQP                  NVW_ENC_PARAMS_RC_VBR_MINQP
#define NV_ENC_PARAMS_RC_2_PASS_QUALITY             NVW_ENC_PARAMS_RC_2_PASS_QUALITY
#define NV_ENC_PARAMS_RC_2_PASS_FRAMESIZE_CAP       NVW_ENC_PARAMS_RC_2_PASS_FRAMESIZE_CAP
#define NV_ENC_PARAMS_RC_2_PASS_VBR                 NVW_ENC_PARAMS_RC_2_PASS_VBR
#define NV_ENC_PIC_PARAMS_VER                       NVW_ENC_PIC_PARAMS_VER
#define NV_ENC_PRESET_CONFIG_VER                    NVW_ENC_PRESET_CONFIG_VER
#define NV_ENC_SUCCESS                              NVW_ENC_SUCCESS
#define NV_ENC_TIER_HEVC_MAIN                       NVW_ENC_TIER_HEVC_MAIN
#define NV_ENC_TIER_HEVC_HIGH                       NVW_ENC_TIER_HEVC_HIGH
#define GUID                                        NVWGUID
#define NVENCSTATUS                                 NVWENCSTATUS
#define NV_ENC_BUFFER_FORMAT                        NVW_ENC_BUFFER_FORMAT
#define NV_ENC_CAPS                                 NVW_ENC_CAPS
#define NV_ENC_INPUT_PTR                            NVW_ENC_INPUT_PTR
#define NV_ENC_OUTPUT_PTR                           NVW_ENC_OUTPUT_PTR
#define NV_ENCODE_API_FUNCTION_LIST                 NVW_ENCODE_API_FUNCTION_LIST
#define NV_ENC_CREATE_BITSTREAM_BUFFER              NVW_ENC_CREATE_BITSTREAM_BUFFER
#define NV_ENC_CREATE_INPUT_BUFFER                  NVW_ENC_CREATE_INPUT_BUFFER
#define NV_ENC_LOCK_INPUT_BUFFER                    NVW_ENC_LOCK_INPUT_BUFFER
#define NV_ENC_LOCK_BITSTREAM                       NVW_ENC_LOCK_BITSTREAM
#define NV_ENC_CAPS_PARAM                           NVW_ENC_CAPS_PARAM
#define NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS        NVW_ENC_OPEN_ENCODE_SESSION_EX_PARAMS
#define NV_ENC_RC_PARAMS                            NVW_ENC_RC_PARAMS
#define NV_ENC_CONFIG_H264_VUI_PARAMETERS           NVW_ENC_CONFIG_H264_VUI_PARAMETERS
#define NV_ENC_CONFIG_H264                          NVW_ENC_CONFIG_H264
#define NV_ENC_CONFIG_HEVC                          NVW_ENC_CONFIG_HEVC
#define NV_ENC_CONFIG                               NVW_ENC_CONFIG
#define NV_ENC_PRESET_CONFIG                        NVW_ENC_PRESET_CONFIG
#define NV_ENC_PIC_PARAMS                           NVW_ENC_PIC_PARAMS
#define NV_ENC_INITIALIZE_PARAMS                    NVW_ENC_INITIALIZE_PARAMS
#define NV_ENC_SEQUENCE_PARAM_PAYLOAD               NVW_ENC_SEQUENCE_PARAM_PAYLOAD
#define NV_ENC_CODEC_H264_GUID                      NVW_ENC_CODEC_H264_GUID
#define NV_ENC_CODEC_HEVC_GUID                      NVW_ENC_CODEC_HEVC_GUID
#define NV_ENC_PRESET_DEFAULT_GUID                  NVW_ENC_PRESET_DEFAULT_GUID
#define NV_ENC_PRESET_HP_GUID                       NVW_ENC_PRESET_HP_GUID
#define NV_ENC_PRESET_HQ_GUID                       NVW_ENC_PRESET_HQ_GUID
#define NV_ENC_PRESET_BD_GUID                       NVW_ENC_PRESET_BD_GUID
#define NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID      NVW_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID
#define NV_ENC_PRESET_LOW_LATENCY_HP_GUID           NVW_ENC_PRESET_LOW_LATENCY_HP_GUID
#define NV_ENC_PRESET_LOW_LATENCY_HQ_GUID           NVW_ENC_PRESET_LOW_LATENCY_HQ_GUID
#define NV_ENC_PRESET_LOSSLESS_DEFAULT_GUID         NVW_ENC_PRESET_LOSSLESS_DEFAULT_GUID
#define NV_ENC_PRESET_LOSSLESS_HP_GUID              NVW_ENC_PRESET_LOSSLESS_HP_GUID
#define NV_ENC_H264_PROFILE_BASELINE_GUID           NVW_ENC_H264_PROFILE_BASELINE_GUID
#define NV_ENC_H264_PROFILE_MAIN_GUID               NVW_ENC_H264_PROFILE_MAIN_GUID
#define NV_ENC_H264_PROFILE_HIGH_GUID               NVW_ENC_H264_PROFILE_HIGH_GUID
#define NV_ENC_H264_PROFILE_HIGH_444_GUID           NVW_ENC_H264_PROFILE_HIGH_444_GUID
#define NV_ENC_H264_PROFILE_CONSTRAINED_HIGH_GUID   NVW_ENC_H264_PROFILE_CONSTRAINED_HIGH_GUID
#define NV_ENC_HEVC_PROFILE_MAIN_GUID               NVW_ENC_HEVC_PROFILE_MAIN_GUID

#endif /* AVCODEC_NVENCODEAPI_H */
