#include "DeviceFormatDlg.h"
#include "ui_DeviceFormatDlg.h"
#include <QFile>
#include "BCCommon.h"
#include "BCMRoom.h"
#include "BCMGroupDisplay.h"

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

void DeviceFormatDlg::updateLocalFile()
{
    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();
    docElem.setAttribute("fullMode", ui->rbtnFull->isChecked()?"1":"0");
    docElem.setAttribute("arrayX", ui->sbw->value());
    docElem.setAttribute("arrayY", ui->sbh->value());
    docElem.setAttribute("width", ui->lew->text().toInt());
    docElem.setAttribute("height", ui->leh->text().toInt());

    // 清空场景和输入通道
    while (!docElem.childNodes().isEmpty())
        docElem.removeChild(docElem.firstChild());

    // 重新添加输入通道
    auto count = ui->sbw->value() * ui->sbh->value();
    if (count > 0) {
        auto eleCh = doc.createElement(QString("Channel"));

        for (int i = 0; i < count; i++) {
            auto eleNode = doc.createElement(QString("Node"));
            eleNode.setAttribute("id", QString::number(i));
            eleNode.setAttribute("name", QString("电脑%1").arg(i+1));

            eleCh.appendChild(eleNode);
        }

        docElem.appendChild(eleCh);
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void DeviceFormatDlg::on_btnCancel_clicked()
{
    close();
}

void DeviceFormatDlg::on_btnApply_clicked()
{
    // write to device

    // write to local file
    updateLocalFile();

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
