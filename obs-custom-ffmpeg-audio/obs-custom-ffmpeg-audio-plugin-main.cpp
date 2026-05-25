#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-custom-ffmpeg-audio", "en-US");

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Custom FFmpeg Audio Encoder - supports any FFmpeg audio codec with customizable options";
}

extern void register_custom_ffmpeg_audio_encoders(void);

bool obs_module_load(void)
{
	register_custom_ffmpeg_audio_encoders();
	return true;
}