#include "BCSignalListWidgetData.h"
#include "ui_BCSignalListWidgetData.h"
#include <QDebug>
#include <QDrag>

#include "BCControl.h"
#include "BCSignal.h"
#include "../Model/BCMChannel.h"
#include "../Model/BCMGroupDisplay.h"
#include "../Common/BCCommon.h"

// 预览小窗的尺寸
#define PREVIEWWIDTH    240
#define PREVIEWHEIGHT   136

BCSignalListWidgetData::BCSignalListWidgetData(BCMChannel *pChannel, BCControl *parent) :
    QWidget(parent),
    ui(new Ui::BCSignalListWidgetData)
{
    ui->setupUi(this);
    this->setAcceptDrops(true);

    m_pInputChannelWidget = parent;
    m_pGroupInputChannelWidget = NULL;
    m_pChannel = pChannel;
    m_hd = (HWND)ui->label_view->winId();

    // 设置名称
    RefreshInputChannelName();
}

BCSignalListWidgetData::BCSignalListWidgetData(BCMChannel *pChannel, BCSignal *parent) :
    QWidget(parent),
    ui(new Ui::BCSignalListWidgetData)
{
    ui->setupUi(this);
    this->setAcceptDrops(true);

    m_pInputChannelWidget = NULL;
    m_pGroupInputChannelWidget = parent;
    m_pChannel = pChannel;
    m_hd = (HWND)ui->label_view->winId();

    // 设置名称
    RefreshInputChannelName();
}

BCSignalListWidgetData::~BCSignalListWidgetData()
{
    delete ui;
}

void BCSignalListWidgetData::RefreshInputChannelName()
{
    if (NULL == m_pChannel)
        return;

    // 设置名称
    ui->label->setText(m_pChannel->GetChannelName().isEmpty() ? m_pChannel->GetChannelBaseName() : m_pChannel->GetChannelName());
}

