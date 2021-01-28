#ifndef DEVICEFORMATDLG_H
#define DEVICEFORMATDLG_H

#include <QDialog>

namespace Ui {
class DeviceFormatDlg;
}

class DeviceFormatDlg : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceFormatDlg(QWidget *parent = nullptr);
    ~DeviceFormatDlg();

private slots:
    void on_btnCancel_clicked();

    void on_btnApply_clicked();

    void on_sbw_valueChanged(int arg1);

    void on_sbh_valueChanged(int arg1);

    void on_lew_editingFinished();

    void on_leh_editingFinished();

private:
    Ui::DeviceFormatDlg *ui;

    void init();
    void updateLocalFile();
};

#endif // DEVICEFORMATDLG_H
