#include "DeviceFormatDlg.h"
#include "ui_DeviceFormatDlg.h"
#include <QFile>
#include <QFileDialog>
#include "BCCommon.h"
#include "BCMRoom.h"
#include "BCMGroupDisplay.h"
#include "BCLocalServer.h"

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
    // from memery
    auto room = BCCommon::Application()->GetMRoom(0);
    if (room) {
        ui->rbtnFull->setChecked(room->isFullScreeMode);
        ui->rbtnMulti->setChecked(!room->isFullScreeMode);

        auto group = room->GetGroupDisplay(0);
        if (group) {
            auto arr = group->GetArraySize();
            auto size = group->GetResolutionSize();

            ui->sbw->setValue(arr.width());
            ui->sbh->setValue(arr.height());

            ui->lew->setText(QString::number(size.width()));
            ui->leh->setText(QString::number(size.height()));

            ui->lblValue->setText(QString("%1*%2").arg(arr.width()*size.width()).arg(arr.height()*size.height()));
        }
    }
}

void DeviceFormatDlg::on_btnCancel_clicked()
{
    close();
}

void DeviceFormatDlg::on_btnApply_clicked()
{
    // write to device
    BCLocalServer::Application()->updateFormatToDevice(ui->rbtnFull->isChecked(),
                                                       ui->sbw->value(), ui->sbh->value(),
                                                       ui->lew->text().toInt(), ui->leh->text().toInt());

    // write to local file
    BCLocalServer::Application()->isUpdateRoomConfig(ui->rbtnFull->isChecked(),
                                                     ui->sbw->value(), ui->sbh->value(),
                                                     ui->lew->text().toInt(), ui->leh->text().toInt());

    // refresh mainwindow
    BCCommon::Application()->RefreshMainWindow();

    // close
    close();
}

void DeviceFormatDlg::on_sbw_valueChanged(int /*arg1*/)
{
    ui->lblValue->setText(QString("%1*%2")
                          .arg(ui->sbw->value()*ui->lew->text().toInt())
                          .arg(ui->sbh->value()*ui->leh->text().toInt()));
}

void DeviceFormatDlg::on_sbh_valueChanged(int /*arg1*/)
{
    ui->lblValue->setText(QString("%1*%2")
                          .arg(ui->sbw->value()*ui->lew->text().toInt())
                          .arg(ui->sbh->value()*ui->leh->text().toInt()));
}

void DeviceFormatDlg::on_lew_editingFinished()
{
    ui->lblValue->setText(QString("%1*%2")
                          .arg(ui->sbw->value()*ui->lew->text().toInt())
                          .arg(ui->sbh->value()*ui->leh->text().toInt()));
}

void DeviceFormatDlg::on_leh_editingFinished()
{
    ui->lblValue->setText(QString("%1*%2")
                          .arg(ui->sbw->value()*ui->lew->text().toInt())
                          .arg(ui->sbh->value()*ui->leh->text().toInt()));
}

void DeviceFormatDlg::on_btnSelect_clicked()
{
    auto fileName = QFileDialog::getOpenFileName(BCCommon::Application(),
                                                 tr("选择配置文件"),
                                                 ".",
                                                 tr("接收卡配置文件(*.rxcfg *.scrcfg)"));
     if (!fileName.isEmpty()) {
         ui->leFileName->setText(fileName);
         QFile file(fileName);
         if (file.open(QIODevice::ReadOnly)) {
             auto data = file.readAll();
             file.close();

             BCLocalServer::Application()->updateConfigFileToDevice(data);
         }
     }
}
