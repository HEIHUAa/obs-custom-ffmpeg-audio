#include "obs-custom-ffmpeg-audio-config-dialog.hpp"
#include "obs-custom-ffmpeg-audio-encoder.hpp"

#include <obs-module.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>

CustomFFmpegAudioConfigDialog::CustomFFmpegAudioConfigDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(obs_module_text("CustomFFmpegAudio.ConfigTitle"));
	setMinimumSize(520, 380);

	auto *main_layout = new QVBoxLayout(this);

	auto *family_layout = new QHBoxLayout();
	family_layout->addWidget(new QLabel(obs_module_text("CustomFFmpegAudio.EncoderFamily")));
	family_combo = new QComboBox();
	for (const encoder_family *f = families; f->id; f++)
		family_combo->addItem(f->display_name, f->id);
	family_layout->addWidget(family_combo, 1);
	main_layout->addLayout(family_layout);

	auto *options_group = new QGroupBox(obs_module_text("CustomOptions"));
	auto *options_layout = new QVBoxLayout(options_group);
	custom_options_edit = new QTextEdit();
	custom_options_edit->setPlaceholderText(obs_module_text("CustomOptions.Tooltip"));
	options_layout->addWidget(custom_options_edit);
	main_layout->addWidget(options_group, 1);

	auto *button_layout = new QHBoxLayout();
	button_layout->addStretch();
	auto *save_btn = new QPushButton(obs_module_text("CustomFFmpegAudio.Save"));
	auto *cancel_btn = new QPushButton(obs_module_text("CustomFFmpegAudio.Cancel"));
	button_layout->addWidget(save_btn);
	button_layout->addWidget(cancel_btn);
	main_layout->addLayout(button_layout);

	connect(family_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &CustomFFmpegAudioConfigDialog::family_changed);
	connect(save_btn, &QPushButton::clicked,
		this, &CustomFFmpegAudioConfigDialog::save_current);
	connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);

	load_family(0);
}

void CustomFFmpegAudioConfigDialog::family_changed(int index)
{
	if (index < 0) return;
	if (previous_index_ >= 0)
		save_family(previous_index_);
	load_family(index);
	previous_index_ = index;
}

void CustomFFmpegAudioConfigDialog::save_current()
{
	int index = family_combo->currentIndex();
	if (index < 0) return;
	save_family(index);
	accept();
}

void CustomFFmpegAudioConfigDialog::load_family(int index)
{
	if (index < 0) return;
	QString family_id = family_combo->itemData(index).toString();

	config_t *config = open_encoder_config();
	if (config) {
		const char *options = config_get_string(config,
			family_id.toUtf8().constData(), "custom_options");
		blog(LOG_INFO, "[Custom FFmpeg Audio] load_family [%s] = '%s'",
		     family_id.toUtf8().constData(), options ? options : "(null)");
		custom_options_edit->setPlainText(options ? options : "");
		config_close(config);
	} else {
		blog(LOG_WARNING, "[Custom FFmpeg Audio] load_family: open_encoder_config returned NULL");
		custom_options_edit->clear();
	}
}

void CustomFFmpegAudioConfigDialog::save_family(int index)
{
	if (index < 0) return;
	QString family_id = family_combo->itemData(index).toString();
	QString options = custom_options_edit->toPlainText();

	blog(LOG_INFO, "[Custom FFmpeg Audio] save_family [%s] = '%s'",
	     family_id.toUtf8().constData(), options.toUtf8().constData());

	config_t *config = open_encoder_config();
	if (config) {
		config_set_string(config, family_id.toUtf8().constData(),
			"custom_options", options.toUtf8().constData());
		config_save(config);
		config_close(config);
	} else {
		blog(LOG_WARNING, "[Custom FFmpeg Audio] save_family: open_encoder_config returned NULL");
	}
}