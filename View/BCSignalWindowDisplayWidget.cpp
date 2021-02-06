#include "BCSignalWindowDisplayWidget.h"
#include "ui_BCSignalWindowDisplayWidget.h"
#include <float.h>
#include <QAction>
#include <QMenu>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include "BCToolBar.h"
#include "BCRibbonMainToolBarAction.h"
#include "../Common/BCLocalServer.h"
#include "../Model/BCMChannel.h"
#include "../Model/BCMGroupDisplay.h"
#include "../Model/BCMRoom.h"
#include "../Model/BCMDisplay.h"
#include "../Setting/BCSettingSignalWindowPropertyDlg.h"
#include "BCFaceWidget.h"
#include "BCRoomWidget.h"
#include "BCGroupDisplayWidget.h"
#include "BCSingleDisplayVirtualWidget.h"
#include "BCDecoder.h"
#include "../Common/BCCommon.h"
#include "BCMMatrix.h"

BCSignalWindowDisplayWidget::BCSignalWindowDisplayWidget(BCGroupDisplayWidget *pGroupDisplayWidget, int x, int y, int w, int h, BCMChannel *pChannel, int winid, BCRoomMainWidget *pSignalWindowMgr, bool bSendCmd) :
    QWidget(pSignalWindowMgr),
    ui(new Ui::BCSignalWindowDisplayWidget)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_Hover, true);
    //this->setMouseTracking( true );
    m_eResizePos = UNRESIZE;

    m_rectFact = QRect(x, y, w, h);
    // 信号窗管理类
    m_pSignalWindowMgr = pSignalWindowMgr;
    // 记录归属组别
    m_pGroupDisplayWidget = pGroupDisplayWidget;
    // 记录输入通道
    m_pInputChannel = pChannel;
    m_nWindowID = winid;
    m_nCopyIndex = pChannel->getCopyIndex();    // 则设置复制窗口的索引

    // 默认不加锁，可以移动和缩放
    m_bLock = false;
    // 记录是否是全屏状态
    m_bFullScene = false;
    // 透明度
    m_transparent = 200;

    // 如果是IPV则显示矩形块
    ui->m_pGenaralBodyWidget->setVisible( 9 == m_pInputChannel->GetSignalSource() ? false : true );
    ui->m_pIPVBodyWidget->setVisible( 9 == m_pInputChannel->GetSignalSource() ? true : false );

    // 鼠标移动时共有属性
    m_x = 0.0;
    m_y = 0.0;
    m_bPress = false;

    ui->m_pLockBtn->setIcon(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsSignalWindowTitleUnLockButtonImagePath));
    ui->m_pCloseBtn->setIcon(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsSignalWindowTitleCloseButtonImagePath));

    // 全屏和恢复合并成一个按钮，默认显示全屏
    QString qsFullscreenPath = BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+(m_bFullScene?BCCommon::g_qsSignalWindowTitleUnFullScreenButtonImagePath:BCCommon::g_qsSignalWindowTitleFullScreenButtonImagePath);
    ui->m_pFullscreenBtn->setIcon(QIcon(qsFullscreenPath));
    ui->m_pWindowShowBtn->setVisible( false );

    MainWindow *pMainWindow = BCCommon::Application();
    if (pMainWindow->GetCurrentUser()->level > 2) {
        ui->m_pLockBtn->setVisible( false );
    }

    // 根据给定的位置比例重置矩形
    QRect virRect = m_pSignalWindowMgr->MapToVirtualRect(x, y, w, h);
    ResizeRect(virRect.left(), virRect.top(), virRect.width(), virRect.height(), bSendCmd, false, false);   // 使用实际坐标值

    // 矩阵模式下不可以修改窗口尺寸
    if (4 == m_pSignalWindowMgr->GetMRoom()->GetType()) {
        setAttribute(Qt::WA_Hover, false);
        ui->m_pLockBtn->setVisible( false );
        ui->m_pFullscreenBtn->setVisible( false );
        ui->m_pWindowShowBtn->setVisible( false );
    }
}

BCSignalWindowDisplayWidget::~BCSignalWindowDisplayWidget()
{
    delete ui;
}

void BCSignalWindowDisplayWidget::onConstructionTimeout()
{
    m_transparent = m_transparent+5;
    if (m_transparent >= 220)
        m_transparent = 220;
    else
        QTimer::singleShot(10, this, SLOT(onConstructionTimeout()));

    this->update();
}

//void BCSignalWindowDisplayWidget::closeEvent(QCloseEvent *e)
//{
//    // 淡出效果
//    QTimer::singleShot(10, this, SLOT(onDestructionTimeout()));
//    e->ignore();    // 添加为了保证不隐藏
//}

void BCSignalWindowDisplayWidget::onDestructionTimeout()
{
    m_transparent = m_transparent-5;
    if (m_transparent <= 0) {
        m_transparent = 0;
        this->deleteLater();
    } else
        QTimer::singleShot(10, this, SLOT(onDestructionTimeout()));

    this->repaint();
}

void BCSignalWindowDisplayWidget::ResizeRectBySorption()
{
    if (!BCCommon::g_bOpenSignalWindowSorption || m_bLock)
        return;

    QPoint ptLT = this->mapTo(m_pSignalWindowMgr, this->rect().topLeft());
    QPoint ptRB = this->mapTo(m_pSignalWindowMgr, QPoint(this->rect().left()+this->rect().width(), this->rect().top()+this->rect().height()));

    // 记录四个边界，scene坐标
    int dSorptionL = ptLT.x();
    int dSorptionT = ptLT.y();
    int dSorptionR = ptRB.x();
    int dSorptionB = ptRB.y();

    // 排列和分屏模式
    QSize roomArray = m_pSignalWindowMgr->GetRoomArray();
    int segX = 1;
    int segY = 1;
    switch ( m_pSignalWindowMgr->GetSegmentation() ) {
    case 1:
        break;
    case 4:
        segX = 2;
        segY = 2;
        break;
    case 6:
        segX = 3;
        segY = 2;
        break;
    case 8:
        segX = 4;
        segY = 2;
        break;
    case 9:
        segX = 3;
        segY = 3;
        break;
    case 12:
        segX = 4;
        segY = 3;
        break;
    case 16:
        segX = 4;
        segY = 4;
        break;
    default:
        break;
    }

    int virSingleWidth = m_pSignalWindowMgr->rect().width()/(roomArray.width()*segX);
    int virSingleHeight = m_pSignalWindowMgr->rect().height()/(roomArray.height()*segY);
    for (int i = 0; i < roomArray.width()*segX; i++) {
        for (int j = 0; j < roomArray.height()*segY; j++) {
            QRectF virRect = QRectF(virSingleWidth*i, virSingleHeight*j, virSingleWidth, virSingleHeight);

            // 外扩
            dSorptionL = ((dSorptionL > virRect.left()) && (dSorptionL < virRect.left()+BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.left() : dSorptionL;
            dSorptionL = ((dSorptionL > virRect.left()+virRect.width()) && (dSorptionL < virRect.left()+virRect.width()+BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.left()+virRect.width() : dSorptionL;
            dSorptionR = ((dSorptionR < virRect.left()) && (dSorptionR > virRect.left()-BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.left() : dSorptionR;
            dSorptionR = ((dSorptionR < virRect.left()+virRect.width()) && (dSorptionR > virRect.left()+virRect.width()-BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.left()+virRect.width() : dSorptionR;
            dSorptionT = ((dSorptionT > virRect.top()) && (dSorptionT < virRect.top()+BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.top() : dSorptionT;
            dSorptionT = ((dSorptionT > virRect.top()+virRect.height()) && (dSorptionT < virRect.top()+virRect.height()+BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.top()+virRect.height() : dSorptionT;
            dSorptionB = ((dSorptionB < virRect.top()) && (dSorptionB > virRect.top()-BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.top() : dSorptionB;
            dSorptionB = ((dSorptionB < virRect.top()+virRect.height()) && (dSorptionB > virRect.top()+virRect.height()-BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.top()+virRect.height() : dSorptionB;

            // 内缩
            dSorptionL = ((dSorptionL < virRect.left()) && (dSorptionL > virRect.left()-BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.left() : dSorptionL;
            dSorptionL = ((dSorptionL < virRect.left()+virRect.width()) && (dSorptionL > virRect.left()+virRect.width()-BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.left()+virRect.width() : dSorptionL;
            dSorptionR = ((dSorptionR > virRect.left()) && (dSorptionR < virRect.left()+BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.left() : dSorptionR;
            dSorptionR = ((dSorptionR > virRect.left()+virRect.width()) && (dSorptionR < virRect.left()+virRect.width()+BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.left()+virRect.width() : dSorptionR;
            dSorptionT = ((dSorptionT < virRect.top()) && (dSorptionT > virRect.top()-BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.top() : dSorptionT;
            dSorptionT = ((dSorptionT < virRect.top()+virRect.height()) && (dSorptionT > virRect.top()+virRect.height()-BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.top()+virRect.height() : dSorptionT;
            dSorptionB = ((dSorptionB > virRect.top()) && (dSorptionB < virRect.top()+BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.top() : dSorptionB;
            dSorptionB = ((dSorptionB > virRect.top()+virRect.height()) && (dSorptionB < virRect.top()+virRect.height()+BCCommon::g_nSignalWindowSorptionWidth)) ? virRect.top()+virRect.height() : dSorptionB;
        }
    }

    // 如果没有进行吸附，则宽度高度取当前尺寸，不进行转换，否则可能会有1像素的误差
    if ((dSorptionL == ptLT.x()) && (dSorptionT == ptLT.y()) && (dSorptionR == ptRB.x()) && (dSorptionB == ptRB.y())) {
        ResizeRect(dSorptionL, dSorptionT, this->rect().width(), this->rect().height(), true, true);
    } else {
        ResizeRect(dSorptionL, dSorptionT, dSorptionR-dSorptionL, dSorptionB-dSorptionT, true, true);
    }
}

void BCSignalWindowDisplayWidget::ResizeRect(int x, int y, int w, int h, bool bSendCmd, bool bReSend, bool bMapToFact)
{
    // 数据不合法直接返回
    if ((x==0) && (y==0) && (w==0) && (h==0))
        return;

    // 转换成实际坐标
    if (bSendCmd && bMapToFact) {
        m_rectFact = m_pSignalWindowMgr->MapToFactRect(x, y, w, h);
    }

    this->resize(w, h);
    this->move(x, y);

    // 刷新文字显示
    RefreshTextDisplay();

    // winsize通讯，吸附时最后一次坐标连续发送，以防指令丢失
    if ( bSendCmd ) {
        int nSendCount = bReSend ? 2 : 1;
        for (int i = 0; i < nSendCount; i++) {
            if ( bReSend ) {
                if (0 == BCCommon::g_nDeviceType) {     // vp2000间隔250ms
                    QThread::msleep( 250 );
                }
            }

            Winsize();
        }

        // 记录输入通道当前屏组上次开窗位置
        m_pInputChannel->SetChannelLastRect(m_pGroupDisplayWidget->GetMGroupDisplay(), QRectF(m_rectFact));
    }
}

void BCSignalWindowDisplayWidget::RefreshTextDisplay()
{
    // 修改header
    ui->m_pWindowTitleLabel->setText( m_pInputChannel->GetChannelBaseName()+m_pInputChannel->GetChannelName() );
    //ui->m_pWindowTitleLabel->setText( m_pInputChannel->GetChannelBaseName());//改变信号窗标题，去中文

    // 修改body
    QString qsWindowFlag = QObject::tr("窗口标识");
    QString qsWindowSignalSource = QObject::tr("信号源");
    ui->m_pWindowFlagLabel->setText(
                QString("%1:%2\r\n%3:%4")
                .arg(qsWindowFlag).arg( m_nWindowID+1 )
                .arg(qsWindowSignalSource).arg( m_pInputChannel->GetChannelID()+1 ) );
}

void BCSignalWindowDisplayWidget::SetLock(bool bLock)
{
    m_bLock = bLock;

    // 刷新图标
    QString qsIconPath = m_bLock ? BCCommon::g_qsSignalWindowTitleLockButtonImagePath : BCCommon::g_qsSignalWindowTitleUnLockButtonImagePath;
    ui->m_pLockBtn->setIcon( QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+qsIconPath) );
}

void BCSignalWindowDisplayWidget::SetFullScene(bool b)
{
    if ( m_bLock )
        return;

    // 赋值状态值
    m_bFullScene = b;
    if ( m_bFullScene ) {
        // 记录窗口尺寸
        m_rectBeforeFullScene = QRect(this->mapTo(m_pSignalWindowMgr, this->rect().topLeft()),
                                      this->mapTo(m_pSignalWindowMgr, this->rect().bottomRight()));
        QRectF rect = m_pSignalWindowMgr->rect();

        ResizeRect(rect.left(), rect.top(), rect.width(), rect.height(), true);
    } else {
        ResizeRect(m_rectBeforeFullScene.x(), m_rectBeforeFullScene.y(), m_rectBeforeFullScene.width(), m_rectBeforeFullScene.height(), true);
    }

    m_pSignalWindowMgr->SetSignalWindowTop( this );
}
// 缩放到单屏
void BCSignalWindowDisplayWidget::ScaleToSingleDisplay()
{
    if ( m_bLock )
        return;

    // 窗口左上角对应的虚拟矩形
    QPoint ptLT = this->mapTo(m_pSignalWindowMgr, this->rect().topLeft());

    // 外扩吸附值包含的矩形块
    QList<BCSingleDisplayVirtualWidget *> lstSorption = m_pSignalWindowMgr->GetSingleDisplayVirtualWidget();

    // 当外扩和内缩同时满足时，取外扩矩形
    QListIterator<BCSingleDisplayVirtualWidget *> it( lstSorption );
    while ( it.hasNext() ) {
        BCSingleDisplayVirtualWidget *pItem = it.next();

        // 虚拟矩形的尺寸
        QPoint virLT = pItem->mapTo(m_pSignalWindowMgr, pItem->rect().topLeft());
        //QPoint virRB = pItem->mapTo(m_pSignalWindowMgr, pItem->rect().bottomRight());
        QPoint virRB = pItem->mapTo(m_pSignalWindowMgr, QPoint(pItem->rect().left()+pItem->rect().width(), pItem->rect().top()+pItem->rect().height()));
        QRectF virRect = QRect(virLT, virRB);

        // 矩形框包含左上角继续
        if ( !virRect.contains( ptLT ) )
            continue;

        // 重置信号窗
        ResizeRect(virRect.x(), virRect.y(), virRect.width(), virRect.height(), true);

        break;
    }
}

// 缩放到所占屏（铺满当前屏）
void BCSignalWindowDisplayWidget::ScaleToOverlapDisplay()
{
    if ( m_bLock )
        return;

    // 窗口左上角对应的虚拟矩形
    QPoint ptLT = this->mapTo(m_pSignalWindowMgr, this->rect().topLeft());
    QPoint ptRB = this->mapTo(m_pSignalWindowMgr, this->rect().bottomRight());
    QRect sigRect(ptLT, ptRB);

    // 排列和分屏模式
    QSize roomArray = m_pSignalWindowMgr->GetRoomArray();
    int segX = 1;
    int segY = 1;
    switch ( m_pSignalWindowMgr->GetSegmentation() ) {
    case 1:
        break;
    case 4:
        segX = 2;
        segY = 2;
        break;
    case 6:
        segX = 3;
        segY = 2;
        break;
    case 8:
        segX = 4;
        segY = 2;
        break;
    case 9:
        segX = 3;
        segY = 3;
        break;
    case 12:
        segX = 4;
        segY = 3;
        break;
    case 16:
        segX = 4;
        segY = 4;
        break;
    default:
        break;
    }

    QList<QRect> lstSorption;
    int virSingleWidth = m_pSignalWindowMgr->rect().width()/(roomArray.width()*segX);
    int virSingleHeight = m_pSignalWindowMgr->rect().height()/(roomArray.height()*segY);
    for (int i = 0; i < roomArray.width()*segX; i++) {
        for (int j = 0; j < roomArray.height()*segY; j++) {
            lstSorption.append( QRect(virSingleWidth*i, virSingleHeight*j, virSingleWidth, virSingleHeight) );
        }
    }

    // 记录边界值
    double dL = DBL_MAX;
    double dR = DBL_MIN;
    double dT = DBL_MAX;
    double dB = DBL_MIN;

    // 当外扩和内缩同时满足时，取外扩矩形
    for (int i = 0; i < lstSorption.count(); i++) {
        QRect virRect = lstSorption.at( i );

        // 矩形框包含左上角继续
        if ( !sigRect.intersects( virRect ) )
            continue;

        dL = dL < virRect.left() ? dL : virRect.left();
        dR = dR > virRect.left()+virRect.width() ? dR : virRect.left()+virRect.width();
        dT = dT < virRect.top() ? dT : virRect.top();
        dB = dB > virRect.top()+virRect.height() ? dB : virRect.top()+virRect.height();
    }

    // 重置信号窗
    ResizeRect(dL, dT, dR-dL, dB-dT, true);
}

void BCSignalWindowDisplayWidget::SetSignalPosition(int type)
{
    bool bRes = false;
    switch (type) {
    case 0:
        bRes = m_pSignalWindowMgr->SetSignalWindowTop( this );
        break;
    case 1:
        bRes = m_pSignalWindowMgr->SetSignalWindowBottom( this );
        break;
    case 2:
        bRes = m_pSignalWindowMgr->SetSignalWindowMoveToTop( this );
        break;
    case 3:
        bRes = m_pSignalWindowMgr->SetSignalWindowMoveToBottom( this );
        break;
    case 4:
        bRes = m_pSignalWindowMgr->RemoveSignalWindowDisplayItem( this );
        break;
    default:
        break;
    }

    // 刷新内部信号窗文字显示，主要是叠放次序，如果叠放次序没有变化则没有必要刷新
    if ( bRes ) {
        m_pSignalWindowMgr->RefreshSignalWindowTextDisplay();
    }
}

void BCSignalWindowDisplayWidget::SetInputChannel(BCMChannel *pChannel)
{
    // 如果通道没有变化则直接返回
    if (pChannel == m_pInputChannel)
        return;

    // 重新赋值输入通道，并刷新内部显示
    m_pInputChannel->RemoveSignalWindowDisplayWidget( this );
    m_pInputChannel = pChannel;
    m_pInputChannel->AddSignalWindowDisplayWidget( this );
    RefreshTextDisplay();

    // 发送一次指令
    Winsize();

    // ??? 暂时置顶，硬件支持后去掉
    SetSignalPosition( 0 );
}

void BCSignalWindowDisplayWidget::Winsize()
{
    if ((NULL == m_pGroupDisplayWidget) || (NULL == m_pInputChannel))
        return;

    // 切换矩阵这里是空
    BCMGroupDisplay *pMGroupDisplay = m_pGroupDisplayWidget->GetMGroupDisplay();
    if (NULL == pMGroupDisplay)
        return;

    int groupid = pMGroupDisplay->GetGroupDisplayID();

    // 和服务器通讯
    if (BCLocalServer::Application()->isFullScreenMode()) {
        BCLocalServer::Application()->winsize(groupid, m_pInputChannel->GetChannelID(), m_nWindowID, m_rectFact.left(), m_rectFact.top(), m_rectFact.left()+m_rectFact.width(), m_rectFact.top()+m_rectFact.height(), m_pInputChannel->GetChannelType(), m_nCopyIndex);
    } else {
        auto matrix = BCCommon::Application()->GetMMatrix();
        if (nullptr == matrix)
        {
            return;
        }

        // matrix switch channel
        auto map = pMGroupDisplay->getDisplayRect(m_rectFact);
        foreach (auto id, map.keys()) {
            // 矩阵切换 m_pInputChannel->GetChannelID() -> id
            matrix->SetSwitch(m_pInputChannel->GetChannelID(), id);

            // 拼接开窗 map.value
            auto rect = map.value(id);
            BCLocalServer::Application()->winsize(0, m_pInputChannel->GetChannelID(), m_nWindowID,
                                                  rect.left(), rect.top(), rect.right(), rect.bottom(), 0, 0);
        }
    }
}

void BCSignalWindowDisplayWidget::SetSignalWindowProperty()
{
    BCSettingSignalWindowPropertyDlg *pDlg = new BCSettingSignalWindowPropertyDlg(this, BCCommon::Application());
    pDlg->exec();
}

void BCSignalWindowDisplayWidget::SetSignalWindowTitle(const QString &title)
{
    QString qsChannelSrcName = m_pInputChannel->GetChannelName();
    if (qsChannelSrcName != title) {
        m_pInputChannel->SetChannelName(title, true);

        // 通知工具栏刷新
        MainWindow *pApplication = BCCommon::Application();
        BCToolBar *pToolBar = pApplication->GetToolBar(MainWindow::SIGNALSOURCETOOLBAR);
        if (NULL != pToolBar) {
            BCFaceWidget* pWidget = dynamic_cast<BCFaceWidget *>( pToolBar->widget() );
            if (NULL != pWidget) {
                pWidget->Refresh( 0 );
            }
        }
    }

    ui->m_pWindowTitleLabel->setText(  m_pInputChannel->GetChannelBaseName()+title );
}

QString BCSignalWindowDisplayWidget::GetSignalWindowTitle()
{
    return ui->m_pWindowTitleLabel->text();
}

void BCSignalWindowDisplayWidget::SetSignalWindowResize(int x, int y, int w, int h, bool bSendCmd)
{
    m_rectFact = QRect(x, y, w, h);

    QRect virRect = m_pSignalWindowMgr->MapToVirtualRect(x, y, w, h);
    ResizeRect(virRect.x(), virRect.y(), virRect.width(), virRect.height(), bSendCmd, false, false);
}

void BCSignalWindowDisplayWidget::contextMenuEvent(QContextMenuEvent *e)
{
    // 构造菜单
    QMenu menu;
    BCCommon::SetSystemFont( &menu );

    auto chMenu = menu.addMenu(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/checkupdate.png"), tr("信号源"));
    QHash<QAction*, BCMChannel*> hash;
    foreach (auto channel, BCCommon::Application()->GetInputChannels()) {
        auto action = chMenu->addAction(QIcon(BCCommon::Application()->GetInputChannelIcon(channel->GetSignalSource())), channel->GetChannelName());
        hash.insert(action, channel);
    }
    QAction *pAttribute = menu.addAction(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsSignalWindowMenuActionAttributeIconPath), QObject::tr("窗口参数"));
    QAction *pScaleToOverlapDisplay = menu.addAction(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsSignalWindowMenuActionScaleToOverlapDisplayIconPath), QObject::tr("铺满当前屏"));
    QAction *pScaleToGroupDisplay = menu.addAction(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsSignalWindowMenuActionScaleToAllDisplayIconPath), QObject::tr("铺满整屏"));
    QAction *pClose = menu.addAction(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsSignalWindowMenuActionCloseIconPath), QObject::tr("关闭"));

    // 返回选择的action
    QAction *pSelectedAction = menu.exec( this->mapToGlobal(e->pos()) );
    if (NULL != pSelectedAction) {
        foreach (auto action, hash.keys()) {
            if (pSelectedAction == action) {
                SetInputChannel(hash.value(action));
            }
        }
        // 缩放到所占屏（铺满当前屏）
        if (pSelectedAction == pScaleToOverlapDisplay) {
            ScaleToOverlapDisplay();
        }
        // 全屏
        if (pSelectedAction == pScaleToGroupDisplay) {
            SetFullScene( true );
        }
        // 关闭
        if (pSelectedAction == pClose) {
            SetSignalPosition( 4 );
        }
        // 属性
        if (pSelectedAction == pAttribute) {
            SetSignalWindowProperty();
        }
    }

    e->accept();
}

void BCSignalWindowDisplayWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    this->ScaleToOverlapDisplay();

    QWidget::mouseDoubleClickEvent( event );
}

void BCSignalWindowDisplayWidget::ServerRequestControlResult()
{
    m_bPress = true;

    // 置顶
    SetSignalPosition(0);
}

void BCSignalWindowDisplayWidget::ServerWinSwitchResult(bool b)
{
    m_pSignalWindowMgr->ServerWinSwitchResult(b, this);
}

void BCSignalWindowDisplayWidget::mousePressEvent(QMouseEvent *event)
{
    // 矩阵不可移动
    if (4 == m_pSignalWindowMgr->GetMRoom()->GetType()) {
        QWidget::mousePressEvent(event);
        return;
    }

    // 点击单个屏幕时
    if ( !m_bLock ) {
        if(event->button()== Qt::LeftButton){

            // 相对于scene左上角的坐标，缩放不影响坐标
            m_x = event->pos().x();
            m_y = event->pos().y();

            // 向服务器发送winsize请求，以下设置放到服务器回复里处理
                m_bPress = true;

                // 置顶
                SetSignalPosition(0);
        }
    }

    QWidget::mousePressEvent(event);
}

void BCSignalWindowDisplayWidget::mouseMoveEvent(QMouseEvent * event)
{
    // 点击单个屏幕时
    if (!m_bLock) {
        if (m_bPress) {
            // 矩形左上角坐标，缩放无影响
            int nOffsetX = event->pos().x() - m_x;
            int nOffsetY = event->pos().y() - m_y;

            // 转换成窗口管理类坐标
            QPoint ptLT = this->mapTo(m_pSignalWindowMgr, this->rect().topLeft());
            QPoint ptRB = this->mapTo(m_pSignalWindowMgr, this->rect().bottomRight());
            QPoint ptCurrentPos = this->mapTo(m_pSignalWindowMgr, event->pos());

            // 窗口管理类矩形框
            QRect parentRect = m_pSignalWindowMgr->rect();  // 父类坐标系坐标

            bool bSendCmd = true;

            // 需要考虑缩放比例，需要考虑到拖动范围
            switch (m_eResizePos) {
            case UNRESIZE: {
                int l = ptLT.x()+nOffsetX;
                int t = ptLT.y()+nOffsetY;

                // 允许拖动最小范围
                l = (l <= parentRect.left()) ? parentRect.left() : l;
                t = (t <= parentRect.top()) ? parentRect.top() : t;

                // 允许拖动最大范围
                l = (l >= parentRect.right()-this->rect().width()+1) ? parentRect.right()-this->rect().width()+1 : l;
                t = (t >= parentRect.bottom()-this->rect().height()+1) ? parentRect.bottom()-this->rect().height()+1 : t;

                l = (l < 0) ? 0 : l;
                t = (t < 0) ? 0 : t;

                // 2000时窗口漫游需要间隔200MS
//                if (0 == BCCommon::g_nDeviceType) {
//                    bSendCmd = false;
//                }

                ResizeRect(l, t, this->rect().width(), this->rect().height(), bSendCmd);
            }
                break;
            case RESIZELT: {
                // 最大最小值限定
                nOffsetX = (ptLT.x()+nOffsetX <= parentRect.left()) ? 0 : nOffsetX;
                nOffsetX = (ptLT.x()+nOffsetX >= ptRB.x()-BCCommon::g_nMinResizeSignalWindowSizeW) ? 0 : nOffsetX;

                nOffsetY = (ptLT.y()+nOffsetY <= parentRect.top()) ? 0 : nOffsetY;
                nOffsetY = (ptLT.y()+nOffsetY >= ptRB.y()-BCCommon::g_nMinResizeSignalWindowSizeH) ? 0 : nOffsetY;

                ResizeRect(ptLT.x()+nOffsetX, ptLT.y()+nOffsetY, this->rect().width()-nOffsetX, rect().height()-nOffsetY, bSendCmd);
                }
                break;
            case RESIZEL: {
                // 最大最小值限定
                nOffsetX = (ptLT.x()+nOffsetX <= parentRect.left()) ? 0 : nOffsetX;
                nOffsetX = (ptLT.x()+nOffsetX >= ptRB.x()-BCCommon::g_nMinResizeSignalWindowSizeW) ? 0 : nOffsetX;

                ResizeRect(ptLT.x()+nOffsetX, ptLT.y(), this->rect().width()-nOffsetX, rect().height(), bSendCmd);
                }
                break;
            case RESIZELB: {
                // 最大最小值限定
                nOffsetX = (ptLT.x()+nOffsetX <= parentRect.left()) ? 0 : nOffsetX;
                nOffsetX = (ptLT.x()+nOffsetX >= ptRB.x()-BCCommon::g_nMinResizeSignalWindowSizeW) ? 0 : nOffsetX;

                ptCurrentPos.setY( (ptCurrentPos.y() >= parentRect.bottom()) ? parentRect.bottom() : ptCurrentPos.y() );
                ptCurrentPos.setY( (ptCurrentPos.y() <= ptLT.y()+BCCommon::g_nMinResizeSignalWindowSizeH) ? ptLT.y()+BCCommon::g_nMinResizeSignalWindowSizeH : ptCurrentPos.y() );

                ResizeRect(ptLT.x()+nOffsetX, ptLT.y(), this->rect().width()-nOffsetX, ptCurrentPos.y()-ptLT.y(), bSendCmd);
                }
                break;
            case RESIZET: {
                nOffsetY = (ptLT.y()+nOffsetY <= parentRect.top()) ? 0 : nOffsetY;
                nOffsetY = (ptLT.y()+nOffsetY >= ptRB.y()-BCCommon::g_nMinResizeSignalWindowSizeH) ? 0 : nOffsetY;

                ResizeRect(ptLT.x(), ptLT.y()+nOffsetY, this->rect().width(), rect().height()-nOffsetY, bSendCmd);
                }
                break;
            case RESIZEB: {
                ptCurrentPos.setY( (ptCurrentPos.y() >= parentRect.bottom()) ? parentRect.bottom() : ptCurrentPos.y() );
                ptCurrentPos.setY( (ptCurrentPos.y() <= ptLT.y()+BCCommon::g_nMinResizeSignalWindowSizeH) ? ptLT.y()+BCCommon::g_nMinResizeSignalWindowSizeH : ptCurrentPos.y() );

                ResizeRect(ptLT.x(), ptLT.y(), this->rect().width(), ptCurrentPos.y()-ptLT.y(), bSendCmd);
                }
                break;
            case RESIZERT: {
                // 最大最小值限定
                ptCurrentPos.setX( (ptCurrentPos.x() >= parentRect.right()) ? parentRect.right() : ptCurrentPos.x() );
                ptCurrentPos.setX( (ptCurrentPos.x() <= ptLT.x()+BCCommon::g_nMinResizeSignalWindowSizeW) ? ptLT.x()+BCCommon::g_nMinResizeSignalWindowSizeW : ptCurrentPos.x() );

                nOffsetY = (ptLT.y()+nOffsetY <= parentRect.top()) ? 0 : nOffsetY;
                nOffsetY = (ptLT.y()+nOffsetY >= ptRB.y()-BCCommon::g_nMinResizeSignalWindowSizeH) ? 0 : nOffsetY;

                ResizeRect(ptLT.x(), ptLT.y()+nOffsetY, ptCurrentPos.x()-ptLT.x(), rect().height()-nOffsetY, bSendCmd);
                }
                break;
            case RESIZER: {
                // 最大最小值限定
                ptCurrentPos.setX( (ptCurrentPos.x() >= parentRect.right()) ? parentRect.right() : ptCurrentPos.x() );
                ptCurrentPos.setX( (ptCurrentPos.x() <= ptLT.x()+BCCommon::g_nMinResizeSignalWindowSizeW) ? ptLT.x()+BCCommon::g_nMinResizeSignalWindowSizeW : ptCurrentPos.x() );

                ResizeRect(ptLT.x(), ptLT.y(), ptCurrentPos.x()-ptLT.x(), rect().height(), bSendCmd);
                }
                break;
            case RESIZERB: {
                // 最大最小值限定
                ptCurrentPos.setX( (ptCurrentPos.x() >= parentRect.right()) ? parentRect.right() : ptCurrentPos.x() );
                ptCurrentPos.setX( (ptCurrentPos.x() <= ptLT.x()+BCCommon::g_nMinResizeSignalWindowSizeW) ? ptLT.x()+BCCommon::g_nMinResizeSignalWindowSizeW : ptCurrentPos.x() );

                ptCurrentPos.setY( (ptCurrentPos.y() >= parentRect.bottom()) ? parentRect.bottom() : ptCurrentPos.y() );
                ptCurrentPos.setY( (ptCurrentPos.y() <= ptLT.y()+BCCommon::g_nMinResizeSignalWindowSizeH) ? ptLT.y()+BCCommon::g_nMinResizeSignalWindowSizeH : ptCurrentPos.y() );

                ResizeRect(ptLT.x(), ptLT.y(), ptCurrentPos.x()-ptLT.x(), ptCurrentPos.y()-ptLT.y(), bSendCmd);
                }
                break;
            default:
                break;
            }
        }
    }

    QWidget::mouseMoveEvent(event);
}

bool BCSignalWindowDisplayWidget::event(QEvent *event)
{
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave
            || event->type() == QEvent::HoverMove) {
        QHoverEvent* pHoverEvent = static_cast<QHoverEvent *>(event);

        // 锁定和点击情况下不做处理
        if ( m_bLock || m_bPress )
            return false;

        // 判断是否需要调整窗口大小，下面为距离左上角的值
        int nltx = pHoverEvent->pos().x() - rect().x();
        int nlty = pHoverEvent->pos().y() - rect().y();

        // x在修改范围时
        if (nltx <= BCCommon::g_nMaxSizeOfModifyRect) {
            if (nlty <= BCCommon::g_nMaxSizeOfModifyRect) {
                // ↖
                m_eResizePos = RESIZELT;
                setCursor(Qt::SizeFDiagCursor);
            } else if (qAbs(nlty-rect().height()) <= BCCommon::g_nMaxSizeOfModifyRect) {
                // ↙
                m_eResizePos = RESIZELB;
                setCursor(Qt::SizeBDiagCursor);
            } else {
                // ←
                m_eResizePos = RESIZEL;
                setCursor(Qt::SizeHorCursor);
            }
        } else if (qAbs(nltx-rect().width()) <= BCCommon::g_nMaxSizeOfModifyRect) {
            if (nlty <= BCCommon::g_nMaxSizeOfModifyRect) {
                // ↗
                m_eResizePos = RESIZERT;
                setCursor(Qt::SizeBDiagCursor);
            } else if (qAbs(nlty-rect().height()) <= BCCommon::g_nMaxSizeOfModifyRect) {
                // ↘
                m_eResizePos = RESIZERB;
                setCursor(Qt::SizeFDiagCursor);
            } else {
                // →
                m_eResizePos = RESIZER;
                setCursor(Qt::SizeHorCursor);
            }
        } else {
            if (nlty <= BCCommon::g_nMaxSizeOfModifyRect) {
                // ↑
                m_eResizePos = RESIZET;
                setCursor(Qt::SizeVerCursor);
            } else if (qAbs(nlty-rect().height()) <= BCCommon::g_nMaxSizeOfModifyRect) {
                // ↓
                m_eResizePos = RESIZEB;
                setCursor(Qt::SizeVerCursor);
            } else {
                // 不拉伸
                m_eResizePos = UNRESIZE;
                setCursor(Qt::ArrowCursor);
            }
        }
    }

    return QWidget::event(event);
}

void BCSignalWindowDisplayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    // 点击单个屏幕时
    if (!m_bLock) {
        if ( m_bPress ) {
            // 如果开启吸附则重置矩形尺寸
            if (BCCommon::g_bOpenSignalWindowSorption)
                ResizeRectBySorption();

            m_bPress = false;
        }
    }

    QWidget::mouseReleaseEvent(event);
}

void BCSignalWindowDisplayWidget::on_m_pLockBtn_clicked()
{
    this->SetLock( !m_bLock );
}

void BCSignalWindowDisplayWidget::on_m_pFullscreenBtn_clicked()
{
    if ( m_bLock )
        return;

    m_bFullScene = !m_bFullScene;
    QString qsFullscreenPath = BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+(m_bFullScene?BCCommon::g_qsSignalWindowTitleUnFullScreenButtonImagePath:BCCommon::g_qsSignalWindowTitleFullScreenButtonImagePath);
    ui->m_pFullscreenBtn->setIcon(QIcon(qsFullscreenPath));

    SetFullScene( m_bFullScene );
}

void BCSignalWindowDisplayWidget::on_m_pWindowShowBtn_clicked()
{
    SetFullScene( false );
}

void BCSignalWindowDisplayWidget::on_m_pCloseBtn_clicked()
{
    m_pSignalWindowMgr->RemoveSignalWindowDisplayItem( this );
}

void BCSignalWindowDisplayWidget::paintEvent(QPaintEvent */*e*/)
{
    QPainter painter(this);

    // 设置边框
    painter.setPen(QPen(Qt::black,1,Qt::SolidLine));

    // 绘制整体
    painter.setBrush( QBrush(/*m_bEcho ? Qt::black :*/ QColor(131, 180, 235, m_transparent)));
    painter.drawRect( QRect(rect().left(), rect().top(), rect().width()-1, rect().height()-1) );

    // 绘制header
    painter.setBrush( QBrush(QColor(250, 141, 69, m_transparent)));
    QRect rectHeader = ui->m_pHeaderWidget->rect();
    painter.drawRect( QRect(rectHeader.left(), rectHeader.top(), rectHeader.width()+5, rectHeader.height()) );
}
