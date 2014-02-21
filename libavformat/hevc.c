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

static int binar_ize(uint8_t n)
{
    int i, ret = 0;
    for (i = 0; i < 8; i++) {
        if (n & (1 << i))
            ret += (int)pow(10, i);
    }
    return ret;
}

static int extract_bit(uint8_t n, int i)
{
    return ((n & (1 << (i & 7))) >> (i & 7));
}

static void dump_profile_tier_level(const char *buf_name, uint8_t *buf, int index, int maxNumSubLayersMinus1)
{
    int i, j, k;
    av_log(NULL, AV_LOG_FATAL, "\n%s index: %02d\n", buf_name, index);
    av_log(NULL, AV_LOG_FATAL, "general_profile_space:                  %3d\n", (buf[index] & 0xc0) >> 6);
    av_log(NULL, AV_LOG_FATAL, "general_tier_flag:                      %3d\n", (buf[index] & 0x20) >> 5);
    av_log(NULL, AV_LOG_FATAL, "general_profile_idc:                    %3d\n", (buf[index] & 0x1f)     );
    av_log(NULL, AV_LOG_FATAL, "%s index: %02d\n", buf_name, ++index);
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 8; j++) {
            k = i * 8;
            av_log(NULL,
                   AV_LOG_FATAL, "general_profile_compatibility_flag[%2d]: %3d\n", j + k, extract_bit(buf[index], j));
        }
        av_log(NULL, AV_LOG_FATAL, "%s index: %02d\n", buf_name, ++index);
    }
    av_log(NULL, AV_LOG_FATAL, "general_progressive_source_flag:        %3d\n", extract_bit(buf[index], 0));
    av_log(NULL, AV_LOG_FATAL, "general_interlaced_source_flag:         %3d\n", extract_bit(buf[index], 1));
    av_log(NULL, AV_LOG_FATAL, "general_non_packed_constraint_flag:     %3d\n", extract_bit(buf[index], 2));
    av_log(NULL, AV_LOG_FATAL, "general_frame_only_constraint_flag:     %3d\n", extract_bit(buf[index], 3));
    av_log(NULL, AV_LOG_FATAL, "general_reserved_zero_44bits[ 0]:       %3d\n", extract_bit(buf[index], 4));
    av_log(NULL, AV_LOG_FATAL, "general_reserved_zero_44bits[ 1]:       %3d\n", extract_bit(buf[index], 5));
    av_log(NULL, AV_LOG_FATAL, "general_reserved_zero_44bits[ 2]:       %3d\n", extract_bit(buf[index], 6));
    av_log(NULL, AV_LOG_FATAL, "general_reserved_zero_44bits[ 3]:       %3d\n", extract_bit(buf[index], 7));
    av_log(NULL, AV_LOG_FATAL, "%s index: %02d\n", buf_name, ++index);
    for(i = 0; i < 5; i++) {
        for(j = 0; j < 8; j++) {
            k = 3 + (i * 8) + 1;
            av_log(NULL,
                   AV_LOG_FATAL, "general_reserved_zero_44bits[%2d]:       %3d\n", j + k, extract_bit(buf[index], j));
        }
        av_log(NULL, AV_LOG_FATAL, "%s index: %02d\n", buf_name, ++index);
    }
    av_log(NULL, AV_LOG_FATAL, "general_level_idc:                      %3d\n", buf[index]);
    for(i = 0; i < maxNumSubLayersMinus1; i++) {
        //fixme: dump that too
        continue;
    }
    av_log(NULL, AV_LOG_FATAL, "\n");
}

int ff_isom_write_hvcc(AVIOContext *pb, const uint8_t *data, int len)
{
    HEVC_DEBUG_LOG("data: %p, len: %d\n", data, len);
    if (len > 6) {
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

                // forbidden bit
                if (buf[0] & 0x80)
                    return AVERROR_INVALIDDATA;

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
                    HEVC_DEBUG_LOG("NAL with type: %d and size: %d\n", nal_type, size);
                    break;
                }

                buf += size;
            }
            av_log(NULL, AV_LOG_FATAL, "\n");

            if (!vps || vps_size > UINT16_MAX ||
                !sps || sps_size > UINT16_MAX ||
                !pps || pps_size > UINT16_MAX) {
                av_log(NULL, AV_LOG_ERROR, "vps: %p size: %"PRIu32"\n", vps, vps_size);//fixme
                av_log(NULL, AV_LOG_ERROR, "sps: %p size: %"PRIu32"\n", sps, sps_size);//fixme
                av_log(NULL, AV_LOG_ERROR, "pps: %p size: %"PRIu32"\n", pps, pps_size);//fixme
                return AVERROR_INVALIDDATA;
            }

            /* Log the NAL unit's contents once we get here */
            av_log(NULL, AV_LOG_FATAL, "\n");
            for (int i = 0; i < vps_size; i++) {
                av_log(NULL, AV_LOG_FATAL, "vps[%2d]: %3d, 0x%02x, %04d %04d\n",
                       i, vps[i], vps[i],
                       binar_ize((vps[i] & 0xf0) >> 4),
                       binar_ize((vps[i] & 0x0f)));
            }
            av_log(NULL, AV_LOG_FATAL, "\n");
            for (int i = 0; i < sps_size; i++) {
                av_log(NULL, AV_LOG_FATAL, "sps[%2d]: %3d, 0x%02x, %04d %04d\n",
                       i, sps[i], sps[i],
                       binar_ize((sps[i] & 0xf0) >> 4),
                       binar_ize((sps[i] & 0x0f)));
            }
            av_log(NULL, AV_LOG_FATAL, "\n");
            for (int i = 0; i < pps_size; i++) {
                av_log(NULL, AV_LOG_FATAL, "pps[%2d]: %3d, 0x%02x, %04d %04d\n",
                       i, pps[i], pps[i],
                       binar_ize((pps[i] & 0xf0) >> 4),
                       binar_ize((pps[i] & 0x0f)));
            }
            av_log(NULL, AV_LOG_FATAL, "\n");

            /* Log the contents of the vars where I think these values are */
            dump_profile_tier_level("VPS", vps, 9, -1);
            dump_profile_tier_level("SPS", sps, 6, -1);

            /* HEVCDecoderConfigurationRecord */
            avio_w8(pb,      1 ); /* configurationVersion */
            avio_w8(pb, sps[ 6]); /* general_profile_space, general_tier_flag, general_profile_idc */
            avio_w8(pb, sps[ 7]); /* general_profile_compatibility_flags 00..07 */
            avio_w8(pb, sps[ 8]); /* general_profile_compatibility_flags 08..15 */
            avio_w8(pb, sps[ 9]); /* general_profile_compatibility_flags 16..23 */
            avio_w8(pb, sps[10]); /* general_profile_compatibility_flags 24..31 */
            avio_w8(pb, sps[11]); /* general_constraint_indicator_flags  00..07 */
            avio_w8(pb, sps[12]); /* general_constraint_indicator_flags  08..15 */
            avio_w8(pb, sps[13]); /* general_constraint_indicator_flags  16..23 */
            avio_w8(pb, sps[14]); /* general_constraint_indicator_flags  24..31 */
            avio_w8(pb, sps[15]); /* general_constraint_indicator_flags  32..39 */
            avio_w8(pb, sps[16]); /* general_constraint_indicator_flags  40..47 */
            avio_w8(pb, sps[17]); /* general_level_idc */
            /*
             * HEVCDecoderConfigurationRecord: continued
             *
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
