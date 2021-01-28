#include "BCSignalTreeWidget.h"
#include <QDebug>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenu>
#include <QAction>
#include <QTreeWidgetItem>
#include "BCSignalName.h"
#include "../Model/BCMRoom.h"
#include "../Model/BCMGroupChannel.h"
#include "../Model/BCMChannel.h"
#include "../Common/BCCommon.h"
#include "../Model/BCMGroupDisplay.h"
#include "../Common/BCLocalServer.h"
#include "BCRoomWidget.h"
#include "BCGroupDisplayWidget.h"
#include "BCFaceWidget.h"
#include "BCToolBar.h"

BCSignalTreeWidgetItem::BCSignalTreeWidgetItem(BCMGroupChannel *pGroupChannel, QTreeWidget *parent)
    :QTreeWidgetItem(parent)
{
    m_pGroupChannel = pGroupChannel;
    m_pChannel = NULL;

    RefreshName( 0 );
}

BCSignalTreeWidgetItem::BCSignalTreeWidgetItem(BCMChannel *pChannel, QTreeWidgetItem *parent)
    :QTreeWidgetItem(parent)
{
    m_pChannel = pChannel;
    m_pGroupChannel = NULL;

    RefreshName( 1 );
}

void BCSignalTreeWidgetItem::RefreshName(int type)
{
    if (0 == type) {
        if (NULL == m_pGroupChannel)
            return;

        // 设置图片和文字
        this->setText(0, m_pGroupChannel->GetName());
    } else if (1 == type) {
        if (NULL == m_pChannel)
            return;

        // 设置图片和文字
        MainWindow *pApplication = BCCommon::Application();
        this->setText(0, m_pChannel->GetChannelBaseName()+m_pChannel->GetChannelName());
        this->setIcon(0, QIcon(pApplication->GetInputChannelIcon(m_pChannel->GetSignalSource())));
    }
}

bool BCSignalTreeWidgetItem::IsChannel()
{
    return (NULL == m_pChannel) ? false : true;
}

// ----------------------------------------------------------------------------------------------------------------------------------------

BCSignalTreeWidget::BCSignalTreeWidget(QWidget *parent)
    :QTreeWidget(parent)
{
    setHeaderHidden(true);

    setStyleSheet( "QTreeView::item:hover{background-color:rgb(0,255,0,50)}" "QTreeView::item:selected{background-color:rgb(255,0,0,100)}" );
}

BCSignalTreeWidget::~BCSignalTreeWidget()
{

}

void BCSignalTreeWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QTreeWidget::mouseReleaseEvent(event);
}

void BCSignalTreeWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    // 双击槽函数的参数
    QTreeWidgetItem* item = this->currentItem();
    if (NULL == item)
        return;

    // 根据ID找到输入通道
    BCMChannel *pChannel = ((BCSignalTreeWidgetItem*)item)->GetChannel();
    if (NULL == pChannel)
        return;

    MainWindow *pApplication = BCCommon::Application();

    BCRoomWidget *pRoomWidget = pApplication->GetCurrentRoomWidget();
    BCRoomMainWidget *pRoomMainWidget = pRoomWidget->GetRoomMainWidget();
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

void BCSignalTreeWidget::contextMenuEvent(QContextMenuEvent* event)
{
    // 如果当前没有item则没有右键菜单，普通item只是修改名称，IPV弹出设置界面
    BCSignalTreeWidgetItem *pCurrentItem = dynamic_cast<BCSignalTreeWidgetItem*>(this->itemAt(event->pos()));
    if (NULL == pCurrentItem)
        return;

    BCMChannel *pChannel = pCurrentItem->GetChannel();
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

void BCSignalTreeWidget::RefreshInputChannelName()
{
    for (int i = 0; i < this->topLevelItemCount(); i++) {
        BCSignalTreeWidgetItem *pGroupChannelItem = dynamic_cast<BCSignalTreeWidgetItem *>( this->topLevelItem( i ) );
        if (NULL == pGroupChannelItem)
            continue;

        pGroupChannelItem->RefreshName( 0 );
        for (int j = 0; j < pGroupChannelItem->childCount(); j++) {
            BCSignalTreeWidgetItem *pChannelItem = dynamic_cast<BCSignalTreeWidgetItem *>( pGroupChannelItem->child( j ) );
            if (NULL == pChannelItem)
                continue;

            pChannelItem->RefreshName( 1 );
        }
    }
}

void BCSignalTreeWidget::Refresh()
{
//    // 清空数据
//    this->clear();

//    // 循环添加自定义信号组
//    MainWindow *pMainWindow = BCCommon::Application();
//    QList<BCMGroupChannel *> lstGroupChannel = pMainWindow->GetGroupInputChannels();
//    QListIterator<BCMGroupChannel *> itGroup( lstGroupChannel );
//    while ( itGroup.hasNext() ) {
//        BCMGroupChannel *pGroupChannel = itGroup.next();

//        BCSignalTreeWidgetItem *pGroupChannelItem = new BCSignalTreeWidgetItem(pGroupChannel, this);
//        this->addTopLevelItem( pGroupChannelItem );

//        // 循环添加信号
//        QList<BCMChannel *> lstChannel = pGroupChannel->GetChannels();
//        QListIterator<BCMChannel *> it( lstChannel );
//        while ( it.hasNext() ) {
//            BCMChannel *pChannel = it.next();

//            BCSignalTreeWidgetItem *pChannelItem = new BCSignalTreeWidgetItem(pChannel, pGroupChannelItem);
//            pGroupChannelItem->addChild( pChannelItem );
//        }
//    }

//    this->expandAll();
}

void BCSignalTreeWidget::ModifyChannelName(BCSignalTreeWidgetItem *pCurrentItem)
{
    BCMChannel *pChannel = pCurrentItem->GetChannel();
    if (NULL == pChannel)
        return;

    BCSignalName *pDlg = new BCSignalName(pChannel, BCCommon::Application());
    if (pDlg->exec() == QDialog::Accepted) {
        pCurrentItem->setText(0, pChannel->GetChannelBaseName()+pChannel->GetChannelName());

        // 同步物理信号源链表
        MainWindow *pApplication = BCCommon::Application();
        BCToolBar *pToolBar = pApplication->GetToolBar( MainWindow::SIGNALSOURCETOOLBAR );
        if (NULL == pToolBar)
            return;

        BCFaceWidget *pSignalSourceWidget = dynamic_cast<BCFaceWidget *>( pToolBar->widget() );
        if (NULL != pSignalSourceWidget)
            pSignalSourceWidget->RefreshChannelName( 0 );
    }
}

