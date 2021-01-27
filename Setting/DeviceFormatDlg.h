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

private:
    Ui::DeviceFormatDlg *ui;

    void init();
};

#endif // DEVICEFORMATDLG_H
