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

#define MAX_SPATIAL_SEGMENTATION 4096 // max. value of u(12) field

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

static int binar_ize(uint8_t n)
{
    int i, ret = 0;
    for (i = 0; i < 8; i++)
        if (n & (1 << i))
            ret += (int)pow(10, i);
    return ret;
}

#define HEVC_DEBUG_LOG(str, ...) av_log(NULL, AV_LOG_FATAL, "%s, %s: %d - " str, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
static void dump_hvcc(HEVCDecoderConfigurationRecord *hvcc)
{
    uint8_t *tmp;
    av_log(NULL, AV_LOG_FATAL, "\n");
    av_log(NULL, AV_LOG_FATAL, "hvcc: configurationVersion:                %4"PRIu8"\n",    hvcc->configurationVersion);
    av_log(NULL, AV_LOG_FATAL, "hvcc: general_profile_space:               %4"PRIu8"\n",    hvcc->general_profile_space);
    av_log(NULL, AV_LOG_FATAL, "hvcc: general_tier_flag:                   %4"PRIu8"\n",    hvcc->general_tier_flag);
    av_log(NULL, AV_LOG_FATAL, "hvcc: general_profile_idc:                 %4"PRIu8"\n",    hvcc->general_profile_idc);
    tmp = (uint8_t*)&hvcc->general_profile_compatibility_flags;
    av_log(NULL, AV_LOG_FATAL, "hvcc: general_profile_compatibility_flags: %04d (0x%08"PRIx32")\n",
           binar_ize((tmp[3] & 0xf0) >> 4), hvcc->general_profile_compatibility_flags);
    tmp = (uint8_t*)&hvcc->general_constraint_indicator_flags;
    av_log(NULL, AV_LOG_FATAL, "hvcc: general_constraint_indicator_flags:  %04d (0x%012"PRIx64")\n",
           binar_ize((tmp[5] & 0xf0) >> 4), hvcc->general_constraint_indicator_flags);
    av_log(NULL, AV_LOG_FATAL, "hvcc: general_level_idc:                   %4"PRIu8"\n",    hvcc->general_level_idc);
    av_log(NULL, AV_LOG_FATAL, "hvcc: min_spatial_segmentation_idc:        %4"PRIu16"\n",   hvcc->min_spatial_segmentation_idc);
    av_log(NULL, AV_LOG_FATAL, "hvcc: parallelismType:                     %4"PRIu8"\n",    hvcc->parallelismType);
    av_log(NULL, AV_LOG_FATAL, "hvcc: chromaFormat:                        %4"PRIu8"\n",    hvcc->chromaFormat);
    av_log(NULL, AV_LOG_FATAL, "hvcc: bitDepthLumaMinus8:                  %4"PRIu8"\n",    hvcc->bitDepthLumaMinus8);
    av_log(NULL, AV_LOG_FATAL, "hvcc: bitDepthChromaMinus8:                %4"PRIu8"\n",    hvcc->bitDepthChromaMinus8);
    av_log(NULL, AV_LOG_FATAL, "hvcc: avgFrameRate:                        %4"PRIu16"\n",   hvcc->avgFrameRate);
    av_log(NULL, AV_LOG_FATAL, "hvcc: constantFrameRate:                   %4"PRIu8"\n",    hvcc->constantFrameRate);
    av_log(NULL, AV_LOG_FATAL, "hvcc: numTemporalLayers:                   %4"PRIu8"\n",    hvcc->numTemporalLayers);
    av_log(NULL, AV_LOG_FATAL, "hvcc: temporalIdNested:                    %4"PRIu8"\n",    hvcc->temporalIdNested);
    av_log(NULL, AV_LOG_FATAL, "hvcc: lengthSizeMinusOne:                  %4"PRIu8"\n",    hvcc->lengthSizeMinusOne);
    av_log(NULL, AV_LOG_FATAL, "\n");
}

static int nal_unit_extract_rbsp(const uint8_t *src, int src_len, uint8_t **dst,
                                 int *dst_len)
{
    int i, len;
    uint8_t *buf;

    if (!dst)
        return AVERROR_BUG;

    av_fast_malloc(dst, dst_len, src_len);
    buf = *dst;
    if (!buf)
        return AVERROR(ENOMEM);

    // always copy the header (2 bytes)
    i = len = 0;
    while (i < 2 && i < src_len)
        buf[len++] = src[i++];

    while (i + 2 < src_len)
        if (!src[i] && !src[i + 1] && src[i + 2] == 3) {
            buf[len++] = src[i++];
            buf[len++] = src[i++];
            i++; // remove emulation_prevention_three_byte
        } else
            buf[len++] = src[i++];

    while (i < src_len)
        buf[len++] = src[i++];

    *dst     = buf;
    *dst_len = len;
    return i - len;
}

static void dump_nal_full(uint8_t *buf, int size, const char *name)
{
    av_log(NULL, AV_LOG_FATAL, "\n");
    for (int i = 0; i < size; i++) {
        av_log(NULL, AV_LOG_FATAL, "%s[%2d]: %3d, 0x%02x, %04d %04d\n", name,
               i, buf[i], buf[i],
               binar_ize((buf[i] & 0xf0) >> 4),
               binar_ize((buf[i] & 0x0f)));
    }
    av_log(NULL, AV_LOG_FATAL, "\n");
}

static void dump_nal_rbsp(uint8_t *buf, int size, const char *name)
{
    uint8_t *tmp = NULL;
    int tmp_size = 0;

    if (nal_unit_extract_rbsp((const uint8_t*)buf, size, &tmp, &tmp_size) < 0)
        return;

    buf  = tmp;
    size = tmp_size;

    av_log(NULL, AV_LOG_FATAL, "\n");
    for (int i = 0; i < size; i++) {
        av_log(NULL, AV_LOG_FATAL, "%s[%2d]: %3d, 0x%02x, %04d %04d\n", name,
               i, buf[i], buf[i],
               binar_ize((buf[i] & 0xf0) >> 4),
               binar_ize((buf[i] & 0x0f)));
    }
    av_log(NULL, AV_LOG_FATAL, "\n");
}

static void nal_unit_parse_header(GetBitContext *gb, uint8_t *nal_type)
{
    skip_bits1(gb); // forbidden_zero_bit

    *nal_type = get_bits(gb, 6);

    // nuh_layer_id          u(6)
    // nuh_temporal_id_plus1 u(3)
    skip_bits(gb, 9);
}

static void hvcc_init(HEVCDecoderConfigurationRecord *hvcc)
{
    memset(hvcc, 0, sizeof(HEVCDecoderConfigurationRecord));
    hvcc->configurationVersion = 1;
    hvcc->lengthSizeMinusOne   = 3; // 4 bytes

    // the following fields have all their valid bits set by default
    // the VPS/SPS/PPS parsing code will unset the bits as necessary
    hvcc->general_profile_compatibility_flags = 0xffffffff;
    hvcc->general_constraint_indicator_flags  = 0xffffffffffff;

    // initialize this field with an invalid value which can be used to detect
    // whether we didn't see any VUI (in wich case it should be reset to zero)
    hvcc->min_spatial_segmentation_idc = MAX_SPATIAL_SEGMENTATION + 1;
}

static void hvcc_finalize(HEVCDecoderConfigurationRecord *hvcc)
{
    if (hvcc->min_spatial_segmentation_idc > MAX_SPATIAL_SEGMENTATION)
        hvcc->min_spatial_segmentation_idc = 0;

    /*
     * parallelismType indicates the type of parallelism that is used to meet
     * the restrictions imposed by min_spatial_segmentation_idc when the value
     * of min_spatial_segmentation_idc is greater than 0.
     */
    if (!hvcc->min_spatial_segmentation_idc)
        hvcc->parallelismType = 0;
}

static void hvcc_update_ptl(HEVCDecoderConfigurationRecord *hvcc,
                            HVCCProfileTierLevel *ptl)
{
    /*
     * The value of general_profile_space in all the parameter sets must be
     * identical.
     */
    hvcc->general_profile_space = ptl->profile_space;

    /*
     * The level indication general_level_idc must indicate a level of
     * capability equal to or greater than the highest level indicated for the
     * highest tier in all the parameter sets.
     */
    if (hvcc->general_tier_flag < ptl->tier_flag)
        hvcc->general_level_idc = ptl->level_idc;
    else
        hvcc->general_level_idc = FFMAX(hvcc->general_level_idc, ptl->level_idc);

    /*
     * The tier indication general_tier_flag must indicate a tier equal to or
     * greater than the highest tier indicated in all the parameter sets.
     */
    hvcc->general_tier_flag = FFMAX(hvcc->general_tier_flag, ptl->tier_flag);

    /*
     * The profile indication general_profile_idc must indicate a profile to
     * which the stream associated with this configuration record conforms.
     *
     * If the sequence parameter sets are marked with different profiles, then
     * the stream may need examination to determine which profile, if any, the
     * entire stream conforms to. If the entire stream is not examined, or the
     * examination reveals that there is no profile to which the entire stream
     * conforms, then the entire stream must be split into two or more
     * sub-streams with separate configuration records in which these rules can
     * be met.
     *
     * Note: set the profile to the highest value for the sake of simplicity.
     */
    hvcc->general_profile_idc = FFMAX(hvcc->general_profile_idc, ptl->profile_idc);
        

    /*
     * Each bit in general_profile_compatibility_flags may only be set if all
     * the parameter sets set that bit.
     */
    hvcc->general_profile_compatibility_flags &= ptl->profile_compatibility_flags;

    /*
     * Each bit in general_constraint_indicator_flags may only be set if all
     * the parameter sets set that bit.
     */
    hvcc->general_constraint_indicator_flags &= ptl->constraint_indicator_flags;
}

static void hvcc_parse_ptl(GetBitContext *gb,
                           HEVCDecoderConfigurationRecord *hvcc,
                           int max_sub_layers_minus1)
{
    int i;
    HVCCProfileTierLevel general_ptl;
    uint8_t sub_layer_profile_present_flag[MAX_SUB_LAYERS];
    uint8_t sub_layer_level_present_flag[MAX_SUB_LAYERS];

    general_ptl.profile_space               = get_bits     (gb, 2);
    general_ptl.tier_flag                   = get_bits1    (gb);
    general_ptl.profile_idc                 = get_bits     (gb, 5);
    general_ptl.profile_compatibility_flags = get_bits_long(gb, 32);
    general_ptl.constraint_indicator_flags  = get_bits64   (gb, 48);
    general_ptl.level_idc                   = get_bits     (gb, 8);
    hvcc_update_ptl(hvcc, &general_ptl);

    for (i = 0; i < max_sub_layers_minus1; i++) {
        sub_layer_profile_present_flag[i] = get_bits1(gb);
        sub_layer_level_present_flag[i]   = get_bits1(gb);
    }

    if (max_sub_layers_minus1 > 0)
        for (i = max_sub_layers_minus1; i < 8; i++)
            skip_bits(gb, 2); // reserved_zero_2bits[i]

    for (i = 0; i < max_sub_layers_minus1; i++) {
        if (sub_layer_profile_present_flag[i]) {
            // sub_layer_profile_space[i]                     u(2)
            // sub_layer_tier_flag[i]                         u(1)
            // sub_layer_profile_idc[i]                       u(5)
            // sub_layer_profile_compatibility_flag[i][0..31] u(32)
            // sub_layer_progressive_source_flag[i]           u(1)
            // sub_layer_interlaced_source_flag[i]            u(1)
            // sub_layer_non_packed_constraint_flag[i]        u(1)
            // sub_layer_frame_only_constraint_flag[i]        u(1)
            // sub_layer_reserved_zero_44bits[i]              u(44)
            skip_bits_long(gb, 32);
            skip_bits_long(gb, 32);
            skip_bits     (gb, 24);
        }

        if (sub_layer_level_present_flag[i])
            skip_bits(gb, 8);
    }
}

static void skip_sub_layer_hrd_parameters(GetBitContext *gb, int cpb_cnt_minus1,
                                          int sub_pic_hrd_params_present_flag)
{
    int i;

    for (i = 0; i <= cpb_cnt_minus1; i++) {
        get_ue_golomb(gb); // bit_rate_value_minus1
        get_ue_golomb(gb); // cpb_size_value_minus1

        if (sub_pic_hrd_params_present_flag) {
            get_ue_golomb(gb); // cpb_size_du_value_minus1
            get_ue_golomb(gb); // bit_rate_du_value_minus1
        }
        skip_bits1(gb); // cbr_flag
    }
}

static void skip_hrd_parameters(GetBitContext *gb, int common_inf_present_flag,
                                int max_sub_layers_minus1)
{
    int i;
    uint8_t sub_pic_hrd_params_present_flag = 0;
    uint8_t nal_hrd_parameters_present_flag = 0;
    uint8_t vcl_hrd_parameters_present_flag = 0;

    if (common_inf_present_flag) {
        nal_hrd_parameters_present_flag = get_bits1(gb);
        vcl_hrd_parameters_present_flag = get_bits1(gb);

        if (nal_hrd_parameters_present_flag ||
            vcl_hrd_parameters_present_flag) {
            sub_pic_hrd_params_present_flag = get_bits1(gb);

            if (sub_pic_hrd_params_present_flag)
                // tick_divisor_minus2                          u(8)
                // du_cpb_removal_delay_increment_length_minus1 u(5)
                // sub_pic_cpb_params_in_pic_timing_sei_flag    u(1)
                // dpb_output_delay_du_length_minus1            u(5)
                skip_bits(gb, 19);

            // bit_rate_scale u(4)
            // cpb_size_scale u(4)
            skip_bits(gb, 8);

            if (sub_pic_hrd_params_present_flag)
                skip_bits(gb, 4); // cpb_size_du_scale

            // initial_cpb_removal_delay_length_minus1 u(5)
            // au_cpb_removal_delay_length_minus1      u(5)
            // dpb_output_delay_length_minus1          u(5)
            skip_bits(gb, 15);
        }
    }

    for (i = 0; i <= max_sub_layers_minus1; i++) {
        int cpb_cnt_minus1                 = 0;
        int low_delay_hrd_flag             = 0;
        int fixed_pic_rate_within_cvs_flag = 0;
        int fixed_pic_rate_general_flag    = get_bits1(gb);

        if (!fixed_pic_rate_general_flag)
            fixed_pic_rate_within_cvs_flag = get_bits1(gb);

        if (fixed_pic_rate_within_cvs_flag)
            get_ue_golomb(gb); // elemental_duration_in_tc_minus1
        else
            low_delay_hrd_flag = get_bits1(gb);

        if (!low_delay_hrd_flag)
            cpb_cnt_minus1 = get_ue_golomb(gb);

        if (nal_hrd_parameters_present_flag)
            skip_sub_layer_hrd_parameters(gb, cpb_cnt_minus1,
                                          sub_pic_hrd_params_present_flag);

        if (vcl_hrd_parameters_present_flag)
            skip_sub_layer_hrd_parameters(gb, cpb_cnt_minus1,
                                          sub_pic_hrd_params_present_flag);
    }
}

static void skip_timing_info(GetBitContext *gb)
{
    skip_bits_long(gb, 32); // num_units_in_tick
    skip_bits_long(gb, 32); // time_scale

    if (get_bits1(gb))          // poc_proportional_to_timing_flag
        get_ue_golomb_long(gb); // num_ticks_poc_diff_one_minus1
}

static void hvcc_parse_vui(GetBitContext *gb,
                           HEVCDecoderConfigurationRecord *hvcc,
                           int max_sub_layers_minus1)
{
    int min_spatial_segmentation_idc;

    if (get_bits1(gb))              // aspect_ratio_info_present_flag
        if (get_bits(gb, 8) == 255) // aspect_ratio_idc
            skip_bits_long(gb, 32); // sar_width u(16), sar_height u(16)

    if (get_bits1(gb))  // overscan_info_present_flag
        skip_bits1(gb); // overscan_appropriate_flag

    if (get_bits1(gb)) {  // video_signal_type_present_flag
        skip_bits(gb, 4); // video_format u(3), video_full_range_flag u(1)

        if (get_bits1(gb)) // colour_description_present_flag
            // colour_primaries         u(8)
            // transfer_characteristics u(8)
            // matrix_coeffs            u(8)
            skip_bits(gb, 24);
    }

    if (get_bits1(gb)) {   // chroma_loc_info_present_flag
        get_ue_golomb(gb); // chroma_sample_loc_type_top_field
        get_ue_golomb(gb); // chroma_sample_loc_type_bottom_field
    }

    // neutral_chroma_indication_flag u(1)
    // field_seq_flag                 u(1)
    // frame_field_info_present_flag  u(1)
    skip_bits(gb, 3);

    if (get_bits1(gb)) {   // default_display_window_flag
        get_ue_golomb(gb); // def_disp_win_left_offset
        get_ue_golomb(gb); // def_disp_win_right_offset
        get_ue_golomb(gb); // def_disp_win_top_offset
        get_ue_golomb(gb); // def_disp_win_bottom_offset
    }

    if (get_bits1(gb)) { // vui_timing_info_present_flag
        skip_timing_info(gb);

        if (get_bits1(gb)) // vui_hrd_parameters_present_flag
            skip_hrd_parameters(gb, 1, max_sub_layers_minus1);
    }

    if (get_bits1(gb)) { // bitstream_restriction_flag
        // tiles_fixed_structure_flag              u(1)
        // motion_vectors_over_pic_boundaries_flag u(1)
        // restricted_ref_pic_lists_flag           u(1)
        skip_bits(gb, 3);

        min_spatial_segmentation_idc = get_ue_golomb(gb);

        /*
         * unsigned int(12) min_spatial_segmentation_idc;
         *
         * The min_spatial_segmentation_idc indication must indicate a level of
         * spatial segmentation equal to or less than the lowest level of
         * spatial segmentation indicated in all the parameter sets.
         */
        hvcc->min_spatial_segmentation_idc = FFMIN(hvcc->min_spatial_segmentation_idc,
                                                   min_spatial_segmentation_idc);

        get_ue_golomb(gb); // max_bytes_per_pic_denom
        get_ue_golomb(gb); // max_bits_per_min_cu_denom
        get_ue_golomb(gb); // log2_max_mv_length_horizontal
        get_ue_golomb(gb); // log2_max_mv_length_vertical
    }
}

static void skip_sub_layer_ordering_info(GetBitContext *gb)
{
    get_ue_golomb(gb); // max_dec_pic_buffering_minus1
    get_ue_golomb(gb); // max_num_reorder_pics
    get_ue_golomb(gb); // max_latency_increase_plus1
}

static int hvcc_parse_vps(uint8_t *vps_buf, int vps_size,
                          HEVCDecoderConfigurationRecord *hvcc)
{
    int vps_max_sub_layers_minus1;
    int ret, nal_size = 0;
    uint8_t nal_type, *nal_buf = NULL;
    GetBitContext gbc, *gb = &gbc;

    ret = nal_unit_extract_rbsp(vps_buf, vps_size, &nal_buf, &nal_size);
    if (ret < 0)
        return ret;

    ret = init_get_bits8(gb, nal_buf, nal_size);
    if (ret < 0)
        return ret;

    nal_unit_parse_header(gb, &nal_type);
    if (nal_type != NAL_VPS)
        return AVERROR_BUG;

    // vps_video_parameter_set_id u(4)
    // vps_reserved_three_2bits   u(2)
    // vps_max_layers_minus1      u(6)
    skip_bits(gb, 12);

    vps_max_sub_layers_minus1 = get_bits(gb, 3);

    /*
     * numTemporalLayers greater than 1 indicates that the stream to which this
     * configuration record applies is temporally scalable and the contained
     * number of temporal layers (also referred to as temporal sub-layer or
     * sub-layer in ISO/IEC 23008-2) is equal to numTemporalLayers. Value 1
     * indicates that the stream is not temporally scalable. Value 0 indicates
     * that it is unknown whether the stream is temporally scalable.
     */
    hvcc->numTemporalLayers = FFMAX(hvcc->numTemporalLayers,
                                    vps_max_sub_layers_minus1 + 1);

    skip_bits1(gb); // vps_temporal_id_nesting_flag

    skip_bits(gb, 16); // vps_reserved_0xffff_16bits

    hvcc_parse_ptl(gb, hvcc, vps_max_sub_layers_minus1);

    // nothing useful for hvcC past this point
    return 0;
}

static void skip_scaling_list_data(GetBitContext *gb)
{
    int i, j, k, num_coeffs;

    for (i = 0; i < 4; i++)
        for (j = 0; j < (i == 3 ? 2 : 6); j++)
            if (!get_bits1(gb))    // scaling_list_pred_mode_flag[i][j]
                get_ue_golomb(gb); // scaling_list_pred_matrix_id_delta[i][j]
            else {
                num_coeffs = FFMIN(64, 1 << (4 + (i << 1)));

                if (i > 1)
                    get_se_golomb(gb); // scaling_list_dc_coef_minus8[i-2][j]

                for (k = 0; k < num_coeffs; k++)
                    get_se_golomb(gb); // scaling_list_delta_coef
            }
}

static int parse_rps(GetBitContext *gb, int rps_idx, int num_rps,
                     int num_delta_pocs[MAX_SHORT_TERM_RPS_COUNT])
{
    int i;

    if (rps_idx && get_bits1(gb)) { // inter_ref_pic_set_prediction_flag
        // this should only happen for slice headers, and this isn't one
        if (rps_idx >= num_rps)
            return AVERROR_INVALIDDATA;

        skip_bits1   (gb); // delta_rps_sign
        get_ue_golomb(gb); // abs_delta_rps_minus1

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
        int num_negative_pics = get_ue_golomb(gb);
        int num_positive_pics = get_ue_golomb(gb);

        num_delta_pocs[rps_idx] = num_negative_pics + num_positive_pics;

        for (i = 0; i < num_negative_pics; i++) {
            get_ue_golomb(gb); // delta_poc_s0_minus1[rps_idx]
            skip_bits1   (gb); // used_by_curr_pic_s0_flag[rps_idx]
        }

        for (i = 0; i < num_positive_pics; i++) {
            get_ue_golomb(gb); // delta_poc_s1_minus1[rps_idx]
            skip_bits1   (gb); // used_by_curr_pic_s1_flag[rps_idx]
        }
    }

    return 0;
}

static int hvcc_parse_sps(uint8_t *sps_buf, int sps_size,
                          HEVCDecoderConfigurationRecord *hvcc)
{
    int sps_max_sub_layers_minus1, log2_max_pic_order_cnt_lsb_minus4;
    int num_short_term_ref_pic_sets, num_delta_pocs[MAX_SHORT_TERM_RPS_COUNT];
    int i, ret, nal_size = 0;
    uint8_t nal_type, *nal_buf = NULL;
    GetBitContext gbc, *gb = &gbc;

    ret = nal_unit_extract_rbsp(sps_buf, sps_size, &nal_buf, &nal_size);
    if (ret < 0)
        return ret;

    ret = init_get_bits8(gb, nal_buf, nal_size);
    if (ret < 0)
        return ret;

    nal_unit_parse_header(gb, &nal_type);
    if (nal_type != NAL_SPS)
        return AVERROR_BUG;

    skip_bits(gb, 4); // sps_video_parameter_set_id

    sps_max_sub_layers_minus1 = get_bits (gb, 3);

    /*
     * numTemporalLayers greater than 1 indicates that the stream to which this
     * configuration record applies is temporally scalable and the contained
     * number of temporal layers (also referred to as temporal sub-layer or
     * sub-layer in ISO/IEC 23008-2) is equal to numTemporalLayers. Value 1
     * indicates that the stream is not temporally scalable. Value 0 indicates
     * that it is unknown whether the stream is temporally scalable.
     */
    hvcc->numTemporalLayers = FFMAX(hvcc->numTemporalLayers,
                                    sps_max_sub_layers_minus1 + 1);

    hvcc->temporalIdNested = get_bits1(gb);

    hvcc_parse_ptl(gb, hvcc, sps_max_sub_layers_minus1);

    get_ue_golomb(gb); // sps_seq_parameter_set_id

    hvcc->chromaFormat = get_ue_golomb(gb);

    if (hvcc->chromaFormat == 3)
        skip_bits1(gb); // separate_colour_plane_flag

    get_ue_golomb(gb); // pic_width_in_luma_samples
    get_ue_golomb(gb); // pic_height_in_luma_samples

    if (get_bits1(gb)) {   // conformance_window_flag
        get_ue_golomb(gb); // conf_win_left_offset
        get_ue_golomb(gb); // conf_win_right_offset
        get_ue_golomb(gb); // conf_win_top_offset
        get_ue_golomb(gb); // conf_win_bottom_offset
    }

    hvcc->bitDepthLumaMinus8          = get_ue_golomb(gb);
    hvcc->bitDepthChromaMinus8        = get_ue_golomb(gb);
    log2_max_pic_order_cnt_lsb_minus4 = get_ue_golomb(gb);

    // sps_sub_layer_ordering_info_present_flag
    i = get_bits1(gb) ? 0 : sps_max_sub_layers_minus1;
    for (; i <= sps_max_sub_layers_minus1; i++)
        skip_sub_layer_ordering_info(gb);

    get_ue_golomb(gb); // log2_min_luma_coding_block_size_minus3
    get_ue_golomb(gb); // log2_diff_max_min_luma_coding_block_size
    get_ue_golomb(gb); // log2_min_transform_block_size_minus2
    get_ue_golomb(gb); // log2_diff_max_min_transform_block_size
    get_ue_golomb(gb); // max_transform_hierarchy_depth_inter
    get_ue_golomb(gb); // max_transform_hierarchy_depth_intra

    if (get_bits1(gb) && // scaling_list_enabled_flag
        get_bits1(gb))   // sps_scaling_list_data_present_flag
        skip_scaling_list_data(gb);

    skip_bits1(gb); // amp_enabled_flag
    skip_bits1(gb); // sample_adaptive_offset_enabled_flag

    if (get_bits1(gb)) {      // pcm_enabled_flag
        skip_bits    (gb, 4); // pcm_sample_bit_depth_luma_minus1
        skip_bits    (gb, 4); // pcm_sample_bit_depth_chroma_minus1
        get_ue_golomb(gb);    // log2_min_pcm_luma_coding_block_size_minus3
        get_ue_golomb(gb);    // log2_diff_max_min_pcm_luma_coding_block_size
        skip_bits1   (gb);    // pcm_loop_filter_disabled_flag
    }

    num_short_term_ref_pic_sets = get_ue_golomb(gb);
    if (num_short_term_ref_pic_sets > MAX_SHORT_TERM_RPS_COUNT)
        return AVERROR_INVALIDDATA;

    for (i = 0; i < num_short_term_ref_pic_sets; i++) {
        ret = parse_rps(gb, i, num_short_term_ref_pic_sets, num_delta_pocs);
        if (ret < 0)
            return ret;
    }

    if (get_bits1(gb)) {                          // long_term_ref_pics_present_flag
        for (i = 0; i < get_ue_golomb(gb); i++) { // num_long_term_ref_pics_sps
            int len = FFMIN(log2_max_pic_order_cnt_lsb_minus4 + 4, 16);
            skip_bits (gb, len); // lt_ref_pic_poc_lsb_sps[i]
            skip_bits1(gb);      // used_by_curr_pic_lt_sps_flag[i]
        }
    }

    skip_bits1(gb); // sps_temporal_mvp_enabled_flag
    skip_bits1(gb); // strong_intra_smoothing_enabled_flag

    if (get_bits1(gb)) // vui_parameters_present_flag
        hvcc_parse_vui(gb, hvcc, sps_max_sub_layers_minus1);

    // nothing useful for hvcC past this point
    return 0;
}

static int hvcc_parse_pps(uint8_t *pps_buf, int pps_size,
                          HEVCDecoderConfigurationRecord *hvcc)
{
    uint8_t tiles_enabled_flag, entropy_coding_sync_enabled_flag;
    uint8_t nal_type, *nal_buf = NULL;
    int ret, nal_size = 0;
    GetBitContext gbc, *gb = &gbc;

    ret = nal_unit_extract_rbsp(pps_buf, pps_size, &nal_buf, &nal_size);
    if (ret < 0)
        return ret;

    ret = init_get_bits8(gb, nal_buf, nal_size);
    if (ret < 0)
        return ret;

    nal_unit_parse_header(gb, &nal_type);
    if (nal_type != NAL_PPS)
        return AVERROR_BUG;

    get_ue_golomb(gb); // pps_pic_parameter_set_id
    get_ue_golomb(gb); // pps_seq_parameter_set_id

    // dependent_slice_segments_enabled_flag u(1)
    // output_flag_present_flag              u(1)
    // num_extra_slice_header_bits           u(3)
    // sign_data_hiding_enabled_flag         u(1)
    // cabac_init_present_flag               u(1)
    skip_bits(gb, 7);

    get_ue_golomb(gb); // num_ref_idx_l0_default_active_minus1
    get_ue_golomb(gb); // num_ref_idx_l1_default_active_minus1

    get_se_golomb(gb); // init_qp_minus26

    // constrained_intra_pred_flag u(1)
    // transform_skip_enabled_flag u(1)
    skip_bits(gb, 2);

    if (get_bits1(gb))     // cu_qp_delta_enabled_flag
        get_ue_golomb(gb); // diff_cu_qp_delta_depth

    get_se_golomb(gb); // pps_cb_qp_offset
    get_se_golomb(gb); // pps_cr_qp_offset

    // weighted_pred_flag               u(1)
    // weighted_bipred_flag             u(1)
    // transquant_bypass_enabled_flag   u(1)
    skip_bits(gb, 3);

    tiles_enabled_flag               = get_bits1(gb);
    entropy_coding_sync_enabled_flag = get_bits1(gb);

    if (entropy_coding_sync_enabled_flag && tiles_enabled_flag)
        hvcc->parallelismType = 0; // mixed-type parallel decoding
    else if (entropy_coding_sync_enabled_flag)
        hvcc->parallelismType = 3; // wavefront-based parallel decoding
    else if (tiles_enabled_flag)
        hvcc->parallelismType = 2; // tile-based parallel decoding
    else
        hvcc->parallelismType = 1; // slice-based parallel decoding

    // nothing useful for hvcC past this point
    return 0;
}

int ff_isom_write_hvcc(AVIOContext *pb, const uint8_t *data, int len)
{
    HEVC_DEBUG_LOG("data: %p, len: %d\n", data, len);
    if (len > 6) {
        /* check for H.265 start code */
        if (AV_RB32(data) == 0x00000001 || AV_RB24(data) == 0x000001) {
            uint8_t *end, *start;
            HEVCDecoderConfigurationRecord hvcc;
            uint32_t vps_size = 0, sps_size = 0, pps_size = 0;
            uint8_t *buf = NULL, *vps = NULL, *sps = NULL, *pps = NULL;

            int ret = ff_avc_parse_nal_units_buf(data, &buf, &len);
            if (ret < 0)
                return ret;

            start = buf;
            end   = buf + len;

            hvcc_init(&hvcc);

            /* look for vps, sps and pps */
            while (end - buf > 4) {
                int ret;
                uint8_t nal_type;
                GetBitContext gbc, *gb = &gbc;
                uint32_t size = FFMIN(AV_RB32(buf), end - buf - 4);
                buf += 4;

                ret = init_get_bits8(gb, buf, size);
                if (ret < 0)
                    return ret;

                nal_unit_parse_header(gb, &nal_type);

                switch (nal_type) {
                case NAL_VPS:
                    dump_nal_full(buf, size, "vps");
                    dump_nal_rbsp(buf, size, "VPS");
                    vps      = buf;
                    vps_size = size;
                    ret      = hvcc_parse_vps(buf, size, &hvcc);
                    if (ret < 0)
                        return ret;
                    break;
                case NAL_SPS:
                    dump_nal_full(buf, size, "sps");
                    dump_nal_rbsp(buf, size, "SPS");
                    sps      = buf;
                    sps_size = size;
                    ret      = hvcc_parse_sps(buf, size, &hvcc);
                    if (ret < 0)
                        return ret;
                    break;
                case NAL_PPS:
                    dump_nal_full(buf, size, "pps");
                    dump_nal_rbsp(buf, size, "PPS");
                    pps      = buf;
                    pps_size = size;
                    ret      = hvcc_parse_pps(buf, size, &hvcc);
                    if (ret < 0)
                        return ret;
                    break;
                default:
                    HEVC_DEBUG_LOG("NAL with type: %d and size: %d\n", nal_type, size);
                    break;
                }

                buf += size;
            }

            if (!vps || vps_size > UINT16_MAX ||
                !sps || sps_size > UINT16_MAX ||
                !pps || pps_size > UINT16_MAX) {
                return AVERROR_INVALIDDATA;
            }

            hvcc_finalize(&hvcc);
            dump_hvcc    (&hvcc);

            // unsigned int(8) configurationVersion = 1;
            avio_w8(pb, hvcc.configurationVersion);

            // unsigned int(2) general_profile_space;
            // unsigned int(1) general_tier_flag;
            // unsigned int(5) general_profile_idc;
            avio_w8(pb, hvcc.general_profile_space << 6 |
                        hvcc.general_tier_flag     << 5 |
                        hvcc.general_profile_idc);

            // unsigned int(32) general_profile_compatibility_flags;
            avio_wb32(pb, hvcc.general_profile_compatibility_flags);

            // unsigned int(48) general_constraint_indicator_flags;
            avio_wb32(pb, hvcc.general_constraint_indicator_flags >> 16);
            avio_wb16(pb, hvcc.general_constraint_indicator_flags);

            // unsigned int(8) general_level_idc;
            avio_w8(pb, hvcc.general_level_idc);

            // bit(4) reserved = ‘1111’b;
            // unsigned int(12) min_spatial_segmentation_idc;
            avio_wb16(pb, hvcc.min_spatial_segmentation_idc | 0xf000);

            // bit(6) reserved = ‘111111’b;
            // unsigned int(2) parallelismType;
            avio_w8(pb, hvcc.parallelismType | 0xfc);

            // bit(6) reserved = ‘111111’b;
            // unsigned int(2) chromaFormat;
            avio_w8(pb, hvcc.chromaFormat | 0xfc);

            // bit(5) reserved = ‘11111’b;
            // unsigned int(3) bitDepthLumaMinus8;
            avio_w8(pb, hvcc.bitDepthLumaMinus8 | 0xf8);

            // bit(5) reserved = ‘11111’b;
            // unsigned int(3) bitDepthChromaMinus8;
            avio_w8(pb, hvcc.bitDepthChromaMinus8 | 0xf8);

            // bit(16) avgFrameRate;
            avio_wb16(pb, hvcc.avgFrameRate);

            // bit(2) constantFrameRate;
            // bit(3) numTemporalLayers;
            // bit(1) temporalIdNested;
            // unsigned int(2) lengthSizeMinusOne;
            avio_w8(pb, hvcc.constantFrameRate << 6 |
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
