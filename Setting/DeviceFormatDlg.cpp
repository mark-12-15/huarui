#include "DeviceFormatDlg.h"
#include "ui_DeviceFormatDlg.h"
#include <QFile>

DeviceFormatDlg::DeviceFormatDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeviceFormatDlg)
{
    ui->setupUi(this);

    init();
}

DeviceFormatDlg::~DeviceFormatDlg()
{
    delete ui;
}

void DeviceFormatDlg::init()
{

}
void DeviceFormatDlg::on_btnCancel_clicked()
{
    close();
}
