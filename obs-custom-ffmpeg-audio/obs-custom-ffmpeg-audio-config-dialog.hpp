#pragma once

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <obs-module.h>
#include <util/config-file.h>

struct encoder_family;

config_t *open_encoder_config(void);
void set_encoder_config_module(obs_module_t *mod);

class CustomFFmpegAudioConfigDialog : public QDialog {
	Q_OBJECT

public:
	CustomFFmpegAudioConfigDialog(QWidget *parent = nullptr);
	~CustomFFmpegAudioConfigDialog() = default;

private slots:
	void on_family_changed(int index);
	void on_quality_toggled(bool checked);
	void on_save();

private:
	void load_config(const char *family_id);
	void save_config(const char *family_id);
	void populate_codecs(const encoder_family *family);

	QComboBox *m_family_combo;
	QComboBox *m_codec_combo;
	QCheckBox *m_use_quality;
	QSlider *m_quality_slider;
	QLabel *m_quality_label;
	QComboBox *m_sample_rate_combo;
	QTextEdit *m_custom_options;
	QSlider *m_strict_slider;
	QLabel *m_strict_label;
	QPushButton *m_ok;
	QPushButton *m_cancel;
};

void load_encoder_config(const char *family_id,
			 std::string &out_codec,
			 bool &out_use_quality,
			 int &out_quality,
			 std::string &out_sample_rate,
			 std::string &out_custom_options,
			 int &out_strict_compliance);

void save_encoder_config(const char *family_id,
			 const char *codec,
			 bool use_quality,
			 int quality,
			 const char *sample_rate,
			 const char *custom_options,
			 int strict_compliance);