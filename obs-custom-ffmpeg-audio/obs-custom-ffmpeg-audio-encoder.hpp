#pragma once

/*
 * Copyright (C) 2026 HEIHUAa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <obs-module.h>
#include <util/config-file.h>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

struct codec_entry {
	const char *name;
	const char *codec_id;
	bool lossless;
	int sample_fmt;
};

struct encoder_family {
	const char *id;
	const char *codec;
	const char *display_name;
	const codec_entry *entries;
	const char *default_codec_id;
	bool supports_quality;
};

extern const encoder_family families[];

struct custom_ffmpeg_audio_encoder {
	obs_encoder_t *encoder;
	const encoder_family *family;

	const AVCodec *codec;
	AVCodecContext *context;
	AVFrame *aframe;

	uint8_t *sample_buffer[MAX_AV_PLANES];
	size_t sample_buffer_size;

	std::vector<uint8_t> packet_data;

	size_t audio_planes;
	int frame_size;
	int frame_size_bytes;

	int64_t total_samples;

	std::string codec_name;
	std::string custom_options;
	std::string sample_rate_str;

	int bitrate;

	bool use_quality;
	int quality;
	int forced_sample_fmt;
};

void register_custom_ffmpeg_audio_encoders(void);

void set_encoder_config_module(obs_module_t *mod);
config_t *open_encoder_config(void);