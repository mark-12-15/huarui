#ifndef COLORSETTINGDLG_H
#define COLORSETTINGDLG_H

#include <QDialog>

namespace Ui {
class ColorSettingDlg;
}

class ColorSettingDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ColorSettingDlg(QWidget *parent = nullptr);
    ~ColorSettingDlg();

private slots:
    void sliderValueRChanged(int value);
    void sliderValueGChanged(int value);
    void sliderValueBChanged(int value);

    void on_lineEditR_editingFinished();
    void on_lineEditG_editingFinished();
    void on_lineEditB_editingFinished();

    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

private:
    Ui::ColorSettingDlg *ui;
};

#endif // COLORSETTINGDLG_H
