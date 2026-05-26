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
#include <QCheckBox>
#include <QSlider>

CustomFFmpegAudioConfigDialog::CustomFFmpegAudioConfigDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(obs_module_text("CustomFFmpegAudio.ConfigTitle"));
	setMinimumSize(560, 500);

	auto *main_layout = new QVBoxLayout(this);

	auto *family_layout = new QHBoxLayout();
	family_layout->addWidget(new QLabel(obs_module_text("CustomFFmpegAudio.EncoderFamily")));
	family_combo = new QComboBox();
	for (const encoder_family *f = families; f->id; f++)
		family_combo->addItem(f->display_name, f->id);
	family_layout->addWidget(family_combo, 1);
	main_layout->addLayout(family_layout);

	auto *codec_layout = new QHBoxLayout();
	codec_layout->addWidget(new QLabel(obs_module_text("Codec")));
	codec_combo = new QComboBox();
	codec_layout->addWidget(codec_combo, 1);
	main_layout->addLayout(codec_layout);

	auto *sr_layout = new QHBoxLayout();
	sr_layout->addWidget(new QLabel(obs_module_text("SampleRate")));
	sample_rate_combo = new QComboBox();
	sample_rate_combo->addItem(obs_module_text("SampleRate.Auto"), "auto");
	sample_rate_combo->addItem("44100", "44100");
	sample_rate_combo->addItem("48000", "48000");
	sample_rate_combo->addItem("96000", "96000");
	sample_rate_combo->addItem("192000", "192000");
	sample_rate_combo->addItem("22050", "22050");
	sample_rate_combo->addItem("32000", "32000");
	sr_layout->addWidget(sample_rate_combo, 1);
	main_layout->addLayout(sr_layout);

	auto *quality_group_widget = new QGroupBox(obs_module_text("Quality"));
	auto *quality_layout = new QVBoxLayout(quality_group_widget);
	quality_group = quality_group_widget;

	use_quality_check = new QCheckBox(obs_module_text("UseQuality"));
	use_quality_check->setToolTip(obs_module_text("UseQuality.Tooltip"));
	quality_layout->addWidget(use_quality_check);

	auto *quality_slider_layout = new QHBoxLayout();
	quality_label = new QLabel(obs_module_text("Quality"));
	quality_slider = new QSlider(Qt::Horizontal);
	quality_slider->setRange(0, 10);
	quality_slider->setTickPosition(QSlider::TicksBelow);
	quality_slider->setTickInterval(1);
	quality_slider_layout->addWidget(quality_label);
	quality_slider_layout->addWidget(quality_slider, 1);
	quality_layout->addLayout(quality_slider_layout);

	main_layout->addWidget(quality_group);

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
	connect(use_quality_check, &QCheckBox::toggled,
		this, &CustomFFmpegAudioConfigDialog::use_quality_toggled);
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

void CustomFFmpegAudioConfigDialog::use_quality_toggled(bool checked)
{
	quality_label->setVisible(checked);
	quality_slider->setVisible(checked);
}

void CustomFFmpegAudioConfigDialog::load_family(int index)
{
	if (index < 0) return;
	QString family_id = family_combo->itemData(index).toString();

	const encoder_family *family = nullptr;
	for (const encoder_family *f = families; f->id; f++) {
		if (family_id == f->id) {
			family = f;
			break;
		}
	}

	codec_combo->blockSignals(true);
	codec_combo->clear();
	if (family) {
		for (const codec_entry *e = family->entries; e->name; e++) {
			QString data = QString("%1|%2").arg(e->codec_id).arg(e->sample_fmt);
			codec_combo->addItem(e->name, data);
		}
	}
	codec_combo->blockSignals(false);

	quality_group->setVisible(family ? family->supports_quality : false);

	config_t *config = open_encoder_config();
	if (config) {
		const char *codec_val = config_get_string(config,
			family_id.toUtf8().constData(), "codec");
		int forced_fmt = -1;
		const char *fmt_str = config_get_string(config,
			family_id.toUtf8().constData(), "forced_sample_fmt");
		if (fmt_str)
			forced_fmt = atoi(fmt_str);

		QString search_key = QString("%1|%2")
			.arg(codec_val ? codec_val : "")
			.arg(forced_fmt);
		int codec_idx = codec_combo->findData(search_key);
		if (codec_idx >= 0)
			codec_combo->setCurrentIndex(codec_idx);

		const char *sr_val = config_get_string(config,
			family_id.toUtf8().constData(), "sample_rate");
		int sr_idx = sample_rate_combo->findData(sr_val ? sr_val : "auto");
		if (sr_idx >= 0)
			sample_rate_combo->setCurrentIndex(sr_idx);

		bool uq = config_get_bool(config, family_id.toUtf8().constData(), "use_quality");
		use_quality_check->setChecked(uq);

		int q = 3;
		const char *q_str = config_get_string(config,
			family_id.toUtf8().constData(), "quality");
		if (q_str)
			q = atoi(q_str);
		quality_slider->setValue(q);

		const char *options = config_get_string(config,
			family_id.toUtf8().constData(), "custom_options");
		custom_options_edit->setPlainText(options ? options : "");

		config_close(config);
	} else {
		custom_options_edit->clear();
	}

	use_quality_toggled(use_quality_check->isChecked());
}

void CustomFFmpegAudioConfigDialog::save_family(int index)
{
	if (index < 0) return;
	QString family_id = family_combo->itemData(index).toString();

	QString codec_data = codec_combo->currentData().toString();
	QStringList parts = codec_data.split('|');
	QString codec_id = parts.value(0);
	int forced_fmt = parts.value(1).toInt();
	QString sr = sample_rate_combo->currentData().toString();
	bool uq = use_quality_check->isChecked();
	int q = quality_slider->value();
	QString options = custom_options_edit->toPlainText();

	blog(LOG_INFO, "[Custom FFmpeg Audio] save_family [%s] codec=%s forced_fmt=%d sr=%s uq=%d q=%d custom='%s'",
	     family_id.toUtf8().constData(), codec_id.toUtf8().constData(), forced_fmt,
	     sr.toUtf8().constData(), uq, q,
	     options.toUtf8().constData());

	config_t *config = open_encoder_config();
	if (config) {
		config_set_string(config, family_id.toUtf8().constData(),
			"codec", codec_id.toUtf8().constData());
		config_set_int(config, family_id.toUtf8().constData(),
			"forced_sample_fmt", forced_fmt);
		config_set_string(config, family_id.toUtf8().constData(),
			"sample_rate", sr.toUtf8().constData());
		config_set_int(config, family_id.toUtf8().constData(),
			"use_quality", uq ? 1 : 0);
		config_set_int(config, family_id.toUtf8().constData(),
			"quality", q);
		config_set_string(config, family_id.toUtf8().constData(),
			"custom_options", options.toUtf8().constData());
		config_save(config);
		config_close(config);
	} else {
		blog(LOG_WARNING, "[Custom FFmpeg Audio] save_family: open_encoder_config returned NULL");
	}
}