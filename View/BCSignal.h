﻿#ifndef BCSIGNAL_H
#define BCSIGNAL_H

#include <QTime>
#include <QWidget>

namespace Ui {
class BCSignal;
}

class BCWidgetBtn;
class BCMChannel;
class BCSignal : public QWidget
{
    Q_OBJECT

public:
    explicit BCSignal(bool bVisible, int minHeight, QWidget *parent = 0);

    // 刷新
    void Refresh();

    // 刷新名称
    void RefreshInputChannelName();

    // 是否隐藏内容
    bool IsVisible();

    BCMChannel* GetCurrentChannel();

    int m_minHeight;

private slots:
    void on_m_pShowModeComboBox_currentIndexChanged(int index);

    void on_m_pShowVedioCheckBox_clicked(bool checked);

    void onSetVisible(bool bVisible);

    void onActionTriggered(int value);
    void onRangeChanged(int min, int max);

    void on_m_pInputChannelVedioTabWidget_currentChanged(int index);

private:
    void init(bool bVisible);

private:
    Ui::BCSignal *ui;

    BCWidgetBtn *m_pHeaderBtn;  // 自定义Header

    QTime       beginTime;          // 记录滚动轴时间
    int         m_nScrollMinValue;  // 记录滚动轴的最大最小值
    int         m_nScrollMaxValue;

    bool event(QEvent *event);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);

    bool m_bModifyHeight;       // 是否修改高度，悬停到最下面时值为true
    bool m_bPress;
};

#endif // BCSIGNAL_H
