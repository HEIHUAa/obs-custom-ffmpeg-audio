#pragma once

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
};

struct encoder_family {
	const char *id;
	const char *codec;
	const char *display_name;
	const codec_entry *entries;
	const char *default_codec_id;
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
	size_t audio_size;
	int frame_size;
	int frame_size_bytes;

	int64_t total_samples;

	std::string codec_name;
	std::string custom_options;
	std::string sample_rate_str;

	int bitrate;

	bool use_quality;
	int quality;
};

void register_custom_ffmpeg_audio_encoders(void);

void set_encoder_config_module(obs_module_t *mod);
config_t *open_encoder_config(void);