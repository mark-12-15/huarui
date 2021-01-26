#include "DeviceConnectDlg.h"
#include "ui_DeviceConnectDlg.h"
#include <QSerialPortInfo>
#include "BCLocalServer.h"

DeviceConnectDlg::DeviceConnectDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeviceConnectDlg)
{
    ui->setupUi(this);

    // 初始化COM连接参数
    QList<QSerialPortInfo> lstSerialPort = QSerialPortInfo::availablePorts();
    for (int i = 0; i < lstSerialPort.count(); i++) {
        QSerialPortInfo info = lstSerialPort.at( i );
        ui->cbCom->addItem( info.portName() );
    }

    if (-1 != ui->cbCom->findText(BCLocalServer::Application()->m_qsCurrentCom))
    {
        ui->cbCom->setCurrentText(BCLocalServer::Application()->m_qsCurrentCom);
    }

    auto preRate = QString::number(BCLocalServer::Application()->m_nCurrentBaudRate);
    if (-1 != ui->cbRate->findText(preRate))
    {
        ui->cbRate->setCurrentText(preRate);
    }
}

DeviceConnectDlg::~DeviceConnectDlg()
{
    delete ui;
}

void DeviceConnectDlg::on_pushButton_2_clicked()
{
    this->close();
}

void DeviceConnectDlg::on_pushButton_clicked()
{
    BCLocalServer::Application()->m_qsCurrentCom = ui->cbCom->currentText();
    BCLocalServer::Application()->m_nCurrentBaudRate = ui->cbRate->currentText().toInt();

    BCLocalServer::Application()->ConnectSerialPort();

    this->accept();
}
