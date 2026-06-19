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
#include <obs-frontend-api.h>
#include <QAction>
#include <QWidget>

#include "obs-custom-ffmpeg-audio-encoder.hpp"
#include "obs-custom-ffmpeg-audio-config-dialog.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-custom-ffmpeg-audio", "en-US");

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Custom FFmpeg Audio Encoder - supports any FFmpeg audio codec with customizable options";
}

bool obs_module_load(void)
{
	set_encoder_config_module(obs_current_module());
	register_custom_ffmpeg_audio_encoders();

	obs_frontend_push_ui_translation(obs_module_get_string);

	QWidget *parent = static_cast<QWidget *>(obs_frontend_get_main_window());
	QAction *action = (QAction *)obs_frontend_add_tools_menu_qaction(
		obs_module_text("CustomFFmpegAudio.Settings"));
	QObject::connect(action, &QAction::triggered, [parent]() {
		CustomFFmpegAudioConfigDialog dialog(parent);
		dialog.exec();
	});

	obs_frontend_pop_ui_translation();

	return true;
}