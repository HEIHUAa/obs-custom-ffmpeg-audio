#pragma once

#include <obs-module.h>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

struct custom_ffmpeg_audio_encoder {
	obs_encoder_t *encoder;

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

	int bitrate;
	int sample_rate;

	bool use_quality;
	int quality;
};

void register_custom_ffmpeg_audio_encoder();