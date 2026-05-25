#include "obs-custom-ffmpeg-audio-config-dialog.hpp"
#include "obs-custom-ffmpeg-audio-encoder.hpp"

#include <obs-module.h>
#include <util/config-file.h>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>

/* ── 配置读写 ──────────────────────────────────────────────── */

static config_t *open_config()
{
	char *path = obs_module_get_config_path(obs_current_module(), "config.ini");
	config_t *config = nullptr;
	int ret = config_open(&config, path, CONFIG_OPEN_ALWAYS);
	bfree(path);
	if (ret != CONFIG_SUCCESS)
		return nullptr;
	return config;
}

void load_encoder_config(const char *family_id,
			 std::string &out_codec,
			 bool &out_use_quality,
			 int &out_quality,
			 std::string &out_sample_rate,
			 std::string &out_custom_options,
			 int &out_strict_compliance)
{
	config_t *config = open_config();
	if (!config)
		return;

	const char *codec = config_get_string(config, family_id, "codec");
	if (codec)
		out_codec = codec;

	out_use_quality = config_get_bool(config, family_id, "use_quality");
	out_quality = (int)config_get_int(config, family_id, "quality");

	const char *sr = config_get_string(config, family_id, "sample_rate");
	if (sr)
		out_sample_rate = sr;

	const char *opts = config_get_string(config, family_id, "custom_options");
	if (opts)
		out_custom_options = opts;

	out_strict_compliance = (int)config_get_int(config, family_id, "strict_compliance");

	int bitrate = (int)config_get_int(config, family_id, "bitrate");
	if (bitrate > 0)
		blog(LOG_INFO, "[Custom FFmpeg Audio] config bitrate=%d (ignored, OBS manages bitrate)", bitrate);

	config_close(config);
}

void save_encoder_config(const char *family_id,
			 const char *codec,
			 bool use_quality,
			 int quality,
			 const char *sample_rate,
			 const char *custom_options,
			 int strict_compliance)
{
	config_t *config = open_config();
	if (!config)
		return;

	config_set_string(config, family_id, "codec", codec);
	config_set_bool(config, family_id, "use_quality", use_quality);
	config_set_int(config, family_id, "quality", quality);
	config_set_string(config, family_id, "sample_rate", sample_rate);
	config_set_string(config, family_id, "custom_options", custom_options);
	config_set_int(config, family_id, "strict_compliance", strict_compliance);

	config_save(config);
	config_close(config);

	blog(LOG_INFO, "[Custom FFmpeg Audio] saved config for %s: codec=%s, use_quality=%d, quality=%d, "
	     "sample_rate=%s, strict_compliance=%d",
	     family_id, codec, use_quality, quality, sample_rate, strict_compliance);
}

/* ── 族定义 ───────────────────────────────────────────────── */

static int count_families()
{
	int n = 0;
	while (families[n].id)
		n++;
	return n;
}

static const encoder_family *find_family_by_index(int index)
{
	if (index >= 0 && index < count_families())
		return &families[index];
	return &families[0];
}

/* ── 对话框构造函数 ────────────────────────────────────────── */

CustomFFmpegAudioConfigDialog::CustomFFmpegAudioConfigDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(obs_module_text("CustomFFmpegAudio.ConfigTitle"));
	setMinimumWidth(420);

	auto *main_layout = new QVBoxLayout(this);
	auto *form = new QFormLayout;

	m_family_combo = new QComboBox(this);

	for (const encoder_family *f = families; f->id; f++)
		m_family_combo->addItem(f->display_name, f->id);

	connect(m_family_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &CustomFFmpegAudioConfigDialog::on_family_changed);
	form->addRow(obs_module_text("CustomFFmpegAudio.EncoderFamily"), m_family_combo);

	m_codec_combo = new QComboBox(this);
	form->addRow(obs_module_text("Codec"), m_codec_combo);

	m_use_quality = new QCheckBox(obs_module_text("UseQuality"), this);
	connect(m_use_quality, &QCheckBox::toggled, this, &CustomFFmpegAudioConfigDialog::on_quality_toggled);
	form->addRow("", m_use_quality);

	auto *quality_layout = new QHBoxLayout;
	m_quality_slider = new QSlider(Qt::Horizontal, this);
	m_quality_slider->setRange(0, 10);
	m_quality_slider->setValue(5);
	m_quality_label = new QLabel("5", this);
	quality_layout->addWidget(m_quality_slider);
	quality_layout->addWidget(m_quality_label);
	connect(m_quality_slider, &QSlider::valueChanged, this, [this](int v) {
		m_quality_label->setText(QString::number(v));
	});
	form->addRow(obs_module_text("Quality"), quality_layout);

	m_sample_rate_combo = new QComboBox(this);
	m_sample_rate_combo->addItem(obs_module_text("SampleRate.Auto"), "auto");
	m_sample_rate_combo->addItem("44100", "44100");
	m_sample_rate_combo->addItem("48000", "48000");
	m_sample_rate_combo->addItem("96000", "96000");
	m_sample_rate_combo->addItem("192000", "192000");
	m_sample_rate_combo->addItem("22050", "22050");
	m_sample_rate_combo->addItem("32000", "32000");
	form->addRow(obs_module_text("SampleRate"), m_sample_rate_combo);

	m_custom_options = new QTextEdit(this);
	m_custom_options->setPlaceholderText(obs_module_text("CustomOptions.Tooltip"));
	m_custom_options->setMaximumHeight(80);
	form->addRow(obs_module_text("CustomOptions"), m_custom_options);

	auto *strict_layout = new QHBoxLayout;
	m_strict_slider = new QSlider(Qt::Horizontal, this);
	m_strict_slider->setRange(-2, 2);
	m_strict_slider->setValue(-2);
	m_strict_label = new QLabel("-2", this);
	strict_layout->addWidget(m_strict_slider);
	strict_layout->addWidget(m_strict_label);
	connect(m_strict_slider, &QSlider::valueChanged, this, [this](int v) {
		m_strict_label->setText(QString::number(v));
	});
	form->addRow(obs_module_text("StrictCompliance"), strict_layout);

	main_layout->addLayout(form);

	auto *btn_layout = new QHBoxLayout;
	btn_layout->addStretch();
	m_ok = new QPushButton(obs_module_text("CustomFFmpegAudio.OK"), this);
	m_cancel = new QPushButton(obs_module_text("CustomFFmpegAudio.Cancel"), this);
	connect(m_ok, &QPushButton::clicked, this, &CustomFFmpegAudioConfigDialog::on_save);
	connect(m_cancel, &QPushButton::clicked, this, &QDialog::reject);
	btn_layout->addWidget(m_ok);
	btn_layout->addWidget(m_cancel);
	main_layout->addLayout(btn_layout);

	if (m_family_combo->count() > 0)
		on_family_changed(0);
}

/* ── 族切换 ──────────────────────────────────────────────── */

void CustomFFmpegAudioConfigDialog::on_family_changed(int index)
{
	const encoder_family *family = find_family_by_index(index);
	if (!family)
		return;

	populate_codecs(family);
	load_config(family->id);
}

void CustomFFmpegAudioConfigDialog::populate_codecs(const encoder_family *family)
{
	m_codec_combo->clear();
	for (const codec_entry *e = family->entries; e->name; e++)
		m_codec_combo->addItem(e->name, e->codec_id);
}

void CustomFFmpegAudioConfigDialog::on_quality_toggled(bool checked)
{
	m_quality_slider->setEnabled(checked);
	m_quality_label->setEnabled(checked);
}

/* ── 加载/写入 ────────────────────────────────────────────── */

void CustomFFmpegAudioConfigDialog::load_config(const char *family_id)
{
	std::string codec, sample_rate, custom_options;
	bool use_quality = false;
	int quality = 5;
	int strict = -2;

	::load_encoder_config(family_id, codec, use_quality, quality,
			      sample_rate, custom_options, strict);

	int idx = m_codec_combo->findData(QString::fromStdString(codec));
	if (idx >= 0)
		m_codec_combo->setCurrentIndex(idx);

	m_use_quality->setChecked(use_quality);
	m_quality_slider->setValue(quality);
	m_quality_slider->setEnabled(use_quality);
	m_quality_label->setEnabled(use_quality);

	idx = m_sample_rate_combo->findData(QString::fromStdString(sample_rate));
	if (idx >= 0)
		m_sample_rate_combo->setCurrentIndex(idx);

	m_custom_options->setText(QString::fromStdString(custom_options));
	m_strict_slider->setValue(strict);
}

void CustomFFmpegAudioConfigDialog::save_config(const char *family_id)
{
	::save_encoder_config(family_id,
			      m_codec_combo->currentData().toString().toUtf8().constData(),
			      m_use_quality->isChecked(),
			      m_quality_slider->value(),
			      m_sample_rate_combo->currentData().toString().toUtf8().constData(),
			      m_custom_options->toPlainText().toUtf8().constData(),
			      m_strict_slider->value());
}

void CustomFFmpegAudioConfigDialog::on_save()
{
	for (int i = 0; i < m_family_combo->count(); i++) {
		QByteArray fid = m_family_combo->itemData(i).toByteArray();
		save_config(fid.constData());
	}
	accept();
}