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
#include "avformat.h"
#include "avio.h"
#include "hevc.h"

#define HEVC_DEBUG()             av_log(NULL, AV_LOG_FATAL, "%s, %s: %d\n",           __FILE__, __FUNCTION__, __LINE__)
#define HEVC_DEBUG_LOG(str, ...) av_log(NULL, AV_LOG_FATAL, "%s, %s: %d - " str "\n", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

static const uint8_t *ff_hevc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
    HEVC_DEBUG();
    const uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(const uint32_t*)p;
//      if ((x - 0x01000100) & (~x) & 0x80008000)   // little endian
//      if ((x - 0x00010001) & (~x) & 0x00800080)   // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p+1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p+2;
                if (p[4] == 0 && p[5] == 1)
                    return p+3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}

const uint8_t *ff_hevc_find_startcode(const uint8_t *p, const uint8_t *end)
{
    HEVC_DEBUG();
    const uint8_t *out= ff_hevc_find_startcode_internal(p, end);
    if (p < out && out < end && !out[-1])
        out--;
    return out;
}

int ff_hevc_parse_nal_units(AVIOContext *pb, const uint8_t *buf_in, int size)
{
    HEVC_DEBUG();
    const uint8_t *p = buf_in;
    const uint8_t *end = p + size;
    const uint8_t *nal_start, *nal_end;

    size = 0;
    nal_start = ff_hevc_find_startcode(p, end);
    for (;;) {
        while (nal_start < end && !*(nal_start++));
        if (nal_start == end)
            break;

        nal_end = ff_hevc_find_startcode(nal_start, end);
        avio_wb32(pb, nal_end - nal_start);
        avio_write(pb, nal_start, nal_end - nal_start);
        size += 4 + nal_end - nal_start;
        nal_start = nal_end;
    }
    return size;
}

int ff_hevc_parse_nal_units_buf(const uint8_t *buf_in, uint8_t **buf, int *size)
{
    HEVC_DEBUG();
    AVIOContext *pb;
    int ret = avio_open_dyn_buf(&pb);
    if(ret < 0)
        return ret;

    ff_hevc_parse_nal_units(pb, buf_in, *size);

    av_freep(buf);
    *size = avio_close_dyn_buf(pb, buf);
    return 0;
}

int ff_isom_write_hvcc(AVIOContext *pb, const uint8_t *data, int len)
{
    HEVC_DEBUG_LOG("data: %p, len: %d", data, len);
    if (len > 6) {
        /* check for H.265 start code */
        if (AV_RB32(data) == 0x00000001 || AV_RB24(data) == 0x000001) {
            uint32_t vps_size = 0, sps_size = 0, pps_size = 0;
            uint8_t *buf = NULL, *end, *start, *vps = NULL, *sps = NULL, *pps = NULL;

            int ret = ff_hevc_parse_nal_units_buf(data, &buf, &len);
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
                nal_type = buf[0] & 0x1f;

                if        (nal_type == NAL_VPS) {
                    vps      = buf;
                    vps_size = size;
                } else if (nal_type == NAL_SPS) {
                    sps       = buf;
                    sps_size = size;
                } else if (nal_type == NAL_PPS) {
                    pps       = buf;
                    pps_size = size;
                }

                buf += size;
            }

            if (!vps || vps_size > UINT16_MAX ||
                !sps || sps_size > UINT16_MAX || sps_size < 4 ||
                !pps || pps_size > UINT16_MAX) {
                av_log(NULL, AV_LOG_ERROR, "vps: %p size: %"PRIu32"\n", vps, vps_size);//fixme
                av_log(NULL, AV_LOG_ERROR, "sps: %p size: %"PRIu32"\n", sps, sps_size);//fixme
                av_log(NULL, AV_LOG_ERROR, "pps: %p size: %"PRIu32"\n", pps, pps_size);//fixme
                return AVERROR_INVALIDDATA;
            }

            avio_w8 (pb, 1); /* configurationVersion */
            //fixme

            avio_wb16(pb, sps_size);
            avio_write(pb, sps, sps_size);
            avio_w8(pb, 1); /* number of pps */
            avio_wb16(pb, pps_size);
            avio_write(pb, pps, pps_size);
            av_free(start);
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
