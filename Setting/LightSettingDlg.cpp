#include "LightSettingDlg.h"
#include "ui_LightSettingDlg.h"
#include "BCLocalServer.h"

LightSettingDlg::LightSettingDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LightSettingDlg)
{
    ui->setupUi(this);

    // init by device
    connect(ui->horizontalSlider, &QSlider::valueChanged, this, &LightSettingDlg::sliderValueChanged);

    ui->lineEdit->setText(QString::number(BCLocalServer::Application()->_lightValue));
    on_lineEdit_editingFinished();
}

LightSettingDlg::~LightSettingDlg()
{
    delete ui;
}

void LightSettingDlg::sliderValueChanged(int value)
{
    int v = 255*value*0.01;
    ui->lineEdit->setText(QString::number(v));
    ui->lblValue->setText(QString("(%1%)").arg(value));
}

void LightSettingDlg::on_lineEdit_editingFinished()
{
    int v = ui->lineEdit->text().toInt();
    if (v < 0 || v > 255)
    {
        return;
    }

    disconnect(ui->horizontalSlider, &QSlider::valueChanged, this, &LightSettingDlg::sliderValueChanged);
    int value = (v / 255.0)*100;
    ui->horizontalSlider->setValue(value);
    ui->lblValue->setText(QString("(%1%)").arg(value));
    connect(ui->horizontalSlider, &QSlider::valueChanged, this, &LightSettingDlg::sliderValueChanged);
}

void LightSettingDlg::on_pushButton_2_clicked()
{
    close();
}

void LightSettingDlg::on_pushButton_clicked()
{
    // send cmd
    BCLocalServer::Application()->updateLightToDevice(ui->lineEdit->text().toInt());

    close();
}
