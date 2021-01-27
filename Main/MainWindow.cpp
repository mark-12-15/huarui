#include "MainWindow.h"

#include <QApplication>
#include <QGraphicsView>
#include "../Common/BCLocalServer.h"
#include "../Common/BCOutsideInterfaceServer.h"
#include "../View/BCToolBar.h"
#include "../View/BCFaceWidget.h"
#include "../View/BCControl.h"
#include "../View/BCRibbonMainToolBar.h"
#include "../View/BCRibbonBar.h"
#include "../View/BCRibbonMainToolBarAction.h"
#include "../View/BCMatrixRoomWidget.h"
#include "../Model/BCMChannel.h"
#include "../Model/BCMGroupDisplay.h"
#include "../Model/BCMDisplay.h"
#include "../Model/BCMRoom.h"
#include "../Model/BCMGroupScene.h"
#include "../Model/BCMWindowScene.h"
#include "../Model/BCMGroupChannel.h"
#include "../Model/BCMGroupScene.h"
#include "../Model/BCMMatrix.h"
#include "../Setting/BCLoginDlg.h"
#include "../Setting/BCSettingOutOfDateDlg.h"
#include "../View/BCRoomWidget.h"
#include "../Setting/BCSettingDebugDlg.h"
#include "../View/BCSignalWindowDisplayWidget.h"
#include "../View/BCScreenName.h"
#include "../Common/BCCommon.h"

MainWindow::MainWindow(QWidget *parent) :
    Qtitan::RibbonMainWindow(parent)
{
    this->setAttribute(Qt::WA_DeleteOnClose);

    // 设置标题和图标
    this->setWindowIcon( QIcon(BCCommon::g_qsImageFilePrefix + BCCommon::g_qsApplicationIcon) );

    // 初始化变量
    m_nQuitFlag                 = 0;
    m_pSystemUser               = NULL;
    m_pOutOfDateDlg             = NULL;
    m_pStatusLabel              = NULL;
    m_pOutOfDateTimer           = NULL;
    m_pDebugDlg                 = NULL;
    m_timeOfPreview             = QTime::currentTime().addMSecs( -10*1000 );
    m_bWindowSceneSetSwitch     = false;
    InitSignalSourceMap();

    // 自定义ribbonBar，目的是屏蔽ribbon本身右键菜单
    BCRibbonBar *pRibbonBar = new BCRibbonBar();
    this->setRibbonBar( pRibbonBar );

    m_ribbonStyle = qobject_cast<Qtitan::RibbonStyle*>(qApp->style());  // 风格根据QApplication内QStyle设置
    m_ribbonStyle->setTheme(OfficeStyle::Office2007Blue);   // 设置默认风格
    //ribbonBar()->setFrameThemeEnabled();    // 关闭Qt MainWindow原来的边框
    ribbonBar()->setTitleBarVisible(false);     // 隐藏ribbon的titlebar
    createOptions();

    // 设置默认字体尺寸
    QFont fnt = m_ribbonStyle->font(ribbonBar());
    fnt.setFamily( BCCommon::g_qsDefaultFontFamily );
    fnt.setPixelSize( BCCommon::g_nDefaultFontPixelSize );
    ribbonBar()->setFont(fnt);

    // 自定义主工具条，用来生成按钮
    m_pMainToolBar = new BCRibbonMainToolBar();

    // 构造房间
    m_pTabWidgetRooms = new QTabWidget( this );
    setCentralWidget( m_pTabWidgetRooms );
    // 安装事件过滤器
    m_pTabWidgetRooms->installEventFilter( this );              // 修改名称和删除场景组菜单

    // 设置tabwidget风格
    m_pTabWidgetRooms->setStyleSheet( BCCommon::g_qsTabWidgetStyle );
    BCCommon::SetSystemFont( m_pTabWidgetRooms );

    // 初始化托盘
    InitTrayIcon();

    // 31秒定时器，提示设备是否过期
    m_pOutOfDateTimer = new QTimer();
    m_pOutOfDateTimer->setInterval( 1000*31 );
    connect(m_pOutOfDateTimer, SIGNAL(timeout()), this, SLOT(onTimeOutOfOutOfDate()));
    m_pOutOfDateTimer->start();

    // 默认尺寸，否则初始化全屏时不好使
    //this->resize(1024, 768);
}

MainWindow::~MainWindow()
{
    // 保存默认场景，读取默认场景不需要保存场景
    if (!BCCommon::g_bConnectWithServer && (0 == BCCommon::g_nDeviceType)) {
        //
    } else {
#ifdef CHANGCHUNPOWER
        QListIterator<BCMRoom *> it( m_lstRooms );
        while ( it.hasNext() ) {
            BCMRoom *pRoom = it.next();

            BCMWindowScene *pWindowScene = pRoom->GetDefaultWindowScene();
            if (NULL == pWindowScene)
                continue;

            pWindowScene->Update();
        }
#endif
    }

    if (NULL != m_pDebugDlg) {
        delete m_pDebugDlg;
        m_pDebugDlg = NULL;
    }

    //*********************************************************************************************************************************
    m_pOutOfDateTimer->stop();
    m_pOutOfDateTimer->deleteLater();

    if (NULL != m_pOutOfDateDlg) {
        delete m_pOutOfDateDlg;
        m_pOutOfDateDlg = NULL;
    }

    if (NULL != m_pSystemUser) {
        delete m_pSystemUser;
        m_pSystemUser = NULL;
    }

    // 析构ui类
    // delete toolbar list
    while (!m_lstToolBars.isEmpty())
        delete m_lstToolBars.takeFirst();

    // delete rooms
    delete m_pTabWidgetRooms;
    m_pTabWidgetRooms = NULL;

    // delete trayicon
    m_pTrayIcon->deleteLater();

    // clear actions
    delete m_pMainToolBar;
    m_pMainToolBar = NULL;

    //*********************************************************************************************************************************
    // 析构数据类

    // 物理输入通道
    while ( !m_lstInputChannels.isEmpty() )
        delete m_lstInputChannels.takeFirst();
    // 房间链表
    while ( !m_lstRooms.isEmpty() )
        delete m_lstRooms.takeFirst();
    // 清空矩阵，对象不需要清空
    while ( !m_lstMatrix.isEmpty() )
        delete m_lstMatrix.takeFirst();

    // 通讯接口
    stop();

    // 析构外部端口服务器
    BCOutsideInterfaceServer::Destroy();
    // 析构本地服务器
    BCLocalServer::Destroy();
}
bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_pTabWidgetRooms) {
        if (e->type() == QEvent::ContextMenu) {
            QContextMenuEvent *pMenuEvent = static_cast<QContextMenuEvent *>( e );
            int tabIndex = m_pTabWidgetRooms->tabBar()->tabAt( pMenuEvent->pos() );
            if (-1 != tabIndex) {
                BCRoomWidget *pRoom = dynamic_cast<BCRoomWidget *>( m_pTabWidgetRooms->widget(tabIndex) );
                if (NULL != pRoom) {
                    BCRoomMainWidget *pMainWidget = pRoom->GetRoomMainWidget();
                    if (NULL != pMainWidget) {
                        QMenu menu;
                        QAction *pModifyNameAction = menu.addAction(tr("修改名称"));
                        QAction *pSelectAction = menu.exec( QCursor::pos() );
                        if (pModifyNameAction == pSelectAction) {
                            BCMRoom *pMRoom = pMainWidget->GetMRoom();
                            BCScreenName dlg(pMRoom->GetRoomName(), this);
                            if(dlg.exec() == QDialog::Accepted) {
                                // data
                                pMRoom->SetRoomName(dlg.name, true);

                                // ui
                                m_pTabWidgetRooms->setTabText(tabIndex, dlg.name);
                            }
                        }
                    }
                }
                BCMatrixRoomWidget *pMatrixRoom = dynamic_cast<BCMatrixRoomWidget *>( m_pTabWidgetRooms->widget(tabIndex) );
                if (NULL != pMatrixRoom) {
                    QMenu menu;
                    QAction *pModifyNameAction = menu.addAction(tr("修改名称"));
                    QAction *pSelectAction = menu.exec( QCursor::pos() );
                    if (pModifyNameAction == pSelectAction) {
                        BCMMatrix *pMatrix = pMatrixRoom->GetMMatrix();
                        BCScreenName dlg(pMatrix->name, this);
                        if(dlg.exec() == QDialog::Accepted) {
                            // data
                            pMatrix->SetMatrixRoomName( dlg.name );

                            // ui
                            m_pTabWidgetRooms->setTabText(tabIndex, dlg.name);
                        }
                    }
                }
            }
        }
    }

    return QWidget::eventFilter(obj, e);
}

QString MainWindow::GetPreviewIPByControlIP(QString ip)
{
    return m_mapControlPreview.value(ip, "");
}
void MainWindow::InitSignalSourceMap()
{
    // 初始化信号源使用的图片
    m_mapSignalSourceIconPath.clear();
    m_mapSignalSourceIconPath.insert(INPUTCHANNELSSIGSRC, BCCommon::g_qsApplicationDefaultStylePath+QString("/signalsourceControl16x16.png"));
    m_mapSignalSourceIconPath.insert(CUSTINPUTCHANNELSSIGSRC, BCCommon::g_qsApplicationDefaultStylePath+QString("/signalsourceSignal16x16.png"));
    m_mapSignalSourceIconPath.insert(VIDEOCONTROLSIGSRC, BCCommon::g_qsApplicationDefaultStylePath+QString("/signalsourceVideoControl16x16.png"));
    m_mapSignalSourceIconPath.insert(SYSPLANSIGSRC, BCCommon::g_qsApplicationDefaultStylePath+QString("/signalsourceSystemplan16x16.png"));
    m_mapSignalSourceIconPath.insert(DECODERSIGSRC, BCCommon::g_qsApplicationDefaultStylePath+QString("/signalsourceDecoder16x16.png"));
    m_mapSignalSourceIconPath.insert(CATCHNETDISPLAYSIGSRC, BCCommon::g_qsApplicationDefaultStylePath+QString("/signalsourceCutScreen16x16.png"));
    m_mapSignalSourceIconPath.insert(MATRIXPANELSIGSRC, BCCommon::g_qsApplicationDefaultStylePath+QString("/signalsourceMatrix16x16.png"));
    m_mapSignalSourceIconPath.insert(MATRIXINPUTSIGSRC, BCCommon::g_qsApplicationDefaultStylePath+QString("/signalsourceMatrixInput16x16.png"));
    m_mapSignalSourceIconPath.insert(MATRIXSWAPSIGSRC, BCCommon::g_qsApplicationDefaultStylePath+QString("/signalsourceMatrixCut16x16.png"));
    m_mapSignalSourceIconPath.insert(WINDOWSCENESIGSRC, BCCommon::g_qsApplicationDefaultStylePath+QString("/signalsourceScene16x16.png"));

    // 初始化信号源使用的名称
    m_mapSignalSourceName.clear();
    m_mapSignalSourceName.insert(INPUTCHANNELSSIGSRC, tr("输入源"));
    m_mapSignalSourceName.insert(CUSTINPUTCHANNELSSIGSRC, tr("信号源列表"));
    m_mapSignalSourceName.insert(VIDEOCONTROLSIGSRC, tr("录像播放控制"));
    m_mapSignalSourceName.insert(SYSPLANSIGSRC, tr("系统预案"));
    m_mapSignalSourceName.insert(DECODERSIGSRC, tr("摄像头列表"));
    m_mapSignalSourceName.insert(CATCHNETDISPLAYSIGSRC, tr("网络抓屏列表"));
    m_mapSignalSourceName.insert(MATRIXPANELSIGSRC, tr("矩阵面板"));
    m_mapSignalSourceName.insert(MATRIXINPUTSIGSRC, tr("矩阵输入列表"));
    m_mapSignalSourceName.insert(MATRIXSWAPSIGSRC, tr("矩阵切换"));
    m_mapSignalSourceName.insert(WINDOWSCENESIGSRC, tr("场景列表"));

    // 初始化输入通道使用的图片
    m_mapInputChannelIconPath.clear();
    m_mapInputChannelIconPath.insert(0, BCCommon::g_qInputChannelPath+QString("VGA.png"));
    m_mapInputChannelIconPath.insert(1, BCCommon::g_qInputChannelPath+QString("DVI.png"));
    m_mapInputChannelIconPath.insert(2, BCCommon::g_qInputChannelPath+QString("CVBS.png"));
    m_mapInputChannelIconPath.insert(3, BCCommon::g_qInputChannelPath+QString("YPbPr.png"));
    m_mapInputChannelIconPath.insert(4, BCCommon::g_qInputChannelPath+QString("HDMI.png"));
    m_mapInputChannelIconPath.insert(5, BCCommon::g_qInputChannelPath+QString("SV.png"));
    m_mapInputChannelIconPath.insert(6, BCCommon::g_qInputChannelPath+QString("HDVI.png"));
    m_mapInputChannelIconPath.insert(7, BCCommon::g_qInputChannelPath+QString("SDI.png"));
    m_mapInputChannelIconPath.insert(8, BCCommon::g_qInputChannelPath+QString("DP.png"));
    m_mapInputChannelIconPath.insert(9, BCCommon::g_qInputChannelPath+QString("IPV.png"));
    m_mapInputChannelIconPath.insert(10, BCCommon::g_qInputChannelPath+QString("HDBaseT.png"));
    m_mapInputChannelIconPath.insert(11, BCCommon::g_qInputChannelPath+QString("FIBER.png"));
    m_mapInputChannelIconPath.insert(20, BCCommon::g_qInputChannelPath+QString("JointMatrix.png"));
}

void MainWindow::ClearRoom()
{
    // 取消修改房间信号槽
    disconnect(m_pTabWidgetRooms, SIGNAL(currentChanged(int)), this, SLOT(onCurrentRoomChanged(int)));

    // 删除内部矩形
    for (int i = 0; i < m_pTabWidgetRooms->count(); i++) {
        // 房间可能是拼接或者是矩阵
        BCRoomWidget *pRoom = dynamic_cast<BCRoomWidget *>( m_pTabWidgetRooms->widget(i) );
        if (NULL != pRoom) {
            delete pRoom;
            pRoom = NULL;
        }
        BCMatrixRoomWidget *pMatrixRoom = dynamic_cast<BCMatrixRoomWidget *>( m_pTabWidgetRooms->widget(i) );
        if (NULL != pMatrixRoom) {
            delete pMatrixRoom;
            pMatrixRoom = NULL;
        }
    }

    // 清空tabwidget
    m_pTabWidgetRooms->clear();
}

void MainWindow::InitRoom()
{
    // 初始化拼接房间
    QListIterator<BCMRoom *> itRoom( m_lstRooms );
    while ( itRoom.hasNext() ) {
        BCMRoom *pMRoom = itRoom.next();
        if (NULL == pMRoom)
            continue;

        // 判断是否是Widget绘制
        BCRoomWidget *pRoomWidget = new BCRoomWidget(pMRoom, m_pTabWidgetRooms);
        m_pTabWidgetRooms->addTab(pRoomWidget, pMRoom->GetRoomName());
    }

    // 初始化矩阵房间
    if (!BCLocalServer::Application()->isFullScreenMode()) {
        for (int i = 0; i < m_lstMatrix.count(); i++) {
            BCMMatrix *pMatrix = m_lstMatrix.at(i);

            BCMatrixRoomWidget *pRoom = new BCMatrixRoomWidget(pMatrix, this);
            m_pTabWidgetRooms->addTab(pRoom, pMatrix->name);
        }
    }

    // 添加修改房间信号槽
    connect(m_pTabWidgetRooms, SIGNAL(currentChanged(int)), this, SLOT(onCurrentRoomChanged(int)));
}

void MainWindow::RefreshRoomName()
{
    // 当有拼接时循环BCMRoom
    int nBeginIndex = 0;
    for (int i = 0; i < m_lstRooms.count(); i++) {
        BCMRoom *pRoom = m_lstRooms.at(i);
        if (NULL == pRoom)
            continue;

        m_pTabWidgetRooms->setTabText(i, pRoom->GetRoomName());
    }

    nBeginIndex = m_lstRooms.count();

    if (!BCLocalServer::Application()->isFullScreenMode()) {
        for (int i = 0; i < m_lstMatrix.count(); i++) {
            BCMMatrix *pMatrix = m_lstMatrix.at(i);
            m_pTabWidgetRooms->setTabText(i+nBeginIndex, pMatrix->name);
        }
    }
}

BCRoomWidget *MainWindow::GetCurrentRoomWidget()
{
    int index = m_pTabWidgetRooms->currentIndex();
    BCRoomWidget *pWall = dynamic_cast<BCRoomWidget *>(m_pTabWidgetRooms->widget(index));

    return pWall;
}

void MainWindow::RefreshMatrixData()
{
    // 重新添加数据
    while ( !m_lstMatrix.isEmpty() )
        delete m_lstMatrix.takeFirst();

    BCLocalServer *pServer = BCLocalServer::Application();
    QList<sMatrix> lstMatrix = pServer->GetMatrixConfig();
    for (int i = 0; i < lstMatrix.count(); i++) {
        sMatrix smatrix = lstMatrix.at( i );

        BCMMatrix *pMatrix = new BCMMatrix();
        pMatrix->id = smatrix.id;
        pMatrix->name = smatrix.name;

        pMatrix->isConnectByNet = smatrix.isConnectByNet;             // 是否网络通信
        pMatrix->ip = smatrix.ip;                     // 网络通信IP
        pMatrix->port = smatrix.port;                       // 网络通信端口
        pMatrix->qsCurrentCom = smatrix.qsCurrentCom;           // 串口号
        pMatrix->nCurrentBaudRate = smatrix.nCurrentBaudRate;           // 波特率
        pMatrix->nCurrentDataBit = smatrix.nCurrentDataBit;            // 数据位
        pMatrix->nCurrentStopBit = smatrix.nCurrentStopBit;            // 停止位
        pMatrix->qsCurrentCheckBit = smatrix.qsCurrentCheckBit;      // 校验位
        pMatrix->qsCurrentStreamCtrl = smatrix.qsCurrentStreamCtrl;    // 控制流

        pMatrix->isOn = smatrix.isOn;                       // 大屏开关状态
        pMatrix->qsOnCmd = smatrix.qsOnCmd;                // 打开指令
        pMatrix->qsOffCmd = smatrix.qsOffCmd;               // 关闭指令

        pMatrix->cmdType = smatrix.cmdType;
        pMatrix->switchFlag = smatrix.switchFlag; // 切换指令表达式，如SW %1 %2...
        pMatrix->loadFlag = smatrix.loadFlag;   // 调取指令，如%1.
        pMatrix->saveFlag = smatrix.saveFlag;   // 保存指令，如%1,

        pMatrix->lstInputNode = smatrix.lstInputNode;   // 输入节点
        pMatrix->lstOutputNode = smatrix.lstOutputNode;  // 输出节点
        pMatrix->lstScene = smatrix.lstScene;       // 场景列表

        m_lstMatrix.append( pMatrix );
    }

    int nIndex = 0;
    for (int i = 0; i < m_pTabWidgetRooms->count(); i++) {
        BCMatrixRoomWidget *pRoom = dynamic_cast<BCMatrixRoomWidget *>( m_pTabWidgetRooms->widget(i) );
        if (NULL == pRoom)
            continue;

        // 更新房间中的数据
        if (nIndex < m_lstMatrix.count()) {
            pRoom->UpdateMMatrix( m_lstMatrix.at(nIndex++) );
        }
    }
}

BCMatrixRoomWidget *MainWindow::GetCurrentMatrixWidget()
{
    return dynamic_cast<BCMatrixRoomWidget *>(m_pTabWidgetRooms->currentWidget());
}

BCMatrixRoomWidget *MainWindow::GetMatrixWidgetByID(int id)
{
    for (int i = 0; i < m_pTabWidgetRooms->count(); i++) {
        BCMatrixRoomWidget *pWidget = dynamic_cast<BCMatrixRoomWidget *>( m_pTabWidgetRooms->widget(i) );
        if (NULL == pWidget)
            continue;

        if (id != pWidget->GetMMatrix()->id)
            continue;

        return pWidget;
    }

    return NULL;
}

BCMatrixRoomWidget *MainWindow::GetCurrentMatrixSceneWidget()
{
    return dynamic_cast<BCMatrixRoomWidget *>(m_pTabWidgetRooms->currentWidget());
}

BCMMatrix *MainWindow::GetMMatrix()
{
    return m_lstMatrix.isEmpty() ? nullptr : m_lstMatrix.first();
}

void MainWindow::InitToolBars()
{
    // 需要根据配置文件添加工具条
    m_pMainToolBar->Build();
    AddToolBar( MainWindow::SIGNALSOURCETOOLBAR );  // 信号源
}

BCToolBar *MainWindow::AddToolBar( TOOLBARTYPE eToolBarType )
{
    // 根据创建对象类型确定默认停放位置
    Qt::DockWidgetArea eDefaultArea = Qt::TopDockWidgetArea;
    switch ( eToolBarType ) {
        case MainWindow::MATRIXTOOLBAR:
            eDefaultArea = Qt::BottomDockWidgetArea;
            break;
        case MainWindow::EXTENDTOOLBAR:
            eDefaultArea = Qt::BottomDockWidgetArea;
            break;
        case MainWindow::SIGNALSOURCETOOLBAR:
            eDefaultArea = Qt::LeftDockWidgetArea;
            break;
        default:
            break;
    }

    // 新建toolbar
    BCToolBar *pToolBar = new BCToolBar(eToolBarType, this);
    this->addDockWidget(eDefaultArea, pToolBar);

    // ??? 功能代码保留
    // 在相同区域内有多个dockwidget，通过设置tabifyDockWidget可使多个widget类似tabwidget样式显示
//    QListIterator<BCToolBar *> it( m_lstToolBars );
//    while ( it.hasNext() ) {
//        BCToolBar *pFirst = it.next();
//        if (NULL == pFirst)
//            continue;

//        if (this->dockWidgetArea(pFirst) != eDefaultArea)
//            continue;

//        // 添加到一个tab中，并保证第一个显示
//        this->tabifyDockWidget(pFirst, pToolBar);
//        pFirst->raise();

//        break;
//    }

    m_lstToolBars.append( pToolBar );

    return pToolBar;
}

BCToolBar *MainWindow::GetToolBar(const QString &qsToolBarName)
{
    if (m_lstToolBars.isEmpty())
        return NULL;

    BCToolBar *pToolBar = NULL;
    QListIterator<BCToolBar *> it(m_lstToolBars);
    while( it.hasNext() ) {
        pToolBar = it.next();

        if (NULL == pToolBar)
            continue;

        if (pToolBar->GetToolBarName() != qsToolBarName)
            continue;

        return pToolBar;
    }

    return NULL;
}

BCToolBar *MainWindow::GetToolBar(TOOLBARTYPE eToolBarType)
{
    if (m_lstToolBars.isEmpty())
        return NULL;

    BCToolBar *pToolBar = NULL;
    QListIterator<BCToolBar *> it(m_lstToolBars);
    while( it.hasNext() ) {
        pToolBar = it.next();

        if (NULL == pToolBar)
            continue;

        if (pToolBar->GetToolBarType() != eToolBarType)
            continue;

        return pToolBar;
    }

    return NULL;
}

void MainWindow::InitTrayIcon()
{
    // 创建托盘
    m_pTrayIcon = new QSystemTrayIcon(this);
    m_pTrayIcon->setIcon( QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsIconPathOfTrayIcon) );
    m_pTrayIcon->setToolTip( BCCommon::g_qsTrayIconTooltips );

    // 创建信号槽，左键单击时显示主窗体
    connect(m_pTrayIcon,
            SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this,
            SLOT(onTrayIconActivated(QSystemTrayIcon::ActivationReason)));

    // 是否显示托盘
    BCCommon::g_bShowTrayIcon ? m_pTrayIcon->show() : m_pTrayIcon->hide();

    // 创建托盘项
    QAction *pShowAction = new QAction(this);
    QAction *pSetAction = new QAction(this);
    QAction *pAboutAction = new QAction(this);
    QAction *pCheckUpdateAction = new QAction(this);
    QAction *pLogOffAction = new QAction(this);
    QAction *pQuitAction = new QAction(this);

    // 设置托盘图标
    pShowAction->setIcon(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsTrayIconShowActionIconPath));
    pSetAction->setIcon(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsTrayIconSetActionIconPath));
    pAboutAction->setIcon(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsTrayIconAboutActionIconPath));
    pCheckUpdateAction->setIcon(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsTrayIconCheckUpdateActionIconPath));
    pLogOffAction->setIcon(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsTrayIconLogOffActionIconPath));
    pQuitAction->setIcon(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+BCCommon::g_qsTrayIconQuitActionIconPath));

    // 设置托盘文本
    pShowAction->setText(tr("显示"));
    pSetAction->setText(tr("设置"));
    pAboutAction->setText(tr("关于"));
    pCheckUpdateAction->setText(tr("检查更新"));
    pLogOffAction->setText(tr("注销"));
    pQuitAction->setText(tr("退出"));

    // 创建菜单并加入Action
    QMenu *pMenu = new QMenu(this);
//    pMenu->addAction(pShowAction);
//    pMenu->addAction(pSetAction);
//    pMenu->addSeparator();
//    pMenu->addAction(pAboutAction);
//    pMenu->addAction(pCheckUpdateAction);
//    pMenu->addSeparator();
//    pMenu->addAction(pLogOffAction);
    pMenu->addAction(pQuitAction);
    m_pTrayIcon->setContextMenu( pMenu );

    // 将事件关联信号槽
    connect(pShowAction, SIGNAL(triggered()), this, SLOT(show()));
    connect(pSetAction, SIGNAL(triggered()), this, SLOT(onTrayIconSet()));
    connect(pAboutAction, SIGNAL(triggered()), this, SLOT(onTrayIconAbout()));
    connect(pCheckUpdateAction, SIGNAL(triggered()), this, SLOT(onTrayIconCheckUpdate()));
    connect(pLogOffAction, SIGNAL(triggered()), this, SLOT(onTrayIconLogOff()));
    connect(pQuitAction, SIGNAL(triggered()), this, SLOT(onTrayIconQuit()));
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger: // 单击
        // 显示窗口
        this->show();
        break;
    case QSystemTrayIcon::DoubleClick: // 双击
        break;
    case QSystemTrayIcon::MiddleClick: // 中间滑轮
        break;
    default:
        break;
    }
}

void MainWindow::onTrayIconSet()
{
    // 弹出更多设置窗口
}

void MainWindow::onTrayIconAbout()
{
    // 弹出关于窗口，显示公司简介等
}

void MainWindow::onTrayIconCheckUpdate()
{
    // 系统检查更新
}

void MainWindow::onTrayIconLogOff()
{
    // 销毁内部对象，弹出登录框
}

void MainWindow::onHide()
{
    // 调试窗口
    if (NULL != m_pDebugDlg) {
        delete m_pDebugDlg;
        m_pDebugDlg = NULL;
    }

    this->hide();
}

void MainWindow::onTrayIconQuit()
{
    // 关闭快速定位窗口
    m_nQuitFlag = 1;
    this->close();
}

void MainWindow::onCurrentRoomChanged(int /*i*/)
{
    // 刷新状态按钮
    RefreshStatusBar();

    // 当前房间
    BCMRoom *pCurrentMRoom = GetCurrentMRoom();
    if (NULL != pCurrentMRoom) {
        // 刷新是否轮训按钮
        BCRibbonMainToolBarAction *pLoopSceneBtn = m_pMainToolBar->GetButtonAction( BCRibbonMainToolBar::WINDOWSCENELOOP );
        if (NULL != pLoopSceneBtn) {
            pLoopSceneBtn->RefreshSceneLoop( pCurrentMRoom );
        }

        // 刷新是否打开大屏开关
        BCRibbonMainToolBarAction *pDisplaySwitchBtn = m_pMainToolBar->GetButtonAction( BCRibbonMainToolBar::DISPLAYSWITCH );
        if (NULL != pDisplaySwitchBtn) {
            pDisplaySwitchBtn->Refresh( pCurrentMRoom );
        }
    }

    // 重新初始化信号源工具条
    BCToolBar *pSignalSrouceToolBar = GetToolBar(MainWindow::SIGNALSOURCETOOLBAR);
    if (NULL != pSignalSrouceToolBar) {
        BCFaceWidget* pWidget = dynamic_cast<BCFaceWidget *>( pSignalSrouceToolBar->widget() );
        if (NULL != pWidget) {
            // 参数0目前只刷新场景控件，因为其他都是全局的，只有场景是分房间的
            pWidget->Refresh( 0 );
        }
    }
}

void MainWindow::RefreshStatusBar()
{
    if (NULL == m_pSystemUser)
        return;

    // 当前房间
    BCMRoom *pCurrentMRoom = GetCurrentMRoom();
    if (NULL != pCurrentMRoom) {
        // 设置状态
        QList<BCMGroupDisplay *> lstGroupDisplay = pCurrentMRoom->GetGroupDisplay();
        if ( !lstGroupDisplay.isEmpty() ) {
            BCMGroupDisplay *pGroupDisplay = lstGroupDisplay.first();
            if (NULL != pGroupDisplay) {
                QSize arrSize = pGroupDisplay->GetArraySize();
                QSize resSize = pGroupDisplay->GetResolutionSize();
                this->statusBar()->showMessage(tr("连接状态：%1 | 屏幕排列：%2*%3 | 分辨率：%4*%5 | 管理员：%6 | %7")
                                               .arg(BCCommon::g_bConnectStatusOK ? tr("已连接") : tr("未连接"))
                                               .arg(arrSize.width())
                                               .arg(arrSize.height())
                                               .arg(resSize.width())
                                               .arg(resSize.height())
                                               .arg(m_pSystemUser->loginName)
                                               .arg(BCLocalServer::Application()->isFullScreenMode() ? tr("整屏模式") : tr("多屏模式")));
            }
        }
    } else {
        this->statusBar()->showMessage(tr("连接状态：%1 | 管理员：%2 | %3")
                                       .arg(BCCommon::g_bConnectStatusOK ? tr("已连接") : tr("未连接"))
                                       .arg(m_pSystemUser->loginName)
                                       .arg(BCLocalServer::Application()->isFullScreenMode() ? tr("整屏模式") : tr("多屏模式")));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!BCCommon::g_bApplicationQuitToTrayIcon || (m_nQuitFlag == 1)) {
        event->accept();
    } else {
        this->hide();
        event->ignore();
    }
}

void MainWindow::onRefreshConnect()
{
    BCLocalServer *pServer = BCLocalServer::Application();

    if ( BCCommon::g_bConnectStatusOK ) {
        pServer->DisConnect();
    } else {
        pServer->ReConnect();
    }
}

void MainWindow::onClearCurrentGroupDisplaySignalWindow()
{
    int index = m_pTabWidgetRooms->currentIndex();
    BCRoomWidget *pWall = dynamic_cast<BCRoomWidget *>(m_pTabWidgetRooms->widget(index));
    if (NULL == pWall)
        return;

    pWall->ClearSignalWindow();
}

void MainWindow::AddDebugCmd(const QString &cmd)
{
    if (NULL == m_pDebugDlg)
        return;

    m_pDebugDlg->AddCmd( cmd );
}

void MainWindow::onDebugDlgVisble()
{
    if (NULL == m_pDebugDlg) {
        m_pDebugDlg = new BCSettingDebugDlg( this );
    }

    m_pDebugDlg->setVisible( true );
}

void MainWindow::Show()
{
    resize(1024, 768);
    showMaximized();
    return;
    // 应用启动时显示模式:0：Nomarl需指定宽高；1：最小化显示；2：最大化显示；3：全屏显示
    switch( BCCommon::g_bApplicationDefaultDisplayMode ) {
    case 0: {
        this->showNormal();
        this->resize(BCCommon::g_bApplicationNomarlDisplayWidth,
                     BCCommon::g_bApplicationNomarlDisplayHeight);
    }
        break;
    case 1:
        this->showMinimized();
        break;
    case 2:
        this->showMaximized();
        break;
    case 3:
        this->showFullScreen();
        break;
    default:
        this->showMaximized();
        break;
    }
}

void MainWindow::stop()
{
    //
}

void MainWindow::RefreshMainWindow()
{
    // 初始化标题和状态栏显示
    this->setWindowTitle( BCCommon::g_qsApplicationTitle );
    m_pTrayIcon->setToolTip( BCCommon::g_qsTrayIconTooltips );
    if (NULL == m_pStatusLabel) {
        m_pStatusLabel = new QLabel(BCCommon::g_qsApplicationVersion, this);
        this->statusBar()->addPermanentWidget( m_pStatusLabel );
    }

    BCLocalServer *pServer = BCLocalServer::Application();
    connect(pServer, &BCLocalServer::roomStateChanged, this, &MainWindow::RefreshStatusBar);
    pServer->AddLog( "[MainWindow::RefreshMainWindow BEGIN.]" );
    QTime beginTime = QTime::currentTime();

    // 根据当前登陆人加载系统数据
    if ( !BCCommon::g_bConnectWithServer ) {
        LoadDataFromLocalServer();
        pServer->AddLog( QString("[MainWindow::RefreshMainWindow LoadDataFromLocalServer OVER. COST: %1 ms]").arg(beginTime.msecsTo( QTime::currentTime() )) );
        beginTime = QTime::currentTime();
    }

    // 使用xml数据初始化房间
    InitRoom();
    pServer->AddLog( QString("[MainWindow::RefreshMainWindow InitRoom OVER. COST: %1 ms]").arg(beginTime.msecsTo( QTime::currentTime() )) );
    beginTime = QTime::currentTime();

    // 添加tool bar
    InitToolBars();
    pServer->AddLog( QString("[MainWindow::RefreshMainWindow InitToolBars OVER. COST: %1 ms]").arg(beginTime.msecsTo( QTime::currentTime() )) );

    // 设置皮肤
    BCCommon::SetApplicationSkin("defaultStyle");
    Show();

    pServer->AddLog( "[MainWindow::RefreshMainWindow END.]" );

    // 刷新房间，默认显示第一个房间
    if ( !m_lstRooms.isEmpty() )
        onCurrentRoomChanged( 0 );
}

void MainWindow::onSwitchUser(bool)
{
    // 记录登录前用户
    BCSUser *pPreUser = m_pSystemUser;

    // 登录界面
    BCLoginDlg loginDlg;
    // 登录不成功则弹窗提示并且返回
    if(loginDlg.exec() != QDialog::Accepted) {
        return;
    }

    // 用户没变化直接返回
    if (pPreUser == m_pSystemUser)
        return;


    // 判断是否是连接服务器
        RefreshMainWindow();
}

QSize MainWindow::GetWinsizeOffset(int gid)
{
    QSize size(0, 0);

    for (int i = 0; i < m_lstWinsizeOffset.count(); i++) {
        BCSWinsizeOffset soff = m_lstWinsizeOffset.at( i );
        if (gid != soff.gid)
            continue;

        size.setWidth( soff.l );
        size.setHeight( soff.t );
        break;
    }

    return size;
}

void MainWindow::LoadGenaralConfig()
{
    // 判断文件是否可读
    QFile file( QString("../xml/BCGenaralConfig.xml") );
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

    BCCommon::g_bApplicationQuitToTrayIcon = docElem.attribute("QuitToTray").toInt() ? true : false;
    //BCCommon::g_bGroupScene = docElem.attribute("GroupScene").toInt() ? true : false;
    BCCommon::g_bSignalWindowCopy = docElem.attribute("SignalWindowCopy").toInt() ? true : false;
    BCCommon::g_nSignalWindowCopyCount = docElem.attribute("SignalWindowCopyCount").toInt();
    BCCommon::g_nSignalWindowCopyCountOfVideo = docElem.attribute("SignalWindowCopyCountOfVideo").toInt();
    BCCommon::g_qsApplicationTitle = docElem.attribute("AppTitle");
    BCCommon::g_qsTrayIconTooltips = docElem.attribute("AppTitle");
    BCCommon::g_qsApplicationCompany = docElem.attribute("AppCompany");
    BCCommon::g_nMonitorCount = docElem.attribute("MonitorCount").toInt();
    int nWallWidth = docElem.attribute("WallWidthRatio").toInt();
    int nWallHeigth = docElem.attribute("WallHeightRatio").toInt();
    BCCommon::g_dWallDisplayWidthHeightRatio = 1.0*nWallWidth/nWallHeigth;
    BCCommon::g_nDeviceType = docElem.attribute("DeviceType").toInt();
    BCCommon::g_bApplicationDefaultDisplayMode = docElem.attribute("AppDefaultDisplayMode").toInt();
    BCCommon::g_bApplicationNomarlDisplayWidth = docElem.attribute("AppNomarlDisplayWidth").toInt();
    BCCommon::g_bApplicationNomarlDisplayHeight = docElem.attribute("AppNomarlDisplayHeight").toInt();
    BCCommon::g_qsConnectName = docElem.attribute("ConnectName");
    BCCommon::g_qsDeviceType2000 = docElem.attribute("DeviceType2000");
    BCCommon::g_qsDeviceType2000A = docElem.attribute("DeviceType2000A");
    BCCommon::g_qsDeviceType4000 = docElem.attribute("DeviceType4000");
    BCCommon::g_qsDeviceTypeMatrix = docElem.attribute("DeviceTypeMatrix");
    BCCommon::g_bDefaultWindowOffset = docElem.attribute("DefaultWindowOffset").toInt() ? true : false;
}

void MainWindow::ClearSystemData()
{
    // 循环dockWidget链表
    QListIterator<BCToolBar *> it( m_lstToolBars );
    while ( it.hasNext() ) {
        BCToolBar *pToolBar = it.next();
        if (NULL == pToolBar)
            continue;

        // 移除dockWidget并隐藏
        this->removeDockWidget( pToolBar );
    }

    // 析构dockWidget
    while ( !m_lstToolBars.isEmpty() )
        delete m_lstToolBars.takeFirst();

    // 清空房间
    ClearRoom();

    // 清空房间
    while (!m_lstRooms.isEmpty())
        delete m_lstRooms.takeFirst();

    // 物理信号
    while (!m_lstInputChannels.isEmpty())
        delete m_lstInputChannels.takeFirst();

    // 清空矩阵
    while (!m_lstMatrix.isEmpty())
        delete m_lstMatrix.takeFirst();
}

// 从本地服务器加载数据
void MainWindow::LoadDataFromLocalServer()
{
    // 如果当前登陆人是空，则直接false
    if (NULL == m_pSystemUser)
        return;

    // 清空系统数据
    ClearSystemData();

    BCLocalServer *pServer = BCLocalServer::Application();

    // ------------------------------------------------------------------------------------------------------------ room
    sRoom sroom = pServer->GetRoomConfig();

    // 新建房间数据类，并添加到链表
    BCMRoom *pRoom = new BCMRoom( sroom.id );
    pRoom->SetType( sroom.type );
    pRoom->SetRoomName( sroom.name );
    pRoom->SetWallSize(sroom.width, sroom.height);
    pRoom->isFullScreeMode = sroom.fullMode;

    // 设置开关指令
    pRoom->SetSwitchConfig(sroom.isNetConnect, sroom.switchip, sroom.switchport, sroom.switchCom, sroom.switchBaudRate, sroom.switchDataBit,
                           sroom.switchStopBit, sroom.switchCheckBit, sroom.switchStreamCtrl, sroom.switchtype, sroom.switchoncmd, sroom.switchoffcmd);
    pRoom->SetSwitchOn((1 == sroom.switchStatus) ? true : false, false);

    m_lstRooms.append( pRoom );

    // 添加场景组
    auto sscreens = pServer->GetRoomScenes();

    // 没有场景组时创建场景组
    BCMGroupScene *pGroupScene = pRoom->GetGroupScene( 0 );

    // 添加场景
    for (int jj = 0; jj < sscreens.count(); jj++) {
        sWindowScene swindowscene = sscreens.at( jj );

        BCMWindowScene *pWindowScene = new BCMWindowScene( pGroupScene );
        pWindowScene->SetWindowSceneID( swindowscene.id );
        pWindowScene->SetIsCycled(swindowscene.cycle == 1 ? true : false, false);
        pWindowScene->SetWindowSceneBaseName(swindowscene.name);
        pWindowScene->SetWindowSceneName(swindowscene.name, false);
        pWindowScene->SetWindowSceneLoopInterval(swindowscene.loopInterval, false);

        pGroupScene->AddWindowScene(pWindowScene, false);

        for (int kk = 0; kk < swindowscene.lstData.count(); kk++) {
            sWindowSceneData swindowscenedata = swindowscene.lstData.at( kk );

            // 解析属性
            BCWindowSceneData *pSceneData = new BCWindowSceneData;
            pSceneData->m_nChannelID = swindowscenedata.chid;
            pSceneData->m_rect = QRectF(swindowscenedata.left,
                                        swindowscenedata.top,
                                        swindowscenedata.width,
                                        swindowscenedata.height);
            //pSceneData->m_nIPVSegmentation = swindowscenedata.ipvSegmentation;
            pSceneData->m_nWindowID = swindowscenedata.winid;
            //pSceneData->m_lstIP = swindowscenedata.lstIP;

            pWindowScene->AddWindowSceneData( pSceneData );
        }
    }

    for (int j = 0; j < sroom.lstGroupDisplay.count(); j++) {
        sGroupDisplay sgroupdisplay = sroom.lstGroupDisplay.at( j );

        // 解析属性，并赋值屏组实例
        BCMGroupDisplay *pGroupDisplay = new BCMGroupDisplay( pRoom );
        pGroupDisplay->SetGroupDisplayID( sgroupdisplay.id );
        pGroupDisplay->SetGroupDisplayName( sgroupdisplay.name );
        pGroupDisplay->SetRect( QRect(sgroupdisplay.left,
                                      sgroupdisplay.top,
                                      sgroupdisplay.width,
                                      sgroupdisplay.height) );
        pGroupDisplay->SetGroupDisplaySize(sgroupdisplay.width, sgroupdisplay.height);
        pGroupDisplay->SetArraySize(sgroupdisplay.arrayX, sgroupdisplay.arrayY);
        pGroupDisplay->SetResolutionSize(sgroupdisplay.resolutionX, sgroupdisplay.resolutionY);

        pRoom->AddGroupDisplay( pGroupDisplay );

        // 添加单屏
        for (int k = 0; k < sgroupdisplay.lstDisplay.count(); k++) {
            sDisplay sdisplay = sgroupdisplay.lstDisplay.at( k );

            BCMDisplay *pDisplay = new BCMDisplay( pGroupDisplay );
            pDisplay->SetDisplayID( sdisplay.id );
            pDisplay->SetDisplayName( sdisplay.name );
            pDisplay->SetResolution(sdisplay.resolutionX, sdisplay.resolutionY);
            pDisplay->SetRect(QRectF(sdisplay.left,
                                    sdisplay.top,
                                    sdisplay.resolutionX,
                                    sdisplay.resolutionY));
            // 如果屏组是矩阵模式则固定是1分割
            pDisplay->SetSegmentation((sroom.type == 4) ? 1 : sdisplay.segmentation);
            pGroupDisplay->AddDisplay( pDisplay );
        }
    }

    // ------------------------------------------------------------------------------------------------------------ inputchannel
    // K: "id": "1"
    // V: "name","pc 1", "channelType":"0", "signalSource":"0"
    QList<sInputChannel> lstInputChannel = pServer->GetInputChannels();
    for (int i = 0; i < lstInputChannel.count(); i++) {
        sInputChannel sinputchannel = lstInputChannel.at( i );

        // 新建输入通道
        BCMChannel *pChannel = new BCMChannel( sinputchannel.id );
        pChannel->SetChannelBaseName( sinputchannel.basename );
        pChannel->SetChannelName( sinputchannel.name );
        pChannel->SetChannelType( sinputchannel.type );
        pChannel->SetSignalSource( sinputchannel.signalsource );

        this->AddInputChannel( pChannel );
    }

    // ------------------------------------------------------------------------------------------------------------ matrix
    // 重新添加数据
    while ( !m_lstMatrix.isEmpty() )
        delete m_lstMatrix.takeFirst();

    QList<sMatrix> lstMatrix = pServer->GetMatrixConfig();
    for (int i = 0; i < lstMatrix.count(); i++) {
        sMatrix smatrix = lstMatrix.at( i );

        BCMMatrix *pMatrix = new BCMMatrix();
        pMatrix->id = smatrix.id;
        pMatrix->name = smatrix.name;

        pMatrix->isConnectByNet = smatrix.isConnectByNet;             // 是否网络通信
        pMatrix->ip = smatrix.ip;                     // 网络通信IP
        pMatrix->port = smatrix.port;                       // 网络通信端口
        pMatrix->qsCurrentCom = smatrix.qsCurrentCom;           // 串口号
        pMatrix->nCurrentBaudRate = smatrix.nCurrentBaudRate;           // 波特率
        pMatrix->nCurrentDataBit = smatrix.nCurrentDataBit;            // 数据位
        pMatrix->nCurrentStopBit = smatrix.nCurrentStopBit;            // 停止位
        pMatrix->qsCurrentCheckBit = smatrix.qsCurrentCheckBit;      // 校验位
        pMatrix->qsCurrentStreamCtrl = smatrix.qsCurrentStreamCtrl;    // 控制流

        pMatrix->isOn = smatrix.isOn;                       // 大屏开关状态
        pMatrix->qsOnCmd = smatrix.qsOnCmd;                // 打开指令
        pMatrix->qsOffCmd = smatrix.qsOffCmd;               // 关闭指令

        pMatrix->cmdType = smatrix.cmdType;
        pMatrix->switchFlag = smatrix.switchFlag; // 切换指令表达式，如SW %1 %2...
        pMatrix->loadFlag = smatrix.loadFlag;   // 调取指令，如%1.
        pMatrix->saveFlag = smatrix.saveFlag;   // 保存指令，如%1,

        pMatrix->lstInputNode = smatrix.lstInputNode;   // 输入节点
        pMatrix->lstOutputNode = smatrix.lstOutputNode;  // 输出节点
        pMatrix->lstScene = smatrix.lstScene;       // 场景列表

        m_lstMatrix.append( pMatrix );
    }
}

BCMRoom *MainWindow::GetMRoom(int id)
{
    QListIterator<BCMRoom *> it( m_lstRooms );
    while ( it.hasNext() ) {
        BCMRoom *pRoom = it.next();

        // 如果ID相对则直接返回
        if (pRoom->GetRoomID() == id)
            return pRoom;
    }

    return NULL;
}

BCMRoom *MainWindow::GetMRoomByGroupDisplayID(int id)
{
    // 1.循环房间
    QListIterator<BCMRoom *> itRoom( m_lstRooms );
    while ( itRoom.hasNext() ) {
        BCMRoom *pRoom = itRoom.next();

        // 2.循环屏组
        QList<BCMGroupDisplay *> lstGroupDisplay = pRoom->GetGroupDisplay();
        QListIterator<BCMGroupDisplay *> itGroup( lstGroupDisplay );
        while ( itGroup.hasNext() ) {
            BCMGroupDisplay *pGroupDisplay = itGroup.next();

            // 如果ID相对则直接返回
            if (pGroupDisplay->GetGroupDisplayID() == id)
                return pRoom;
        }
    }

    return NULL;
}

BCMGroupDisplay *MainWindow::GetGroupDisplay(int id)
{
    QListIterator<BCMRoom *> itRoom( m_lstRooms );
    while ( itRoom.hasNext() ) {
        BCMRoom *pRoom = itRoom.next();

        QListIterator<BCMGroupDisplay *> itGroupDisplay( pRoom->GetGroupDisplay() );
        while ( itGroupDisplay.hasNext() ) {
            BCMGroupDisplay *pGroupDisplay = itGroupDisplay.next();

            // 如果ID相对则直接返回
            if (pGroupDisplay->GetGroupDisplayID() == id)
                return pGroupDisplay;
        }
    }

    return NULL;
}

BCMDisplay *MainWindow::GetDisplay(int groupDisplayID, int displayID)
{
    QListIterator<BCMRoom *> itRoom( m_lstRooms );
    while ( itRoom.hasNext() ) {
        BCMRoom *pRoom = itRoom.next();

        QListIterator<BCMGroupDisplay *> itGroupDisplay( pRoom->GetGroupDisplay() );
        while ( itGroupDisplay.hasNext() ) {
            BCMGroupDisplay *pGroupDisplay = itGroupDisplay.next();

            if (groupDisplayID != pGroupDisplay->GetGroupDisplayID())
                continue;

            QListIterator<BCMDisplay *> itDisplay( pGroupDisplay->GetDisplays() );
            while ( itDisplay.hasNext() ) {
                BCMDisplay *pDisplay = itDisplay.next();

                // 如果ID相对则直接返回
                if (pDisplay->GetDisplayID() == displayID)
                    return pDisplay;
            }
        }
    }

    return NULL;
}

BCMRoom *MainWindow::GetCurrentMRoom()
{
    int n = m_pTabWidgetRooms->currentIndex();
    if ((m_lstRooms.count() <= 0) || (n < 0) || (n > m_lstRooms.count()-1))
        return NULL;

    return m_lstRooms.at( n );
}

BCMRoom *MainWindow::GetCurrentSceneMRoom()
{
    int index = m_pTabWidgetRooms->currentIndex();
    if (-1 == index)
        return NULL;

    if (m_lstRooms.count() > index)
        return m_lstRooms.at(index);
    else {
        if (m_lstMatrix.count()+m_lstRooms.count() > index) {
            // 找到对应矩阵
            BCMMatrix *pMatrix = m_lstMatrix.at(index-m_lstRooms.count());
            for (int i = 0; i < m_lstRooms.count(); i++) {
                BCMRoom *pRoom = m_lstRooms.at( i );
                    return pRoom;
            }
        }
    }

    return NULL;
}

BCMChannel *MainWindow::GetCurrentInputChannel()
{
    BCToolBar *pInputToolBar = GetToolBar( MainWindow::SIGNALSOURCETOOLBAR );
    if (NULL == pInputToolBar)
        return NULL;

    BCFaceWidget *pFaceWidget = dynamic_cast<BCFaceWidget *>( pInputToolBar->widget() );
    if (NULL == pFaceWidget)
        return NULL;

    BCControl *pSignal = dynamic_cast<BCControl *>( pFaceWidget->GetWidget(INPUTCHANNELSSIGSRC) );
    if (NULL == pSignal)
        return NULL;

    return pSignal->GetCurrentChannel();
}

void MainWindow::AddInputChannel(BCMChannel *pChannel)
{
    if (NULL == pChannel)
        return;

    m_lstInputChannels.append( pChannel );
}

BCMChannel *MainWindow::GetInputChannel(int id, int type)
{
    for (int i = 0; i < m_lstInputChannels.count(); i++) {
        BCMChannel *pChannel = m_lstInputChannels.at( i );
        if (NULL == pChannel)
            continue;

        if (0 == BCCommon::g_nDeviceType) {         // VP2000需要同时判断类型和ID
            if ((id != pChannel->GetChannelID()) || (type != pChannel->GetChannelType()))
                continue;
        } else {                                    // VP4000可以直接通道ID值找到具体通道
            if (id != pChannel->GetChannelID())
                continue;
        }

        return pChannel;
    }

    return NULL;
}

// *********************************************************************************************************************************
// 一下为ribbon风格相关代码
void MainWindow::createOptions()
{
    // 设置风格
    Qtitan::RibbonStyle::Theme themeId = Qtitan::RibbonStyle::Office2007Blue;
    if (m_ribbonStyle)
        themeId = m_ribbonStyle->getTheme();

    m_menuOptions = ribbonBar()->addMenu(tr("选项"));
    BCCommon::SetSystemFont( m_menuOptions );   // 设置字体
    QAction* actionStyle = m_menuOptions->addAction(tr("皮肤"));
    BCCommon::SetSystemFont( actionStyle );   // 设置字体
    QMenu* menuStyle = new QMenu(ribbonBar());
    BCCommon::SetSystemFont( menuStyle );   // 设置字体
    m_styleActions = new QActionGroup(this);

    QAction* actionBlue = menuStyle->addAction(tr("经典蓝"));
    actionBlue->setCheckable(true);
    actionBlue->setChecked(themeId == RibbonStyle::Office2007Blue);
    actionBlue->setObjectName("Office2007Blue");

    QAction* actionBlack = menuStyle->addAction(tr("雅致黑"));
    actionBlack->setObjectName("Office2007Black");
    actionBlack->setCheckable(true);
    actionBlack->setChecked(themeId == RibbonStyle::Office2007Black);

    QAction* actionSilver = menuStyle->addAction(tr("亮银色"));
    actionSilver->setObjectName("Office2007Silver");
    actionSilver->setCheckable(true);
    actionSilver->setChecked(themeId == RibbonStyle::Office2007Silver);

    QAction* actionAqua = menuStyle->addAction(tr("水绿色"));
    actionAqua->setObjectName("Office2007Aqua");
    actionAqua->setCheckable(true);
    actionAqua->setChecked(themeId == RibbonStyle::Office2007Aqua);

    QAction* actionScenic = menuStyle->addAction(tr("清爽蓝"));
    actionScenic->setObjectName("Windows7Scenic");
    actionScenic->setCheckable(true);
    actionScenic->setChecked(themeId == RibbonStyle::Windows7Scenic);

    QAction* action2010Blue = menuStyle->addAction(tr("暗蓝色"));
    action2010Blue->setObjectName("Office2010Blue");
    action2010Blue->setCheckable(true);
    action2010Blue->setChecked(themeId == RibbonStyle::Office2010Blue);

    QAction* action2010Silver = menuStyle->addAction(tr("暗银色"));
    action2010Silver->setObjectName("Office2010Silver");
    action2010Silver->setCheckable(true);
    action2010Silver->setChecked(themeId == RibbonStyle::Office2010Silver);

    QAction* action2010Black = menuStyle->addAction(tr("暗黑色"));
    action2010Black->setObjectName("Office2010Black");
    action2010Black->setCheckable(true);
    action2010Black->setChecked(themeId == RibbonStyle::Office2010Black);


    QAction* action2013White = menuStyle->addAction(tr("象牙白"));
    action2013White->setObjectName("Office2013White");
    action2013White->setCheckable(true);
    action2013White->setChecked(themeId == RibbonStyle::Office2013White);

    QAction* action2013Gray = menuStyle->addAction(tr("水墨灰"));
    action2013Gray->setObjectName("Office2013Gray");
    action2013Gray->setCheckable(true);
    action2013Gray->setChecked(themeId == RibbonStyle::Office2013Gray);

    QAction* action2013Dark = menuStyle->addAction(tr("碳素灰"));
    action2013Dark->setObjectName("Office2013Dark");
    action2013Dark->setCheckable(true);
    action2013Dark->setChecked(themeId == RibbonStyle::Office2013Dark);

    QAction* action2016Colorful = menuStyle->addAction(tr("海洋蓝"));
    action2016Colorful->setObjectName("Office2016Colorful");
    action2016Colorful->setCheckable(true);
    action2016Colorful->setChecked(themeId == RibbonStyle::Office2016Colorful);

    QAction* action2016White = menuStyle->addAction(tr("玉瓷白"));
    action2016White->setObjectName("Office2016White");
    action2016White->setCheckable(true);
    action2016White->setChecked(themeId == RibbonStyle::Office2016White);

    QAction* action2016DarkGray = menuStyle->addAction(tr("深幽黑"));
    action2016DarkGray->setObjectName("Office2016DarkGray");
    action2016DarkGray->setCheckable(true);

    // 以下为2007-2010风格样式，添加时有个bug
    // 1.默认为2007-2010其中一种时最大化缩小到普通大小时按钮无变化
    // 2.07-10和13-16之间切换时会出现闪屏现象，在07-10或者13-16单个区间段切换没有

    BCCommon::SetSystemFont( actionBlue );   // 设置字体
    BCCommon::SetSystemFont( actionBlack );   // 设置字体
    BCCommon::SetSystemFont( actionSilver );   // 设置字体
    BCCommon::SetSystemFont( actionAqua );   // 设置字体
    BCCommon::SetSystemFont( actionScenic );   // 设置字体
    BCCommon::SetSystemFont( action2010Blue );   // 设置字体
    BCCommon::SetSystemFont( action2010Silver );   // 设置字体
    BCCommon::SetSystemFont( action2010Black );   // 设置字体
    BCCommon::SetSystemFont( action2013White );   // 设置字体
    BCCommon::SetSystemFont( action2013Gray );   // 设置字体
    BCCommon::SetSystemFont( action2013Dark );   // 设置字体
    BCCommon::SetSystemFont( action2016Colorful );   // 设置字体
    BCCommon::SetSystemFont( action2016White );   // 设置字体
    BCCommon::SetSystemFont( action2016DarkGray );   // 设置字体

    m_styleActions->addAction(actionBlue);
    m_styleActions->addAction(actionBlack);
    m_styleActions->addAction(actionSilver);
    m_styleActions->addAction(actionAqua);
    m_styleActions->addAction(actionScenic);
    m_styleActions->addAction(action2010Blue);
    m_styleActions->addAction(action2010Silver);
    m_styleActions->addAction(action2010Black);

    // 以下为2013-2016风格样式
    m_styleActions->addAction(action2013White);
    m_styleActions->addAction(action2013Gray);
    m_styleActions->addAction(action2013Dark);
    m_styleActions->addAction(action2016Colorful);
    m_styleActions->addAction(action2016White);
    m_styleActions->addAction(action2016DarkGray);

    actionStyle->setMenu(menuStyle);
    connect(m_styleActions, SIGNAL(triggered(QAction*)), this, SLOT(optionsTheme(QAction*)));

    m_menuOptions->addSeparator();

    QAction* actionTitleGroupsVisible = m_menuOptions->addAction(tr("显示按钮组名"));
    BCCommon::SetSystemFont( actionTitleGroupsVisible );   // 设置字体
    actionTitleGroupsVisible ->setCheckable(true);
    actionTitleGroupsVisible ->setChecked(true);
    connect(actionTitleGroupsVisible , SIGNAL(triggered(bool)), this, SLOT(setTitleGroupsVisible(bool)));

    m_lstRibbonAction.clear();

    m_lstRibbonAction.append(actionStyle);
    m_lstRibbonAction.append(actionBlue);
    m_lstRibbonAction.append(actionBlack);
    m_lstRibbonAction.append(actionSilver);
    m_lstRibbonAction.append(actionAqua);
    m_lstRibbonAction.append(actionScenic);
    m_lstRibbonAction.append(action2010Blue);
    m_lstRibbonAction.append(action2010Silver);
    m_lstRibbonAction.append(action2010Black);
    m_lstRibbonAction.append(action2010Black);
    m_lstRibbonAction.append(action2013White);
    m_lstRibbonAction.append(action2013Gray);
    m_lstRibbonAction.append(action2013Dark);
    m_lstRibbonAction.append(action2016Colorful);
    m_lstRibbonAction.append(action2016White);
    m_lstRibbonAction.append(action2016DarkGray);
    m_lstRibbonAction.append(actionTitleGroupsVisible);

    // 初始化最小化按钮
    m_actionRibbonMinimize = ribbonBar()->addAction(QIcon(BCCommon::g_qsApplicationDefaultStylePath+"/ribbonMinimize.png"), tr("最小化"), Qt::ToolButtonIconOnly);
    connect(m_actionRibbonMinimize, SIGNAL(triggered()), this, SLOT(maximizeToggle()));
    connect(ribbonBar(), SIGNAL(minimizationChanged(bool)), this, SLOT(minimizationChanged(bool)));

    // 读XML初始化信息
    QFile file( QString("../xml/BCApplicationStyle.xml") );
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
    if (0 != docElem.attribute("MinHeader").toInt()) {
        maximizeToggle();
        minimizationChanged( true );
    }

    if (0 != docElem.attribute("HideGroup").toInt()) {
        setTitleGroupsVisible( false );
        actionTitleGroupsVisible->setChecked( false );
    }
    int nStyle = docElem.attribute("Style").toInt();
    switch ( nStyle ) {
    case 1:
        actionBlack->setChecked( true );
        optionsTheme( actionBlack );
        break;
    case 2:
        actionSilver->setChecked( true );
        optionsTheme( actionSilver );
        break;
    case 3:
        actionAqua->setChecked( true );
        optionsTheme( actionAqua );
        break;
    case 4:
        actionScenic->setChecked( true );
        optionsTheme( actionScenic );
        break;
    case 5:
        action2010Blue->setChecked( true );
        optionsTheme( action2010Blue );
        break;
    case 6:
        action2010Silver->setChecked( true );
        optionsTheme( action2010Silver );
        break;
    case 7:
        action2010Black->setChecked( true );
        optionsTheme( action2010Black );
        break;
    case 8:
        action2013White->setChecked( true );
        optionsTheme( action2013White );
        break;
    case 9:
        action2013Gray->setChecked( true );
        optionsTheme( action2013Gray );
        break;
    case 10:
        action2013Dark->setChecked( true );
        optionsTheme( action2013Dark );
        break;
    case 11:
        action2016Colorful->setChecked( true );
        optionsTheme( action2016Colorful );
        break;
    case 12:
        action2016White->setChecked( true );
        optionsTheme( action2016White );
        break;
    case 13:
        action2016DarkGray->setChecked( true );
        optionsTheme( action2016DarkGray );
        break;
    default:
        actionBlue->setChecked( true );
        optionsTheme( actionBlue );
        break;
    }
}

void MainWindow::maximizeToggle()
{
    ribbonBar()->setMinimized(!ribbonBar()->isMinimized());
}

void MainWindow::minimizationChanged(bool minimized)
{
    m_actionRibbonMinimize->setChecked(minimized);
    m_actionRibbonMinimize->setIcon(minimized ? QIcon(BCCommon::g_qsApplicationDefaultStylePath+"/ribbonMaximize.png") :
        QIcon(BCCommon::g_qsApplicationDefaultStylePath+"/ribbonMinimize.png"));

    // 读XML初始化信息
    QFile file( QString("../xml/BCApplicationStyle.xml") );
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
    docElem.setAttribute("MinHeader", minimized ? 1 : 0);

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();

}

void MainWindow::setTitleGroupsVisible(bool checked)
{
    ribbonBar()->setTitleGroupsVisible(checked);

    // 读XML初始化信息
    QFile file( QString("../xml/BCApplicationStyle.xml") );
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
    docElem.setAttribute("HideGroup", checked ? 0 : 1);

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void MainWindow::onTimeOutOfOutOfDate()
{
    // 1.设备过期则显示用户联系厂家索要许可
//    BCLocalServer *pServer = BCLocalServer::Application();
//    if (1 == pServer->IsOutOfDate()) {
//        if (NULL == m_pOutOfDateDlg)
//            m_pOutOfDateDlg = new BCSettingOutOfDateDlg( this );

//        m_pOutOfDateDlg->exec();
//    }

    // 2.定时任务
    QString qsTime = QDateTime::currentDateTime().toString("hh:mm");    // 当前时间，如19：30
    QString qsWeek = QDateTime::currentDateTime().toString("ddd");      // 当前星期数，如"周六"
    int nWeek = 1;
    if ("周一" == qsWeek) {
        nWeek = 1;
    } else if ("周二" == qsWeek) {
        nWeek = 2;
    } else if ("周三" == qsWeek) {
        nWeek = 3;
    } else if ("周四" == qsWeek) {
        nWeek = 4;
    } else if ("周五" == qsWeek) {
        nWeek = 5;
    } else if ("周六" == qsWeek) {
        nWeek = 6;
    } else if ("周日" == qsWeek) {
        nWeek = 7;
    }

    for (int i = 0; i < m_lstTaskPlanning.count(); i++) {
        sTaskPlanning staskplanning = m_lstTaskPlanning.at( i );

        // 如果循环周期不包括今天则下个任务
        if ( !staskplanning.cycle.contains( QString::number(nWeek) ) )
            continue;

        // 如果时间不等则不执行
        if (staskplanning.time != qsTime)
            continue;

        // 查找房间
        qDebug() << staskplanning.roomID << "~~~~~~~~~~~~~~~~~~~~~~~~~11";
        BCMRoom *pRoom = this->GetMRoom( staskplanning.roomID );
        if (NULL == pRoom)
            continue;
        qDebug() << staskplanning.roomID << "~~~~~~~~~~~~~~~~~~~~~~~~~22";

        switch ( staskplanning.taskType ) {
        case 0: { // 开机
            pRoom->SetSwitchOn( true );
            // 刷新全局窗口定位代码
            BCRibbonMainToolBarAction *pSwitchBtn = m_pMainToolBar->GetButtonAction( BCRibbonMainToolBar::DISPLAYSWITCH );
            if (NULL != pSwitchBtn) {
                pSwitchBtn->Refresh( pRoom );
            }
        }
            break;
        case 1: { // 关机
            pRoom->SetSwitchOn( false );
            // 刷新全局窗口定位代码
            BCRibbonMainToolBarAction *pSwitchBtn = m_pMainToolBar->GetButtonAction( BCRibbonMainToolBar::DISPLAYSWITCH );
            if (NULL != pSwitchBtn) {
                pSwitchBtn->Refresh( pRoom );
            }
        }
            break;
        case 2: { // 打开轮训
            pRoom->SetLoopWindowScene( true );
            // 刷新全局窗口定位代码
            BCRibbonMainToolBarAction *pSceneLoopBtn = m_pMainToolBar->GetButtonAction( BCRibbonMainToolBar::WINDOWSCENELOOP );
            if (NULL != pSceneLoopBtn) {
                pSceneLoopBtn->RefreshSceneLoop( pRoom );
            }
        }
            break;
        case 3: { // 关闭轮训
            pRoom->SetLoopWindowScene( false );
            // 刷新全局窗口定位代码
            BCRibbonMainToolBarAction *pSceneLoopBtn = m_pMainToolBar->GetButtonAction( BCRibbonMainToolBar::WINDOWSCENELOOP );
            if (NULL != pSceneLoopBtn) {
                pSceneLoopBtn->RefreshSceneLoop( pRoom );
            }
        }
        case 4: { // 调用场景
            bool bShow = false;
            BCMGroupScene *pGroupScene = pRoom->GetGroupScene(0);

            BCMWindowScene *pWindowScene = pGroupScene->GetWindowScene( staskplanning.sceneID );
            if (NULL != pWindowScene) {
                pWindowScene->Show();
                bShow = true;
            }

            if ( bShow )
                break;
        }
            break;
        default:
            break;
        }
    }
}

void MainWindow::optionsTheme(QAction* action)
{
    int nStyle = 0;
    Qtitan::RibbonStyle::Theme themeId = Qtitan::RibbonStyle::Office2007Blue;
    if (action->objectName() == "Office2007Blue") {
        nStyle = 0;
        themeId = RibbonStyle::Office2007Blue;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2007Black") {
        nStyle = 1;
        themeId = RibbonStyle::Office2007Black;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2007Silver") {
        nStyle = 2;
        themeId = RibbonStyle::Office2007Silver;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2007Aqua") {
        nStyle = 3;
        themeId = RibbonStyle::Office2007Aqua;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Windows7Scenic") {
        nStyle = 4;
        themeId = OfficeStyle::Windows7Scenic;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2010Blue") {
        nStyle = 5;
        themeId = OfficeStyle::Office2010Blue;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2010Silver") {
        nStyle = 6;
        themeId = OfficeStyle::Office2010Silver;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2010Black") {
        nStyle = 7;
        themeId = OfficeStyle::Office2010Black;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2013White") {
        nStyle = 8;
        themeId = OfficeStyle::Office2013White;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2013Gray") {
        nStyle = 9;
        themeId = OfficeStyle::Office2013Gray;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2013Dark") {
        nStyle = 10;
        themeId = OfficeStyle::Office2013Dark;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2016Colorful") {
        nStyle = 11;
        themeId = OfficeStyle::Office2016Colorful;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2016White") {
        nStyle = 12;
        themeId = OfficeStyle::Office2016White;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    } else if (action->objectName() == "Office2016DarkGray") {
        nStyle = 13;
        themeId = OfficeStyle::Office2016DarkGray;
        BCCommon::SetApplicationSkin( "defaultStyle" );
    }

    m_ribbonStyle->setTheme(themeId);

    // 读XML初始化信息
    QFile file( QString("../xml/BCApplicationStyle.xml") );
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
    docElem.setAttribute("Style", nStyle);

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}
