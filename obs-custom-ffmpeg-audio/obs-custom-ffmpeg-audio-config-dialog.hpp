#pragma once

#include <QDialog>

class QComboBox;
class QTextEdit;

class CustomFFmpegAudioConfigDialog : public QDialog {
	Q_OBJECT
public:
	CustomFFmpegAudioConfigDialog(QWidget *parent = nullptr);

private slots:
	void family_changed(int index);
	void save_current();

private:
	QComboBox *family_combo;
	QTextEdit *custom_options_edit;
	int previous_index_ = -1;

	void load_family(int index);
	void save_family(int index);
};