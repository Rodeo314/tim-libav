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

#include "libavcodec/hevc.h"
#include "libavutil/intreadwrite.h"
#include "avc.h"
#include "avformat.h"
#include "avio.h"
#include "hevc.h"

#define HEVC_DEBUG_LOG(str, ...) av_log(NULL, AV_LOG_FATAL, "%s, %s: %d - " str, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SPS_MIN_SIZE 13

int ff_isom_write_hvcc(AVIOContext *pb, const uint8_t *data, int len)
{
    HEVC_DEBUG_LOG("data: %p, len: %d\n", data, len);
    if (len > SPS_MIN_SIZE + 2) {
        /* check for H.265 start code */
        if (AV_RB32(data) == 0x00000001 || AV_RB24(data) == 0x000001) {
            uint32_t vps_size = 0, sps_size = 0, pps_size = 0;
            uint8_t *buf = NULL, *end, *start, *vps = NULL, *sps = NULL, *pps = NULL;

            int ret = ff_avc_parse_nal_units_buf(data, &buf, &len);
            if (ret < 0)
                return ret;
            start = buf;
            end   = buf + len;

            /* look for vps, sps and pps */
            while (end - buf > 4) {
                uint32_t size;
                uint8_t nal_type;
                size = FFMIN(AV_RB32(buf), end - buf - 4);
                buf += 4;
                nal_type = (buf[0] >> 1) & 0x3f;

                switch (nal_type) {
                case NAL_VPS:
                    vps      = buf;
                    vps_size = size;
                    HEVC_DEBUG_LOG("VPS with type: %d and size: %d\n", nal_type, size);
                    break;
                case NAL_SPS:
                    sps      = buf;
                    sps_size = size;
                    HEVC_DEBUG_LOG("SPS with type: %d and size: %d\n", nal_type, size);
                    break;
                case NAL_PPS:
                    pps      = buf;
                    pps_size = size;
                    HEVC_DEBUG_LOG("PPS with type: %d and size: %d\n", nal_type, size);
                    break;
                default:
                    break;
                }

                buf += size;
            }

            if (!vps || vps_size > UINT16_MAX ||
                !sps || sps_size > UINT16_MAX || sps_size < SPS_MIN_SIZE ||
                !pps || pps_size > UINT16_MAX) {
                av_log(NULL, AV_LOG_ERROR, "vps: %p size: %"PRIu32"\n", vps, vps_size);//fixme
                av_log(NULL, AV_LOG_ERROR, "sps: %p size: %"PRIu32"\n", sps, sps_size);//fixme
                av_log(NULL, AV_LOG_ERROR, "pps: %p size: %"PRIu32"\n", pps, pps_size);//fixme
                return AVERROR_INVALIDDATA;
            }

            avio_w8(pb,      1 ); /* configurationVersion */
            avio_w8(pb, sps[ 1]); /* general_profile_space, general_tier_flag, general_profile_idc */
            avio_w8(pb, sps[ 2]); /* general_profile_compatibility_flags 00..07 */
            avio_w8(pb, sps[ 3]); /* general_profile_compatibility_flags 08..15 */
            avio_w8(pb, sps[ 4]); /* general_profile_compatibility_flags 16..23 */
            avio_w8(pb, sps[ 5]); /* general_profile_compatibility_flags 24..31 */
            avio_w8(pb, sps[ 6]); /* general_constraint_indicator_flags  00..07 */
            avio_w8(pb, sps[ 7]); /* general_constraint_indicator_flags  08..15 */
            avio_w8(pb, sps[ 8]); /* general_constraint_indicator_flags  16..23 */
            avio_w8(pb, sps[ 9]); /* general_constraint_indicator_flags  24..31 */
            avio_w8(pb, sps[10]); /* general_constraint_indicator_flags  32..39 */
            avio_w8(pb, sps[11]); /* general_constraint_indicator_flags  40..47 */
            avio_w8(pb, sps[12]); /* general_level_idc */
            /*
             *          bit(4)  reserved = ‘1111’b;
             * unsigned int(12) min_spatial_segmentation_idc;
             *
             *          bit(6)  reserved = ‘111111’b;
             * unsigned int(2)  parallelismType;
             *
             *          bit(6)  reserved = ‘111111’b;
             * unsigned int(2)  chromaFormat;
             *
             *          bit(5)  reserved = ‘11111’b;
             * unsigned int(3)  bitDepthLumaMinus8;
             *
             *          bit(5)  reserved = ‘11111’b;
             * unsigned int(3)  bitDepthChromaMinus8;
             *
             *          bit(16) avgFrameRate;
             *
             * bit(2)           constantFrameRate;
             * bit(3)           numTemporalLayers;
             * bit(1)           temporalIdNested;
             * unsigned int(2)  lengthSizeMinusOne;
             *
             * unsigned int(8)  numOfArrays;
             *
             * for (j = 0; j < numOfArrays; j++) {
             *     bit(1) array_completeness;
             *     unsigned int(1) reserved = 0;
             *     unsigned int(6) NAL_unit_type;
             *     unsigned int(16) numNalus;
             *     for (i = 0; i < numNalus; i++) {
             *         unsigned int(16) nalUnitLength;
             *         bit(8*nalUnitLength) nalUnit;
             *     }
             * }
             */
        } else {
            avio_write(pb, data, len);
        }
    }
    return 0;
}

int ff_hevc_write_annexb_extradata(const uint8_t *in, uint8_t **buf, int *size)
{
    uint16_t sps_size, pps_size;
    uint8_t *out;
    int out_size;

    *buf = NULL;
    if (*size >= 4 && (AV_RB32(in) == 0x00000001 || AV_RB24(in) == 0x000001))
        return 0;
    if (*size < 11 || in[0] != 1)
        return AVERROR_INVALIDDATA;

    sps_size = AV_RB16(&in[6]);
    if (11 + sps_size > *size)
        return AVERROR_INVALIDDATA;
    pps_size = AV_RB16(&in[9 + sps_size]);
    if (11 + sps_size + pps_size > *size)
        return AVERROR_INVALIDDATA;
    out_size = 8 + sps_size + pps_size;
    out = av_mallocz(out_size);
    if (!out)
        return AVERROR(ENOMEM);
    AV_WB32(&out[0], 0x00000001);
    memcpy(out + 4, &in[8], sps_size);
    AV_WB32(&out[4 + sps_size], 0x00000001);
    memcpy(out + 8 + sps_size, &in[11 + sps_size], pps_size);
    *buf = out;
    *size = out_size;
    return 0;
}
