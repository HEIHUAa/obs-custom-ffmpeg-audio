#pragma once

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