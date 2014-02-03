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

#include "frame.h"

enum AVDownmixType {
    AV_DOWNMIX_TYPE_UNKNOWN,
    AV_DOWNMIX_TYPE_LORO,
    AV_DOWNMIX_TYPE_LTRT,
    AV_DOWNMIX_TYPE_DPLII,
    AV_DOWNMIX_TYPE_NB
};

typedef struct AVDownmixInfo {
    enum AVDownmixType preferred_downmix_type;
    double center_mix_level;
    double center_mix_level_ltrt;
    double surround_mix_level;
    double surround_mix_level_ltrt;
    double lfe_mix_level;
} AVDownmixInfo;

/**
 * Allocate an AVDownmixInfo structure and set its fields to default values.
 * The resulting struct can be freed using av_freep().
 *
 * @return An AVDownmixInfo filled with default values or NULL on failure.
 */
AVDownmixInfo *av_downmix_info_alloc(void);

/**
 * Allocate a complete AVFrameSideData and add it to the frame.
 *
 * @param frame The frame which side data is added to.
 *
 * @return The AVDownmixInfo structure to be filled by caller.
 */
AVDownmixInfo *av_downmix_info_create_side_data(AVFrame *frame);
