#ifndef DEVICECONNECTDLG_H
#define DEVICECONNECTDLG_H

#include <QDialog>

namespace Ui {
class DeviceConnectDlg;
}

class DeviceConnectDlg : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceConnectDlg(QWidget *parent = nullptr);
    ~DeviceConnectDlg();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

private:
    Ui::DeviceConnectDlg *ui;
};

#endif // DEVICECONNECTDLG_H
