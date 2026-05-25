#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QAction>
#include <QWidget>

#include "obs-custom-ffmpeg-audio-config-dialog.hpp"

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

	obs_frontend_push_ui_translation(obs_current_module());

	QWidget *parent = static_cast<QWidget *>(obs_frontend_get_main_window());
	QAction *action = new QAction(obs_module_text("CustomFFmpegAudio.Settings"), nullptr);
	QObject::connect(action, &QAction::triggered, [parent]() {
		CustomFFmpegAudioConfigDialog dialog(parent);
		dialog.exec();
	});
	obs_frontend_add_tools_menu_qaction(
		obs_module_text("CustomFFmpegAudio.Settings"), action);

	obs_frontend_pop_ui_translation();

	return true;
}