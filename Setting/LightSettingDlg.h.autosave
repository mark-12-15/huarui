#ifndef LIGHTSETTINGDLG_H
#define LIGHTSETTINGDLG_H

#include <QDialog>

namespace Ui {
class LightSettingDlg;
}

class LightSettingDlg : public QDialog
{
    Q_OBJECT

public:
    explicit LightSettingDlg(QWidget *parent = nullptr);
    ~LightSettingDlg();

private slots:
    void sliderValueChanged(int value);

    void on_lineEdit_editingFinished();

    void on_pushButton_2_clicked();
    
    void on_pushButton_clicked();
    
private:
    Ui::LightSettingDlg *ui;
};

#endif // LIGHTSETTINGDLG_H
