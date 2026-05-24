#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-custom-ffmpeg-audio", "en-US");

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Custom FFmpeg Audio Encoder - supports any FFmpeg audio codec with customizable options";
}

extern struct obs_encoder_info custom_ffmpeg_audio_encoder_info;

bool obs_module_load(void)
{
	obs_register_encoder(&custom_ffmpeg_audio_encoder_info);
	return true;
}