#include "BCSignalListWidgetData.h"
#include "ui_BCSignalListWidgetData.h"
#include <QDebug>
#include <QMimeData>
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

void BCSignalListWidgetData::dragEnterEvent(QDragEnterEvent* event){
    //相同类不能放
    BCSignalListWidgetData* source = qobject_cast<BCSignalListWidgetData*>(event->source());
    if(source && (source != this)){
        return;
    }

    if (event->mimeData()->hasFormat("inputChannel"))
    {
        //signal_data(m_pChannel);
        event->accept();
    }
    else
       event->ignore();
}

void BCSignalListWidgetData::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        if(NULL == m_pChannel)
            return;

        // 构造拖拽数据
        QList<QString> listdata;

        listdata.append("1");       //1 类型标识
        listdata.append( QString::number(m_pChannel->GetChannelType()) );
        listdata.append( QString::number(m_pChannel->GetChannelID()) );

        QByteArray exData;
        QDataStream dataStream(&exData,QIODevice::WriteOnly);
        dataStream << listdata;

        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData();
        mimeData->setData("inputChannel", exData);
        drag->setMimeData(mimeData);
        drag->exec(Qt::CopyAction);
    }
}
