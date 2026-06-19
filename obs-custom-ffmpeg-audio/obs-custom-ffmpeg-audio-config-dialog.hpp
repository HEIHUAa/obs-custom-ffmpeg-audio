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

#include <QDialog>

class QComboBox;
class QTextEdit;
class QCheckBox;
class QSlider;
class QLabel;
class QGroupBox;

class CustomFFmpegAudioConfigDialog : public QDialog {
	Q_OBJECT
public:
	CustomFFmpegAudioConfigDialog(QWidget *parent = nullptr);

private slots:
	void family_changed(int index);
	void save_current();
	void use_quality_toggled(bool checked);

private:
	QComboBox *family_combo;
	QComboBox *codec_combo;
	QComboBox *sample_rate_combo;
	QGroupBox *quality_group;
	QCheckBox *use_quality_check;
	QSlider *quality_slider;
	QLabel *quality_label;
	QTextEdit *custom_options_edit;
	int previous_index_ = -1;

	void load_family(int index);
	void save_family(int index);
};