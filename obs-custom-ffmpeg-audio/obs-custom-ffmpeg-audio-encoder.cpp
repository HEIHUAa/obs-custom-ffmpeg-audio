#include "obs-custom-ffmpeg-audio-encoder.hpp"
#include <util/dstr.h>
#include <cstring>
#include <cstdlib>

#define do_log(level, format, ...) \
	blog(level, "[Custom FFmpeg Audio: '%s'] " format, \
	     obs_encoder_get_name(enc->encoder), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

/* ── 预定义编码器列表 ──────────────────────────────────────── */

static const codec_entry aac_codecs[] = {
	{"AAC (FFmpeg)",                "aac",           false},
	{"AAC (libfdk)",                "libfdk_aac",    false},
	{nullptr, nullptr, false}
};

static const codec_entry opus_codecs[] = {
	{"Opus (libopus)",              "libopus",       false},
	{nullptr, nullptr, false}
};

static const codec_entry flac_codecs[] = {
	{"FLAC (lossless)",             "flac",          true},
	{nullptr, nullptr, false}
};

static const codec_entry alac_codecs[] = {
	{"ALAC (lossless)",             "alac",          true},
	{nullptr, nullptr, false}
};

static const codec_entry mp3_codecs[] = {
	{"MP3 (libmp3lame)",            "libmp3lame",    false},
	{nullptr, nullptr, false}
};

static const codec_entry ac3_codecs[] = {
	{"AC3",                         "ac3",           false},
	{"E-AC3",                       "eac3",          false},
	{nullptr, nullptr, false}
};

static const codec_entry vorbis_codecs[] = {
	{"Vorbis (libvorbis)",          "libvorbis",     false},
	{nullptr, nullptr, false}
};

static const codec_entry pcm_codecs[] = {
	{"PCM 16-bit",                  "pcm_s16le",     true},
	{"PCM 24-bit",                  "pcm_s24le",     true},
	{"PCM 32-bit float",            "pcm_f32le",     true},
	{"PCM unsigned 8-bit",          "pcm_u8",        true},
	{"PCM A-law",                   "pcm_alaw",      true},
	{"PCM mu-law",                  "pcm_mulaw",     true},
	{"ADPCM MS",                    "adpcm_ms",      true},
	{"G.722",                       "g722",          false},
	{nullptr, nullptr, false}
};

/* ── 编码器家族定义 ──────────────────────────────────────── */

const encoder_family families[] = {
	{"custom_ffmpeg_audio_aac",    "aac",       "Custom FFmpeg Audio (AAC)",    aac_codecs,   "aac"},
	{"custom_ffmpeg_audio_opus",   "opus",      "Custom FFmpeg Audio (Opus)",   opus_codecs,  "libopus"},
	{"custom_ffmpeg_audio_flac",   "flac",      "Custom FFmpeg Audio (FLAC)",   flac_codecs,  "flac"},
	{"custom_ffmpeg_audio_alac",   "alac",      "Custom FFmpeg Audio (ALAC)",   alac_codecs,  "alac"},
	{"custom_ffmpeg_audio_mp3",    "mp3",       "Custom FFmpeg Audio (MP3)",    mp3_codecs,   "libmp3lame"},
	{"custom_ffmpeg_audio_ac3",    "ac3",       "Custom FFmpeg Audio (AC3)",    ac3_codecs,   "ac3"},
	{"custom_ffmpeg_audio_vorbis", "vorbis",    "Custom FFmpeg Audio (Vorbis)", vorbis_codecs,"libvorbis"},
	{"custom_ffmpeg_audio_pcm",    "pcm_s16le", "Custom FFmpeg Audio (PCM)",    pcm_codecs,   "pcm_s16le"},
	{nullptr, nullptr, nullptr, nullptr, nullptr},
};

static bool is_lossless_codec(const char *codec_id)
{
	if (!codec_id)
		return false;
	for (const encoder_family *f = families; f->id; f++) {
		for (const codec_entry *e = f->entries; e->name; e++) {
			if (strcmp(e->codec_id, codec_id) == 0)
				return e->lossless;
		}
	}
	return false;
}

/* ── OBS音频格式与FFmpeg采样格式转换 ───────────────────── */
static inline enum audio_format convert_ffmpeg_sample_format(enum AVSampleFormat fmt)
{
	switch (fmt) {
	case AV_SAMPLE_FMT_U8:
		return AUDIO_FORMAT_U8BIT;
	case AV_SAMPLE_FMT_S16:
		return AUDIO_FORMAT_16BIT;
	case AV_SAMPLE_FMT_S32:
		return AUDIO_FORMAT_32BIT;
	case AV_SAMPLE_FMT_FLT:
		return AUDIO_FORMAT_FLOAT;
	case AV_SAMPLE_FMT_U8P:
		return AUDIO_FORMAT_U8BIT_PLANAR;
	case AV_SAMPLE_FMT_S16P:
		return AUDIO_FORMAT_16BIT_PLANAR;
	case AV_SAMPLE_FMT_S32P:
		return AUDIO_FORMAT_32BIT_PLANAR;
	case AV_SAMPLE_FMT_FLTP:
		return AUDIO_FORMAT_FLOAT_PLANAR;
	case AV_SAMPLE_FMT_NONE:
	default:
		return AUDIO_FORMAT_FLOAT_PLANAR;
	}
}

static inline std::string ffmpeg_error_str(int errnum)
{
	char buf[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(errnum, buf, sizeof(buf));
	return std::string(buf);
}

/* ── 编码器回调 ──────────────────────────────────────────── */

static const char *enc_get_name(void *type_data)
{
	const encoder_family *family = (const encoder_family *)type_data;
	if (family && family->display_name)
		return family->display_name;
	return "Custom FFmpeg Audio";
}

static void enc_defaults2(obs_data_t *settings, void *type_data)
{
	const encoder_family *family = (const encoder_family *)type_data;
	blog(LOG_INFO, "[Custom FFmpeg Audio] enc_defaults called for %s",
	     family ? family->id : "unknown");

	obs_data_set_default_string(settings, "codec",
		family ? family->default_codec_id : "aac");
	obs_data_set_default_int(settings, "bitrate", 128);
	obs_data_set_default_string(settings, "sample_rate", "auto");
	obs_data_set_default_int(settings, "quality", 3);
	obs_data_set_default_bool(settings, "use_quality", false);
	obs_data_set_default_string(settings, "custom_options", "");
	obs_data_set_default_int(settings, "strict_compliance", -2);
}

static inline int64_t rescale_ts(int64_t val, AVCodecContext *ctx, AVRational new_base)
{
	return av_rescale_q_rnd(val, ctx->time_base, new_base,
			    (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
}

static void enc_destroy(void *data)
{
	auto *enc = static_cast<custom_ffmpeg_audio_encoder *>(data);

	if (enc->context) {
		avcodec_send_frame(enc->context, nullptr);
		while (true) {
			AVPacket drain_pkt = {0};
			int ret = avcodec_receive_packet(enc->context, &drain_pkt);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				av_packet_unref(&drain_pkt);
				break;
			}
			av_packet_unref(&drain_pkt);
		}
	}

	if (enc->sample_buffer[0])
		av_freep(&enc->sample_buffer[0]);

	if (enc->context)
		avcodec_free_context(&enc->context);

	if (enc->aframe)
		av_frame_free(&enc->aframe);

	delete enc;
}

static bool initialize_codec(custom_ffmpeg_audio_encoder *enc)
{
	enc->aframe = av_frame_alloc();
	if (!enc->aframe) {
		warn("Failed to allocate audio frame");
		return false;
	}

	AVDictionary *opts = nullptr;

	if (!enc->custom_options.empty()) {
		int ret = av_dict_parse_string(&opts, enc->custom_options.c_str(), "=", " ", 0);
		if (ret < 0) {
			warn("Failed to parse custom options: %s", ffmpeg_error_str(ret).c_str());
		}
	}

	enc->context->strict_std_compliance =
		(int)obs_data_get_int(obs_encoder_get_settings(enc->encoder), "strict_compliance");

	int ret = avcodec_open2(enc->context, enc->codec, &opts);

	if (opts) {
		AVDictionaryEntry *entry = nullptr;
		while ((entry = av_dict_get(opts, "", entry, AV_DICT_IGNORE_SUFFIX)))
			warn("Unused option: %s=%s", entry->key, entry->value);
		av_dict_free(&opts);
	}

	if (ret < 0) {
		struct dstr error_msg = {0};
		std::string err_str = ffmpeg_error_str(ret);
		dstr_printf(&error_msg, "Failed to open codec '%s': %s",
			    enc->codec_name.c_str(), err_str.c_str());
		obs_encoder_set_last_error(enc->encoder, error_msg.array);
		dstr_free(&error_msg);
		warn("Failed to open codec '%s': %s", enc->codec_name.c_str(), err_str.c_str());
		return false;
	}

	enc->aframe->format = enc->context->sample_fmt;
	enc->aframe->ch_layout = enc->context->ch_layout;
	enc->aframe->sample_rate = enc->context->sample_rate;

	enc->frame_size = enc->context->frame_size;
	if (!enc->frame_size)
		enc->frame_size = 1024;

	enc->frame_size_bytes =
		enc->frame_size * (int)get_audio_bytes_per_channel(
			convert_ffmpeg_sample_format(enc->context->sample_fmt));

	int channels = enc->context->ch_layout.nb_channels;
	ret = av_samples_alloc(enc->sample_buffer, nullptr, channels,
			       enc->frame_size, enc->context->sample_fmt, 0);
	if (ret < 0) {
		warn("Failed to create audio sample buffer: %s", ffmpeg_error_str(ret).c_str());
		return false;
	}

	enc->sample_buffer_size = (size_t)ret;

	return true;
}

static void init_sizes(custom_ffmpeg_audio_encoder *enc, audio_t *audio)
{
	const struct audio_output_info *aoi = audio_output_get_info(audio);
	enum audio_format fmt = convert_ffmpeg_sample_format(enc->context->sample_fmt);
	enc->audio_planes = get_audio_planes(fmt, aoi->speakers);
}

static void *enc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	auto *enc = new custom_ffmpeg_audio_encoder;
	enc->encoder = encoder;

	const encoder_family *family = (const encoder_family *)
		obs_encoder_get_type_data(encoder);
	if (!family)
		family = &families[0];
	enc->family = family;

	blog(LOG_INFO, "[Custom FFmpeg Audio] enc_create, selected family: %s, default codec: %s",
	     enc->family->id, enc->family->default_codec_id);

	const char *codec_id = obs_data_get_string(settings, "codec");
	if (!codec_id || !*codec_id) {
		codec_id = enc->family->default_codec_id;
		obs_data_set_string(settings, "codec", codec_id);
		blog(LOG_INFO, "[Custom FFmpeg Audio] codec was empty, using default: %s", codec_id);
	}
	enc->bitrate = (int)obs_data_get_int(settings, "bitrate");
	enc->use_quality = obs_data_get_bool(settings, "use_quality");
	enc->quality = (int)obs_data_get_int(settings, "quality");
	{
		config_t *cfg = open_encoder_config();
		if (cfg) {
			const char *saved = config_get_string(cfg, enc->family->id, "custom_options");
			enc->custom_options = (saved && *saved) ? saved
				: obs_data_get_string(settings, "custom_options");
			config_close(cfg);
		} else {
			enc->custom_options = obs_data_get_string(settings, "custom_options");
		}
	}
	enc->codec_name = codec_id;

	audio_t *audio = nullptr;
	const struct audio_output_info *aoi = nullptr;
	int channels = 0;
	const char *sample_rate_str = nullptr;
	const enum AVSampleFormat *sample_fmts = nullptr;
	const int *supported_samplerates = nullptr;

	bool lossless = is_lossless_codec(codec_id);

	info("---------------------------------");
	info("initializing encoder...");

	enc->codec = avcodec_find_encoder_by_name(codec_id);
	if (!enc->codec) {
		warn("Couldn't find encoder '%s'", codec_id);
		goto fail;
	}

	enc->context = avcodec_alloc_context3(enc->codec);
	if (!enc->context) {
		warn("Failed to create codec context");
		goto fail;
	}

	audio = obs_encoder_audio(encoder);
	aoi = audio_output_get_info(audio);

	channels = (int)audio_output_get_channels(audio);
	av_channel_layout_default(&enc->context->ch_layout, channels);

	if (aoi->speakers == SPEAKERS_4POINT1)
		av_channel_layout_from_mask(&enc->context->ch_layout,
					    AV_CH_LAYOUT_4POINT1);
	if (aoi->speakers == SPEAKERS_2POINT1)
		av_channel_layout_from_mask(&enc->context->ch_layout,
					    AV_CH_LAYOUT_SURROUND);
	if (aoi->speakers == SPEAKERS_7POINT1 &&
	    strcmp(codec_id, "alac") == 0)
		av_channel_layout_from_mask(&enc->context->ch_layout,
					    AV_CH_LAYOUT_7POINT1_WIDE_BACK);

	sample_rate_str = obs_data_get_string(settings, "sample_rate");
	int sr;
	if (strcmp(sample_rate_str, "auto") == 0)
		sr = (int)audio_output_get_sample_rate(audio);
	else
		sr = atoi(sample_rate_str);

	enc->context->sample_rate = sr;

	sample_fmts = nullptr;
	supported_samplerates = nullptr;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(61, 13, 100)
	sample_fmts = enc->codec->sample_fmts;
	supported_samplerates = enc->codec->supported_samplerates;
#else
	avcodec_get_supported_config(enc->context, enc->codec, AV_CODEC_CONFIG_SAMPLE_FORMAT,
				     0, (const void **)&sample_fmts, nullptr);
	avcodec_get_supported_config(enc->context, enc->codec, AV_CODEC_CONFIG_SAMPLE_RATE,
				     0, (const void **)&supported_samplerates, nullptr);
#endif

	if (sample_fmts) {
		const enum AVSampleFormat *fmt = sample_fmts;
		enum AVSampleFormat preferred = AV_SAMPLE_FMT_FLTP;
		while (*fmt != AV_SAMPLE_FMT_NONE) {
			if (*fmt == preferred) {
				enc->context->sample_fmt = *fmt;
				break;
			}
			fmt++;
		}
		if (enc->context->sample_fmt == AV_SAMPLE_FMT_NONE)
			enc->context->sample_fmt = sample_fmts[0];
	} else {
		enc->context->sample_fmt = AV_SAMPLE_FMT_FLTP;
	}

	if (supported_samplerates) {
		const int *rate = supported_samplerates;
		int cur_rate = enc->context->sample_rate;
		int closest = 0;
		while (*rate) {
			int dist = abs(cur_rate - *rate);
			int closest_dist = abs(cur_rate - closest);
			if (dist < closest_dist)
				closest = *rate;
			rate++;
		}
		if (closest)
			enc->context->sample_rate = closest;
	}

	if (lossless || enc->use_quality) {
		enc->context->bit_rate = 0;
		if (enc->use_quality) {
			av_opt_set_double(enc->context->priv_data, "quality",
					  (double)enc->quality / 10.0, 0);
		}
	} else {
		enc->context->bit_rate = enc->bitrate * 1000;
	}

	enc->context->flags = AV_CODEC_FLAG_GLOBAL_HEADER;

	char ch_layout_desc[256];
	av_channel_layout_describe(&enc->context->ch_layout, ch_layout_desc, 256);
	info("codec: %s, bitrate: %" PRId64 " kbps, channels: %d, layout: %s, sample_rate: %d",
	     codec_id, enc->context->bit_rate / 1000,
	     enc->context->ch_layout.nb_channels, ch_layout_desc,
	     enc->context->sample_rate);

	init_sizes(enc, audio);

	if (!initialize_codec(enc))
		goto fail;

	return enc;

fail:
	enc_destroy(enc);
	return nullptr;
}

static bool do_encode(custom_ffmpeg_audio_encoder *enc,
		      struct encoder_packet *packet, bool *received_packet)
{
	AVRational time_base = {1, enc->context->sample_rate};

	enc->aframe->nb_samples = enc->frame_size;
	enc->aframe->pts = av_rescale_q(enc->total_samples,
					AVRational{1, enc->context->sample_rate},
					enc->context->time_base);
	enc->aframe->ch_layout = enc->context->ch_layout;

	int channels = enc->context->ch_layout.nb_channels;
	int ret = avcodec_fill_audio_frame(enc->aframe, channels,
					   enc->context->sample_fmt,
					   enc->sample_buffer[0],
					   enc->frame_size_bytes * channels, 1);
	if (ret < 0) {
		warn("avcodec_fill_audio_frame failed: %s", ffmpeg_error_str(ret).c_str());
		return false;
	}

	enc->total_samples += enc->frame_size;

	ret = avcodec_send_frame(enc->context, enc->aframe);
	while (ret == AVERROR(EAGAIN)) {
		AVPacket drain_pkt = {0};
		ret = avcodec_receive_packet(enc->context, &drain_pkt);
		if (ret == 0) {
			av_packet_unref(&drain_pkt);
			ret = avcodec_send_frame(enc->context, enc->aframe);
		} else {
			break;
		}
	}

	if (ret < 0 && ret != AVERROR_EOF) {
		warn("avcodec_send_frame failed: %s", ffmpeg_error_str(ret).c_str());
		return false;
	}

	*received_packet = false;

	while (true) {
		AVPacket avpacket = {0};
		ret = avcodec_receive_packet(enc->context, &avpacket);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			av_packet_unref(&avpacket);
			break;
		}
		if (ret < 0) {
			warn("avcodec_receive_packet failed: %s", ffmpeg_error_str(ret).c_str());
			av_packet_unref(&avpacket);
			return false;
		}

		if (!*received_packet) {
			enc->packet_data.resize(0);
			enc->packet_data.insert(enc->packet_data.end(),
						avpacket.data, avpacket.data + avpacket.size);

			packet->pts = rescale_ts(avpacket.pts, enc->context, time_base);
			packet->dts = rescale_ts(avpacket.dts, enc->context, time_base);
			packet->data = enc->packet_data.data();
			packet->size = avpacket.size;
			packet->type = OBS_ENCODER_AUDIO;
			packet->keyframe = true;
			packet->timebase_num = 1;
			packet->timebase_den = (int32_t)enc->context->sample_rate;

			*received_packet = true;
		}

		av_packet_unref(&avpacket);
	}

	return true;
}

static bool enc_encode(void *data, struct encoder_frame *frame,
		       struct encoder_packet *packet, bool *received_packet)
{
	auto *enc = static_cast<custom_ffmpeg_audio_encoder *>(data);

	int channels = enc->context->ch_layout.nb_channels;
	size_t plane_size = (size_t)enc->frame_size_bytes * channels / enc->audio_planes;

	for (size_t i = 0; i < enc->audio_planes; i++)
		memcpy(enc->sample_buffer[i], frame->data[i], plane_size);

	return do_encode(enc, packet, received_packet);
}

static size_t enc_frame_size(void *data)
{
	auto *enc = static_cast<custom_ffmpeg_audio_encoder *>(data);
	return (size_t)enc->frame_size;
}

static bool enc_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	auto *enc = static_cast<custom_ffmpeg_audio_encoder *>(data);
	*extra_data = enc->context->extradata;
	*size = enc->context->extradata_size;
	return true;
}

static void enc_audio_info(void *data, struct audio_convert_info *info)
{
	auto *enc = static_cast<custom_ffmpeg_audio_encoder *>(data);
	int channels = enc->context->ch_layout.nb_channels;
	info->format = convert_ffmpeg_sample_format(enc->context->sample_fmt);
	info->samples_per_sec = (uint32_t)enc->context->sample_rate;
	if (channels != 7 && channels <= 8)
		info->speakers = (enum speaker_layout)channels;
	else
		info->speakers = SPEAKERS_UNKNOWN;
}

static uint32_t enc_initial_padding(void *data)
{
	auto *enc = static_cast<custom_ffmpeg_audio_encoder *>(data);
	return enc->context->initial_padding;
}

/* ── 属性面板 ────────────────────────────────────────────── */

static bool codec_modified(obs_properties_t *props, obs_property_t *prop,
			   obs_data_t *settings)
{
	UNUSED_PARAMETER(prop);

	const char *codec_id = obs_data_get_string(settings, "codec");
	bool lossless = is_lossless_codec(codec_id);
	bool use_quality = obs_data_get_bool(settings, "use_quality");

	obs_property_t *p_bitrate = obs_properties_get(props, "bitrate");
	obs_property_t *p_quality = obs_properties_get(props, "quality");
	obs_property_t *p_use_quality = obs_properties_get(props, "use_quality");

	obs_property_set_visible(p_bitrate, !lossless && !use_quality);
	obs_property_set_visible(p_quality, use_quality);
	obs_property_set_visible(p_use_quality, !lossless);

	return true;
}

static bool quality_mode_modified(obs_properties_t *props, obs_property_t *prop,
				  obs_data_t *settings)
{
	UNUSED_PARAMETER(prop);
	return codec_modified(props, prop, settings);
}

static obs_properties_t *enc_properties2(void *data, void *type_data)
{
	UNUSED_PARAMETER(data);

	const encoder_family *family = (const encoder_family *)type_data;
	if (!family)
		family = &families[0];
	blog(LOG_INFO, "[Custom FFmpeg Audio] enc_properties2 using family: %s", family->id);

	obs_properties_t *props = obs_properties_create();
	obs_property_t *prop;

	prop = obs_properties_add_list(props, "codec",
		obs_module_text("Codec"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	for (const codec_entry *e = family->entries; e->name; e++)
		obs_property_list_add_string(prop, e->name, e->codec_id);

	obs_property_set_modified_callback(prop, codec_modified);

	prop = obs_properties_add_int(props, "bitrate",
		obs_module_text("Bitrate"), 16, 1024, 32);
	obs_property_int_set_suffix(prop, " kbps");

	prop = obs_properties_add_bool(props, "use_quality",
		obs_module_text("UseQuality"));
	obs_property_set_modified_callback(prop, quality_mode_modified);
	obs_property_set_long_description(prop,
		obs_module_text("UseQuality.Tooltip"));

	prop = obs_properties_add_int_slider(props, "quality",
		obs_module_text("Quality"), 0, 10, 1);
	obs_property_set_long_description(prop,
		obs_module_text("Quality.Tooltip"));

	prop = obs_properties_add_list(props, "sample_rate",
		obs_module_text("SampleRate"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(prop, obs_module_text("SampleRate.Auto"), "auto");
	obs_property_list_add_string(prop, "44100", "44100");
	obs_property_list_add_string(prop, "48000", "48000");
	obs_property_list_add_string(prop, "96000", "96000");
	obs_property_list_add_string(prop, "192000", "192000");
	obs_property_list_add_string(prop, "22050", "22050");
	obs_property_list_add_string(prop, "32000", "32000");

	prop = obs_properties_add_text(props, "custom_options",
		obs_module_text("CustomOptions"), OBS_TEXT_MULTILINE);
	obs_property_set_long_description(prop,
		obs_module_text("CustomOptions.Tooltip"));

	prop = obs_properties_add_int_slider(props, "strict_compliance",
		obs_module_text("StrictCompliance"), -2, 2, 1);
	obs_property_set_long_description(prop,
		obs_module_text("StrictCompliance.Tooltip"));

	return props;
}

static bool enc_update(void *data, obs_data_t *settings)
{
	auto *enc = static_cast<custom_ffmpeg_audio_encoder *>(data);
	const char *codec_id = obs_data_get_string(settings, "codec");
	bool lossless = is_lossless_codec(codec_id);
	bool use_quality = obs_data_get_bool(settings, "use_quality");

	if (!use_quality && !lossless) {
		enc->bitrate = (int)obs_data_get_int(settings, "bitrate");
		if (enc->context) {
			enc->context->bit_rate = enc->bitrate * 1000;
		}
	}

	enc->use_quality = use_quality;
	enc->quality = (int)obs_data_get_int(settings, "quality");
	enc->custom_options = obs_data_get_string(settings, "custom_options");

	return true;
}

/* ── 注册所有编码器家族 ─────────────────────────────────── */

void register_custom_ffmpeg_audio_encoders(void)
{
	for (const encoder_family *f = families; f->id; f++) {
		struct obs_encoder_info info = {};
		info.id = f->id;
		info.type = OBS_ENCODER_AUDIO;
		info.codec = f->codec;
		info.get_name = enc_get_name;
		info.create = enc_create;
		info.destroy = enc_destroy;
		info.encode = enc_encode;
		info.get_frame_size = enc_frame_size;
		info.get_defaults2 = enc_defaults2;
		info.get_properties2 = enc_properties2;
		info.update = enc_update;
		info.get_extra_data = enc_extra_data;
		info.get_audio_info = enc_audio_info;
		info.get_priming_samples = enc_initial_padding;
		info.type_data = (void *)f;
		obs_register_encoder_s(&info, sizeof(info));

		blog(LOG_INFO, "[Custom FFmpeg Audio] registered encoder: %s (codec: %s)",
		     f->id, f->codec);
	}
}

static obs_module_t *g_config_module = nullptr;

void set_encoder_config_module(obs_module_t *mod)
{
	g_config_module = mod;
}

config_t *open_encoder_config(void)
{
	char *path = obs_module_get_config_path(g_config_module, "config.ini");
	if (!path) return nullptr;
	config_t *config = nullptr;
	int ret = config_open(&config, path, CONFIG_OPEN_ALWAYS);
	bfree(path);
	if (ret != CONFIG_SUCCESS) return nullptr;
	return config;
}