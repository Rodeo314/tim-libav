/*
 * Copyright (c) 2014 Tim Walker <tdskywalker@gmail.com>
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

#ifndef AVFORMAT_HEVC_H
#define AVFORMAT_HEVC_H

#include <stdint.h>
#include "avio.h"

#define HEVC_DEBUG_LOG(str, ...) av_log(NULL, AV_LOG_FATAL, "%s, %s: %d - " str, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

static av_unused int binar_ize(uint8_t n)
{
    int i, ret = 0;
    for (i = 0; i < 8; i++)
        if (n & (1 << i))
            ret += (int)pow(10, i);
    return ret;
}

typedef struct HEVCDecoderConfigurationRecord {
    uint8_t  configurationVersion;
    uint8_t  general_profile_space;
    uint8_t  general_tier_flag;
    uint8_t  general_profile_idc;
    uint32_t general_profile_compatibility_flags;
    uint64_t general_constraint_indicator_flags;
    uint8_t  general_level_idc;
    uint16_t min_spatial_segmentation_idc;
    uint8_t  parallelismType;
    uint8_t  chromaFormat;
    uint8_t  bitDepthLumaMinus8;
    uint8_t  bitDepthChromaMinus8;
    uint16_t avgFrameRate;
    uint8_t  constantFrameRate;
    uint8_t  numTemporalLayers;
    uint8_t  temporalIdNested;
    uint8_t  lengthSizeMinusOne;
} HEVCDecoderConfigurationRecord;

typedef struct HVCCProfileTierLevel {
    uint8_t  profile_space;
    uint8_t  tier_flag;
    uint8_t  profile_idc;
    uint32_t profile_compatibility_flags;
    uint64_t constraint_indicator_flags;
    uint8_t  level_idc;
} HVCCProfileTierLevel;

int ff_isom_write_hvcc(AVIOContext *pb, const uint8_t *data, int len);

#endif /* AVFORMAT_HEVC_H */
