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

#include "libavcodec/get_bits.h"
#include "libavcodec/golomb.h"
#include "libavcodec/hevc.h"
#include "libavutil/intreadwrite.h"
#include "avc.h"
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

static const AVRational vui_sar[] = {
    {  0,   1 },
    {  1,   1 },
    { 12,  11 },
    { 10,  11 },
    { 16,  11 },
    { 40,  33 },
    { 24,  11 },
    { 20,  11 },
    { 32,  11 },
    { 80,  33 },
    { 18,  11 },
    { 15,  11 },
    { 64,  33 },
    { 160, 99 },
    {  4,   3 },
    {  3,   2 },
    {  2,   1 },
};

static void hvcc_init(HEVCDecoderConfigurationRecord *hvcc)
{
    memset(hvcc, 0, sizeof(HEVCDecoderConfigurationRecord));
    hvcc->configurationVersion = 1;
    hvcc->lengthSizeMinusOne   = 3; // 4 bytes
}

static void parse_nal_header(GetBitContext *gb, uint8_t *nal_type)
{
    skip_bits1(gb); // forbidden_zero_bit

    *nal_type = get_bits(gb, 6);

    skip_bits(gb, 6); // nuh_layer_id
    skip_bits(gb, 3); // nuh_temporal_id_plus1
}

static void hvcc_parse_ptl(GetBitContext *gb,
                           HEVCDecoderConfigurationRecord *hvcc,
                           int max_sub_layers_minus1)
{
    int i, num_sub_layers;
    uint8_t  general_profile_space;
    uint8_t  general_tier_flag;
    uint8_t  general_profile_idc;
    uint32_t general_profile_compatibility_flags;
    uint64_t general_constraint_indicator_flags;
    uint8_t  general_level_idc;
    uint8_t  sub_layer_profile_present_flag[MAX_SUB_LAYERS];
    uint8_t  sub_layer_level_present_flag[MAX_SUB_LAYERS];

    general_profile_space               = get_bits     (gb, 2);
    general_tier_flag                   = get_bits1    (gb);
    general_profile_idc                 = get_bits     (gb, 5);
    general_profile_compatibility_flags = get_bits_long(gb, 32);
    general_constraint_indicator_flags  = get_bits64   (gb, 48);
    general_level_idc                   = get_bits     (gb, 8);

    //fixme?
    hvcc->general_profile_space                = FFMAX(general_profile_space, hvcc->general_profile_space);
    hvcc->general_tier_flag                   |= general_tier_flag;
    hvcc->general_profile_idc                  = FFMAX(general_profile_idc, hvcc->general_profile_idc);
    hvcc->general_level_idc                    = FFMAX(general_level_idc, hvcc->general_level_idc);
    hvcc->general_profile_compatibility_flags |= general_profile_compatibility_flags;
    hvcc->general_constraint_indicator_flags  |= general_constraint_indicator_flags;

    num_sub_layers = FFMIN(max_sub_layers_minus1, MAX_SUB_LAYERS);
    
    for (i = 0; i < num_sub_layers; i++) {
        sub_layer_profile_present_flag[i] = get_bits1(gb);
        sub_layer_level_present_flag[i]   = get_bits1(gb);
    }

    if (num_sub_layers > 0)
        for (i = num_sub_layers; i < 8; i++)
            skip_bits(gb, 2); // reserved_zero_2bits[i]

    for (i = 0; i < num_sub_layers; i++) {
        if (sub_layer_profile_present_flag[i]) {
            skip_bits_long(gb, 32);
            skip_bits_long(gb, 32);
            skip_bits     (gb, 24);
        }

        if (sub_layer_level_present_flag[i])
            skip_bits(gb, 8);
    }
}

static void skip_scaling_list_data(GetBitContext *gb)
{
    int i, j, k, num_coeffs;

    for (i = 0; i < 4; i++)
        for (j = 0; j < (i == 3 ? 2 : 6); j++)
            if (!get_bits1(gb))         // scaling_list_pred_mode_flag[i][j]
                get_ue_golomb_long(gb); // scaling_list_pred_matrix_id_delta[i][j]
            else {
                num_coeffs = FFMIN(64, 1 << (4 + (i << 1)));
                if (i > 1)
                    get_se_golomb(gb); // scaling_list_dc_coef_minus8[i-2][j]
                for (k = 0; k < num_coeffs; k++)
                    get_se_golomb(gb); // scaling_list_delta_coef
            }
}

static int decode_rps(GetBitContext *gb, int rps_idx, int num_rps,
                      int num_delta_pocs[MAX_SHORT_TERM_RPS_COUNT])
{
    int i;

    if (rps_idx && get_bits1(gb)) { // inter_ref_pic_set_prediction_flag
        // this should only happen for slice headers, and this isn't one
        if (rps_idx >= num_rps)
            return AVERROR_INVALIDDATA;

        skip_bits1        (gb); // delta_rps_sign
        get_ue_golomb_long(gb); // abs_delta_rps_minus1

        num_delta_pocs[rps_idx] = 0;

        /*
         * From libavcodec/hevc_ps.c:
         *
         * if (is_slice_header) {
         *    //foo
         * } else
         *     rps_ridx = &sps->st_rps[rps - sps->st_rps - 1];
         *
         * where:
         * rps:             &sps->st_rps[rps_idx]
         * sps->st_rps:     &sps->st_rps[0]
         * is_slice_header: rps_idx == num_rps
         *
         * thus:
         * if (num_rps != rps_idx)
         *     rps_ridx = &sps->st_rps[rps_idx - 1];
         *
         * NumDeltaPocs[RefRpsIdx]: num_delta_pocs[rps_idx - 1]
         */
        for (i = 0; i < num_delta_pocs[rps_idx - 1]; i++) {
            uint8_t use_delta_flag = 0;
            uint8_t used_by_curr_pic_flag = get_bits1(gb);
            if (!used_by_curr_pic_flag)
                use_delta_flag = get_bits1(gb);

            if (used_by_curr_pic_flag || use_delta_flag)
                num_delta_pocs[rps_idx]++;
        }
    } else {
        unsigned int num_negative_pics = get_ue_golomb_long(gb);
        unsigned int num_positive_pics = get_ue_golomb_long(gb);

        num_delta_pocs[rps_idx] = num_negative_pics + num_positive_pics;

        for (i = 0; i < num_negative_pics; i++) {
            get_ue_golomb_long(gb); // delta_poc_s0_minus1[rps_idx]
            skip_bits1        (gb); // used_by_curr_pic_s0_flag[rps_idx]
        }

        for (i = 0; i < num_positive_pics; i++) {
            get_ue_golomb_long(gb); // delta_poc_s1_minus1[rps_idx]
            skip_bits1        (gb); // used_by_curr_pic_s1_flag[rps_idx]
        }
    }

    return 0;
}

static void skip_sub_layer_hrd_parameters(GetBitContext *gb, int cpb_cnt_minus1,
                                          int sub_pic_hrd_params_present_flag)
{
    int i;

    for (i = 0; i <= cpb_cnt_minus1; i++) {
        get_ue_golomb_long(gb); // bit_rate_value_minus1
        get_ue_golomb_long(gb); // cpb_size_value_minus1

        if (sub_pic_hrd_params_present_flag) {
            get_ue_golomb_long(gb); // cpb_size_du_value_minus1
            get_ue_golomb_long(gb); // bit_rate_du_value_minus1
        }
        skip_bits1(gb); // cbr_flag
    }
}

static void skip_hrd_parameters(GetBitContext *gb, int max_sub_layers_minus1)
{
    int i;
    uint8_t sub_pic_hrd_params_present_flag = 0;
    uint8_t nal_hrd_parameters_present_flag = get_bits1(gb);
    uint8_t vcl_hrd_parameters_present_flag = get_bits1(gb);

    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
        sub_pic_hrd_params_present_flag = get_bits1(gb);
        if (sub_pic_hrd_params_present_flag) {
            skip_bits(gb, 8); // tick_divisor_minus2
            skip_bits(gb, 5); // du_cpb_removal_delay_increment_length_minus1
            skip_bits(gb, 1); // sub_pic_cpb_params_in_pic_timing_sei_flag
            skip_bits(gb, 5); // dpb_output_delay_du_length_minus1
        }

        skip_bits(gb, 4); // bit_rate_scale
        skip_bits(gb, 4); // cpb_size_scale

        if (sub_pic_hrd_params_present_flag)
            skip_bits(gb, 4);  // cpb_size_du_scale

        skip_bits(gb, 5); // initial_cpb_removal_delay_length_minus1
        skip_bits(gb, 5); // au_cpb_removal_delay_length_minus1
        skip_bits(gb, 5); // dpb_output_delay_length_minus1
    }

    for (i = 0; i <= max_sub_layers_minus1; i++) {
        int cpb_cnt_minus1                 = 0;
        int low_delay_hrd_flag             = 0;
        int fixed_pic_rate_within_cvs_flag = 0;
        int fixed_pic_rate_general_flag    = get_bits1(gb);
        
        if (!fixed_pic_rate_general_flag)
            fixed_pic_rate_within_cvs_flag = get_bits1(gb);
        
        if (fixed_pic_rate_within_cvs_flag)
            get_ue_golomb_long(gb);  // elemental_duration_in_tc_minus1
        else
            low_delay_hrd_flag = get_bits1(gb);
        
        if (!low_delay_hrd_flag)
            cpb_cnt_minus1 = get_ue_golomb_long(gb);
        
        if (nal_hrd_parameters_present_flag)
            skip_sub_layer_hrd_parameters(gb, cpb_cnt_minus1,
                                          sub_pic_hrd_params_present_flag);
        if (vcl_hrd_parameters_present_flag)
            skip_sub_layer_hrd_parameters(gb, cpb_cnt_minus1,
                                          sub_pic_hrd_params_present_flag);
    }
}

static void decode_vui(GetBitContext *gb, VUI *vui, int max_sub_layers_minus1)
{
    if (get_bits1(gb)) { // aspect_ratio_info_present_flag
        uint8_t sar_idx = get_bits(gb, 8);
        if (sar_idx < FF_ARRAY_ELEMS(vui_sar))
            vui->sar = vui_sar[sar_idx];
        else if (sar_idx == 255) {
            vui->sar.num = get_bits(gb, 16);
            vui->sar.den = get_bits(gb, 16);
        }
    }

    vui->overscan_info_present_flag = get_bits1(gb);
    if (vui->overscan_info_present_flag)
        vui->overscan_appropriate_flag = get_bits1(gb);

    vui->video_signal_type_present_flag = get_bits1(gb);
    if (vui->video_signal_type_present_flag) {
        vui->video_format          = get_bits (gb, 3);
        vui->video_full_range_flag = get_bits1(gb);

        vui->colour_description_present_flag = get_bits1(gb);
        if (vui->colour_description_present_flag) {
            vui->colour_primaries        = get_bits(gb, 8);
            vui->transfer_characteristic = get_bits(gb, 8);
            vui->matrix_coeffs           = get_bits(gb, 8);
        }
    }

    vui->chroma_loc_info_present_flag = get_bits1(gb);
    if (vui->chroma_loc_info_present_flag) {
        vui->chroma_sample_loc_type_top_field    = get_ue_golomb_long(gb);
        vui->chroma_sample_loc_type_bottom_field = get_ue_golomb_long(gb);
    }

    vui->neutra_chroma_indication_flag = get_bits1(gb);
    vui->field_seq_flag                = get_bits1(gb);
    vui->frame_field_info_present_flag = get_bits1(gb);

    vui->default_display_window_flag = get_bits1(gb);
    if (vui->default_display_window_flag) {
        // TODO: * 2 is only valid for 4:2:0
        vui->def_disp_win.left_offset   = get_ue_golomb_long(gb) * 2;
        vui->def_disp_win.right_offset  = get_ue_golomb_long(gb) * 2;
        vui->def_disp_win.top_offset    = get_ue_golomb_long(gb) * 2;
        vui->def_disp_win.bottom_offset = get_ue_golomb_long(gb) * 2;
    }

    vui->vui_timing_info_present_flag = get_bits1(gb);
    if (vui->vui_timing_info_present_flag) {
        vui->vui_num_units_in_tick = get_bits_long(gb, 32);
        vui->vui_time_scale        = get_bits_long(gb, 32);

        vui->vui_poc_proportional_to_timing_flag = get_bits1(gb);
        if (vui->vui_poc_proportional_to_timing_flag)
            vui->vui_num_ticks_poc_diff_one_minus1 = get_ue_golomb_long(gb);

        vui->vui_hrd_parameters_present_flag = get_bits1(gb);
        if (vui->vui_hrd_parameters_present_flag)
            skip_hrd_parameters(gb, max_sub_layers_minus1);
    }

    vui->bitstream_restriction_flag = get_bits1(gb);
    if (vui->bitstream_restriction_flag) {
        vui->tiles_fixed_structure_flag              = get_bits1(gb);
        vui->motion_vectors_over_pic_boundaries_flag = get_bits1(gb);
        vui->restricted_ref_pic_lists_flag           = get_bits1(gb);
        vui->min_spatial_segmentation_idc            = get_ue_golomb_long(gb);
        vui->max_bytes_per_pic_denom                 = get_ue_golomb_long(gb);
        vui->max_bits_per_min_cu_denom               = get_ue_golomb_long(gb);
        vui->log2_max_mv_length_horizontal           = get_ue_golomb_long(gb);
        vui->log2_max_mv_length_vertical             = get_ue_golomb_long(gb);
    }
}

static int hvcc_parse_sps(uint8_t *sps_buf, int sps_size,
                          HEVCDecoderConfigurationRecord *hvcc)
{
    uint8_t nal_type;
    GetBitContext gb;
    VUI vui = { { 0 } };
    int i, ret;
    int log2_max_pic_order_cnt_lsb_minus4;
    int chroma_format_idc, sps_max_sub_layers_minus1;
    int num_short_term_ref_pic_sets, num_delta_pocs[MAX_SHORT_TERM_RPS_COUNT];

    ret = init_get_bits8(&gb, sps_buf, sps_size);
    if (ret < 0)
        return ret;

    parse_nal_header(&gb, &nal_type);
    if (nal_type != NAL_SPS)
        return AVERROR_INVALIDDATA;

    // FIXME: clearly there's something here (extract_rbsp)?

    skip_bits(&gb, 4); // sps_video_parameter_set_id

    sps_max_sub_layers_minus1 = get_bits (&gb, 3);
    hvcc->temporalIdNested    = get_bits1(&gb);

    hvcc_parse_ptl(&gb, hvcc, sps_max_sub_layers_minus1);

    get_ue_golomb_long(&gb); // sps_seq_parameter_set_id

    chroma_format_idc  = get_ue_golomb_long(&gb);
    hvcc->chromaFormat = chroma_format_idc & 0x3;

    if (chroma_format_idc == 3)
        skip_bits1(&gb); // separate_colour_plane_flag

    get_ue_golomb_long(&gb); // pic_width_in_luma_samples
    get_ue_golomb_long(&gb); // pic_height_in_luma_samples

    if (get_bits1(&gb)) {        // conformance_window_flag
        get_ue_golomb_long(&gb); // conf_win_left_offset
        get_ue_golomb_long(&gb); // conf_win_right_offset
        get_ue_golomb_long(&gb); // conf_win_top_offset
        get_ue_golomb_long(&gb); // conf_win_bottom_offset
    }

    hvcc->bitDepthLumaMinus8   = get_ue_golomb_long(&gb) & 0x7;
    hvcc->bitDepthChromaMinus8 = get_ue_golomb_long(&gb) & 0x7;

    log2_max_pic_order_cnt_lsb_minus4 = get_ue_golomb_long(&gb);

    for (i = get_bits1(&gb) ? 0 : sps_max_sub_layers_minus1;
         i <= sps_max_sub_layers_minus1; i++) {
        get_ue_golomb_long(&gb); // sps_max_dec_pic_buffering_minus1[i]
        get_ue_golomb_long(&gb); // sps_max_num_reorder_pics[i]
        get_ue_golomb_long(&gb); // sps_max_latency_increase_plus1[i]
    }

    get_ue_golomb_long(&gb); // log2_min_luma_coding_block_size_minus3
    get_ue_golomb_long(&gb); // log2_diff_max_min_luma_coding_block_size
    get_ue_golomb_long(&gb); // log2_min_transform_block_size_minus2
    get_ue_golomb_long(&gb); // log2_diff_max_min_transform_block_size
    get_ue_golomb_long(&gb); // max_transform_hierarchy_depth_inter
    get_ue_golomb_long(&gb); // max_transform_hierarchy_depth_intra

    if (get_bits1(&gb) && // scaling_list_enabled_flag
        get_bits1(&gb))   // sps_scaling_list_data_present_flag
        skip_scaling_list_data(&gb);
            

    skip_bits1(&gb); // amp_enabled_flag
    skip_bits1(&gb); // sample_adaptive_offset_enabled_flag

    if (get_bits1(&gb)) {           // pcm_enabled_flag
        skip_bits         (&gb, 4); // pcm_sample_bit_depth_luma_minus1
        skip_bits         (&gb, 4); // pcm_sample_bit_depth_chroma_minus1
        get_ue_golomb_long(&gb);    // log2_min_pcm_luma_coding_block_size_minus3
        get_ue_golomb_long(&gb);    // log2_diff_max_min_pcm_luma_coding_block_size
        skip_bits1        (&gb);    // pcm_loop_filter_disabled_flag
    }

    num_short_term_ref_pic_sets = get_ue_golomb_long(&gb);
    if (num_short_term_ref_pic_sets > MAX_SHORT_TERM_RPS_COUNT)
        return AVERROR_INVALIDDATA;

    for (i = 0; i < num_short_term_ref_pic_sets; i++) {
        ret = decode_rps(&gb, i, num_short_term_ref_pic_sets, num_delta_pocs);
        if (ret < 0)
            return ret;
    }

    if (get_bits1(&gb)) {                               // long_term_ref_pics_present_flag
        for (i = 0; i < get_ue_golomb_long(&gb); i++) { // num_long_term_ref_pics_sps
            int len = FFMIN(log2_max_pic_order_cnt_lsb_minus4 + 4, 16);
            skip_bits (&gb, len); // lt_ref_pic_poc_lsb_sps[i]
            skip_bits1(&gb);      // used_by_curr_pic_lt_sps_flag[i]
        }
    }

    skip_bits1(&gb); // sps_temporal_mvp_enabled_flag
    skip_bits1(&gb); // strong_intra_smoothing_enabled_flag

    hvcc->min_spatial_segmentation_idc = 0;
    hvcc->parallelismType              = 0;
    if (get_bits1(&gb)) { // vui_parameters_present_flag
        decode_vui(&gb, &vui, sps_max_sub_layers_minus1);
        if (vui.bitstream_restriction_flag)
            hvcc->min_spatial_segmentation_idc = vui.min_spatial_segmentation_idc;
    }

    return 0;
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
                int ret;
                uint32_t size;
                uint8_t nal_type;
                GetBitContext gb;
                size = FFMIN(AV_RB32(buf), end - buf - 4);
                buf += 4;

                ret = init_get_bits8(&gb, buf, size);
                if (ret < 0)
                    return ret;

                parse_nal_header(&gb, &nal_type);

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


            HEVCDecoderConfigurationRecord hvcc;
            hvcc_init(&hvcc);
            ret = hvcc_parse_sps(sps, sps_size, &hvcc);
            if (ret < 0)
                return ret;

            /* HEVCDecoderConfigurationRecord */
            avio_w8  (pb, hvcc.configurationVersion);
            avio_w8  (pb, hvcc.general_profile_space << 6 |
                          hvcc.general_tier_flag     << 5 |
                          hvcc.general_profile_idc);
            avio_wb32(pb, hvcc.general_profile_compatibility_flags);
            avio_wb32(pb, hvcc.general_constraint_indicator_flags >> 16);
            avio_wb16(pb, hvcc.general_constraint_indicator_flags);
            avio_w8  (pb, hvcc.general_level_idc);
            avio_wb16(pb, hvcc.min_spatial_segmentation_idc | 0xf000);
            avio_w8  (pb, hvcc.parallelismType              | 0xfc);
            avio_w8  (pb, hvcc.chromaFormat                 | 0xfc);
            avio_w8  (pb, hvcc.bitDepthLumaMinus8           | 0xf8);
            avio_w8  (pb, hvcc.bitDepthChromaMinus8         | 0xf8);
            avio_wb16(pb, hvcc.avgFrameRate);
            avio_w8  (pb, hvcc.constantFrameRate << 6 |
                          hvcc.numTemporalLayers << 3 |
                          hvcc.temporalIdNested  << 2 |
                          hvcc.lengthSizeMinusOne);
            /*
             * HEVCDecoderConfigurationRecord: continued
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
