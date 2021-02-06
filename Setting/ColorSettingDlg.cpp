#include "ColorSettingDlg.h"
#include "ui_ColorSettingDlg.h"
#include "BCLocalServer.h"

ColorSettingDlg::ColorSettingDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ColorSettingDlg)
{
    ui->setupUi(this);

    // init by device
    connect(ui->sliderR, &QSlider::valueChanged, this, &ColorSettingDlg::sliderValueRChanged);
    connect(ui->sliderG, &QSlider::valueChanged, this, &ColorSettingDlg::sliderValueGChanged);
    connect(ui->sliderB, &QSlider::valueChanged, this, &ColorSettingDlg::sliderValueBChanged);

    ui->lineEditR->setText(QString::number(BCLocalServer::Application()->_rValue));
    ui->lineEditG->setText(QString::number(BCLocalServer::Application()->_bValue));
    ui->lineEditB->setText(QString::number(BCLocalServer::Application()->_gValue));

    on_lineEditR_editingFinished();
    on_lineEditG_editingFinished();
    on_lineEditB_editingFinished();
}

ColorSettingDlg::~ColorSettingDlg()
{
    delete ui;
}

void ColorSettingDlg::sliderValueRChanged(int value)
{
    int v = 255*value*0.01;
    ui->lineEditR->setText(QString::number(v));
    ui->lblValueR->setText(QString("(%1%)").arg(value));
}

void ColorSettingDlg::sliderValueGChanged(int value)
{
    int v = 255*value*0.01;
    ui->lineEditG->setText(QString::number(v));
    ui->lblValueG->setText(QString("(%1%)").arg(value));
}

void ColorSettingDlg::sliderValueBChanged(int value)
{
    int v = 255*value*0.01;
    ui->lineEditB->setText(QString::number(v));
    ui->lblValueB->setText(QString("(%1%)").arg(value));
}

void ColorSettingDlg::on_lineEditR_editingFinished()
{
    int v = ui->lineEditR->text().toInt();
    if (v < 0 || v > 255)
    {
        return;
    }

    disconnect(ui->sliderR, &QSlider::valueChanged, this, &ColorSettingDlg::sliderValueRChanged);
    int value = (v / 255.0)*100;
    ui->sliderR->setValue(value);
    ui->lblValueR->setText(QString("(%1%)").arg(value));
    connect(ui->sliderR, &QSlider::valueChanged, this, &ColorSettingDlg::sliderValueRChanged);
}

void ColorSettingDlg::on_lineEditG_editingFinished()
{
    int v = ui->lineEditG->text().toInt();
    if (v < 0 || v > 255)
    {
        return;
    }

    disconnect(ui->sliderG, &QSlider::valueChanged, this, &ColorSettingDlg::sliderValueGChanged);
    int value = (v / 255.0)*100;
    ui->sliderG->setValue(value);
    ui->lblValueG->setText(QString("(%1%)").arg(value));
    connect(ui->sliderG, &QSlider::valueChanged, this, &ColorSettingDlg::sliderValueGChanged);
}

void ColorSettingDlg::on_lineEditB_editingFinished()
{
    int v = ui->lineEditB->text().toInt();
    if (v < 0 || v > 255)
    {
        return;
    }

    disconnect(ui->sliderB, &QSlider::valueChanged, this, &ColorSettingDlg::sliderValueBChanged);
    int value = (v / 255.0)*100;
    ui->sliderB->setValue(value);
    ui->lblValueB->setText(QString("(%1%)").arg(value));
    connect(ui->sliderB, &QSlider::valueChanged, this, &ColorSettingDlg::sliderValueBChanged);
}

void ColorSettingDlg::on_pushButton_2_clicked()
{
    close();
}

void ColorSettingDlg::on_pushButton_clicked()
{
    // send cmd
    BCLocalServer::Application()->updateColorToDevice(ui->lineEditR->text().toInt(),
                                                      ui->lineEditG->text().toInt(),
                                                      ui->lineEditB->text().toInt());

    close();
}
