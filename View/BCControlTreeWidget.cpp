#include "BCControlTreeWidget.h"
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include "../Common/BCCommon.h"
#include "../Common/BCLocalServer.h"
#include "../Model/BCMGroupDisplay.h"
#include "../Model/BCMChannel.h"
#include "../Model/BCMRoom.h"
#include "BCSignalName.h"
#include "BCRoomWidget.h"
#include "BCGroupDisplayWidget.h"
#include "BCControl.h"
#include "../Model/BCMMatrix.h"
#include "BCFaceWidget.h"
#include "BCToolBar.h"

// 根据开窗数量修改输入通道的颜色显示
// 共分6级，黑色（无窗口），蓝色（1个窗口），紫色，深紫，粉红，橙色（5个或者5个以上）
#define COLOROFCOUNT0   QColor(0, 0, 0)
#define COLOROFCOUNT1   QColor(0, 0, 255)
#define COLOROFCOUNT2   QColor(153, 50, 204)
#define COLOROFCOUNT3   QColor(75, 0, 130)
#define COLOROFCOUNT4   QColor(255, 105, 180)
#define COLOROFCOUNT5   QColor(255, 165, 0)

class BCControlTreeWidgetItem : public QTreeWidgetItem
{
public:
    BCControlTreeWidgetItem(BCMChannel *pChannel, bool bJoint, QTreeWidget *parent);

    BCMChannel *GetMChannel();

    //void RefreshTextColor();

    void RefreshToopTip();

    void RefreshInputChannelName();

private:
    BCMChannel *m_pChannel;
    bool m_bJoint;
};

BCControlTreeWidgetItem::BCControlTreeWidgetItem(BCMChannel *pChannel, bool bJoint, QTreeWidget *parent)
    :QTreeWidgetItem(parent)
{
    m_pChannel = pChannel;
    m_bJoint = bJoint;

    // 设置图片和文字
    MainWindow *pApplication = BCCommon::Application();
    //QString qsText = QString("%1%2-%3").arg(m_pChannel->GetChannelBaseName()).arg(m_pChannel->GetSignalWindowDisplayWidgetCount()).arg(m_pChannel->GetChannelName());
    this->setText(0, m_pChannel->GetChannelName());
    int signalSrc = m_bJoint ? 20 : m_pChannel->GetSignalSource();
    this->setIcon(0, QIcon(pApplication->GetInputChannelIcon( signalSrc )));

    RefreshToopTip();
}

BCMChannel *BCControlTreeWidgetItem::GetMChannel()
{
    return m_pChannel;
}

void BCControlTreeWidgetItem::RefreshInputChannelName()
{
    //QString qsText = QString("%1%2-%3").arg(m_pChannel->GetChannelBaseName()).arg(m_pChannel->GetSignalWindowDisplayWidgetCount()).arg(m_pChannel->GetChannelName());
    this->setText(0, m_pChannel->GetChannelName());
}

void BCControlTreeWidgetItem::RefreshToopTip()
{
    // set tooltip
    if ( m_pChannel->IsEnable() ) {
        QString qsResolution = QObject::tr("%1分辨率: %2*%3").arg(m_pChannel->GetChannelBaseName()).arg(m_pChannel->GetResolution().width()).arg(m_pChannel->GetResolution().height());
        this->setToolTip(0, qsResolution);
    } else {
        this->setToolTip(0, QObject::tr("无信号接入"));
    }

    // set icon
    MainWindow *pApplication = BCCommon::Application();
    int signalSrc = m_bJoint ? 20 : m_pChannel->GetSignalSource();
    //this->setIcon(0, QIcon(pApplication->GetInputChannelIcon(m_pChannel->GetSignalSource()+m_pChannel->IsEnable()?0:11)));
    this->setIcon(0, QIcon(pApplication->GetInputChannelIcon( signalSrc )));
}

//void BCControlTreeWidgetItem::RefreshTextColor()
//{
//    switch ( m_pChannel->GetSignalWindowDisplayItemCount() ) {
//    case 0:
//        setForeground(0, QBrush(COLOROFCOUNT0));
//        break;
//    case 1:
//        setForeground(0, QBrush(COLOROFCOUNT1));
//        break;
//    case 2:
//        setForeground(0, QBrush(COLOROFCOUNT2));
//        break;
//    case 3:
//        setForeground(0, QBrush(COLOROFCOUNT3));
//        break;
//    case 4:
//        setForeground(0, QBrush(COLOROFCOUNT4));
//        break;
//    default:
//        setForeground(0, QBrush(COLOROFCOUNT5));
//        break;
//    }
//}

// --------------------------------------------------------------------------------------------------------------------------------------------

BCControlTreeWidget::BCControlTreeWidget(QWidget *parent)
    :QTreeWidget(parent)
{
    setHeaderHidden(true);

    setStyleSheet( "QTreeView::item:hover{background-color:rgb(0,255,0,50)}" "QTreeView::item:selected{background-color:rgb(255,0,0,100)}" );
}

BCControlTreeWidget::~BCControlTreeWidget()
{

}

void BCControlTreeWidget::contextMenuEvent(QContextMenuEvent* event)
{
    // 如果当前没有item则没有右键菜单，普通item只是修改名称，IPV弹出设置界面
    BCControlTreeWidgetItem *pCurrentItem = dynamic_cast<BCControlTreeWidgetItem*>(this->itemAt(event->pos()));
    if (NULL == pCurrentItem)
        return;

    BCMChannel *pChannel = pCurrentItem->GetMChannel();
    if (NULL == pChannel)
        return;

    QMenu menu;
    QAction *pModifyNameAction = NULL;

    pModifyNameAction = menu.addAction(tr("修改名称"));

    QAction *pSelectAction = menu.exec( QCursor::pos() );
    if (NULL == pSelectAction)
        return;

    if (pSelectAction == pModifyNameAction) {
        ModifyChannelName( pCurrentItem );
    }
}

void BCControlTreeWidget::ModifyChannelName(BCControlTreeWidgetItem *pCurrentItem)
{
    BCMChannel *pChannel = pCurrentItem->GetMChannel();
    if (NULL == pChannel)
        return;

    BCSignalName *pDlg = new BCSignalName(pChannel, BCCommon::Application());
    if (pDlg->exec() == QDialog::Accepted) {
        pCurrentItem->RefreshInputChannelName();
        //pCurrentItem->setText(0, pChannel->GetChannelBaseName()+pChannel->GetChannelName());

        // 同步物理信号源链表
        MainWindow *pApplication = BCCommon::Application();
        BCToolBar *pToolBar = pApplication->GetToolBar( MainWindow::SIGNALSOURCETOOLBAR );
        if (NULL == pToolBar)
            return;

        BCFaceWidget *pSignalSourceWidget = dynamic_cast<BCFaceWidget *>( pToolBar->widget() );
        if (NULL != pSignalSourceWidget)
            pSignalSourceWidget->RefreshChannelName( 1 );
    }
}

void BCControlTreeWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    // 双击槽函数的参数
    QTreeWidgetItem* item = this->currentItem();
    if (NULL == item)
        return;

    // 根据ID找到输入通道
    BCMChannel *pChannel = ((BCControlTreeWidgetItem*)item)->GetMChannel();
    if (NULL == pChannel)
        return;

    MainWindow *pApplication = BCCommon::Application();

    BCRoomWidget *pRoomWidget = pApplication->GetCurrentRoomWidget();
    BCRoomMainWidget *pRoomMainWidget = pRoomWidget->GetRoomMainWidget();

    // 如果是矩阵房间则不允许双击开窗
    if (4 == pRoomMainWidget->GetMRoom()->GetType())
        return;

    BCGroupDisplayWidget *pGroupDisplayWidget = pRoomMainWidget->GetCurrentGroupDisplay();
    if (NULL == pGroupDisplayWidget)
        return;

    BCMGroupDisplay *pMGroupDisplay = pGroupDisplayWidget->GetMGroupDisplay();
    if (NULL == pMGroupDisplay)
        return;
    QRectF rect;
    if ( !pChannel->IsHaveLastRect( pMGroupDisplay ) ) {
        int ArrangeX = pMGroupDisplay->GetArraySize().width();
        int ArrangeY = pMGroupDisplay->GetArraySize().height();

        rect.setLeft(0);
        rect.setTop(0);
        rect.setWidth(pMGroupDisplay->GetRect().width() / ArrangeX);
        rect.setHeight(pMGroupDisplay->GetRect().height()/ ArrangeY);
    } else {
        rect = pChannel->GetChannelLastRect( pMGroupDisplay );
    }

    // 向服务器发送winsize请求，以下设置放到服务器回复里处理
    pRoomMainWidget->AddSignalWindow(rect.left(), rect.top(), rect.width(), rect.height(), pGroupDisplayWidget, pChannel);

    pRoomMainWidget->RefreshSignalWindowTextDisplay();
}

void BCControlTreeWidget::RefreshToopTip()
{
    for (int i = 0; i < this->topLevelItemCount(); i++) {
        QTreeWidgetItem *pItem = this->topLevelItem(i);
        if (NULL == pItem)
            continue;

        BCControlTreeWidgetItem *pSelfItem = dynamic_cast<BCControlTreeWidgetItem *>( pItem );
        if (NULL == pSelfItem)
            continue;

        pSelfItem->RefreshToopTip();
    }
}

void BCControlTreeWidget::RefreshInputChannelName()
{
    for (int i = 0; i < this->topLevelItemCount(); i++) {
        QTreeWidgetItem *pItem = this->topLevelItem(i);
        if (NULL == pItem)
            continue;

        BCControlTreeWidgetItem *pSelfItem = dynamic_cast<BCControlTreeWidgetItem *>( pItem );
        if (NULL == pSelfItem)
            continue;

        pSelfItem->RefreshInputChannelName();
    }
}

bool BCControlTreeWidget::IsJointMatrixChannel(int chid)
{
    MainWindow *pMainWindow = BCCommon::Application();
    QList<BCMMatrix *> lstMatrix = pMainWindow->GetMMatrix();
    for (int i = 0; i < lstMatrix.count(); i++) {
        BCMMatrix *pMatrix = lstMatrix.at( i );
        if (1 != pMatrix->jointWithVP4000)
            continue;

        for (int j = 0; j < pMatrix->lstOutputNode.count(); j++) {
            sMatrixNode node = pMatrix->lstOutputNode.at( j );
            if (chid == node.jointWithVP4000ChannelID)
                return true;
        }
    }

    return false;
}

BCMChannel *BCControlTreeWidget::GetCurrentChannel()
{
    BCControlTreeWidgetItem *pItem = dynamic_cast<BCControlTreeWidgetItem *>( this->currentItem() );
    if (NULL == pItem)
        return NULL;

    return pItem->GetMChannel();
}

void BCControlTreeWidget::Refresh(int type, const QList<BCMChannel*> &lstChannels, BCControl *parent)
{
    // 清空链表
    this->clear();

    // 根据当前屏组的物理输入通道链表初始化
    QListIterator<BCMChannel*> it( lstChannels );
    while ( it.hasNext() ) {
        BCMChannel *pMChannel = it.next();
        if (pMChannel->GetChannelType() != type)
            continue;

        // 如果是联控占用的通道则不显示
        bool bJoint = false;
        if (-1 != BCCommon::g_nIsContainsMatrix) {
            if ( IsJointMatrixChannel(pMChannel->GetChannelID()) )
                bJoint = true;
                //continue;
        }

        this->addTopLevelItem(new BCControlTreeWidgetItem(pMChannel, bJoint, this));
    }

    // 如果没有数据则设置为空
    if (0 == this->topLevelItemCount()) {
        parent->SetTabVisibleFalse( type );
    }
}

//拖动
void BCControlTreeWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("inputChannel")) {
        event->accept();
    } else {
        event->ignore();
    }
}

// 只能进入一次
void BCControlTreeWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton) {
        QList<QString> listdata;
        // 单选
        BCControlTreeWidgetItem *pDragItem = dynamic_cast<BCControlTreeWidgetItem*>( this->itemAt( event->pos() ) );
        if (NULL != pDragItem) {
            listdata << "0";       //0 类型标识
            listdata << QString::number(pDragItem->GetMChannel()->GetChannelType());
            listdata << QString::number(pDragItem->GetMChannel()->GetChannelID());
        }

        if ( listdata.isEmpty() )
            return;

        QByteArray exData;
        QDataStream dataStream(&exData,QIODevice::WriteOnly);
        dataStream <<listdata;

        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData();
        mimeData->setData("inputChannel", exData);
        drag->setMimeData(mimeData);

        drag->exec(Qt::CopyAction);
    }

    QTreeWidget::mouseMoveEvent( event );
}

void BCControlTreeWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QTreeWidget::mouseReleaseEvent( event );
}

void BCControlTreeWidget::HighLightInputChannel(BCMChannel *pChannel)
{
    for (int i = 0; i < this->topLevelItemCount(); i++) {
        QTreeWidgetItem *pItem = this->topLevelItem(i);
        if (NULL == pItem)
            continue;

        BCControlTreeWidgetItem *pSelfItem = dynamic_cast<BCControlTreeWidgetItem *>( pItem );
        if (NULL == pSelfItem)
            continue;

        if (pSelfItem->GetMChannel() == pChannel)
            this->setCurrentItem( pItem );
    }
}
