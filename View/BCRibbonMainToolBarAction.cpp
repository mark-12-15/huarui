#include "BCRibbonMainToolBarAction.h"

#include <QFileIconProvider>
#include <QAction>
#include "../Common/BCCommon.h"
#include "BCLocationDlg.h"
#include "BCToolBar.h"

#include "../Setting/BCSettingDisplayInfoDlg.h"
#include "../View/BCScene.h"
#include "../View/BCFaceWidget.h"
#include "../Setting/BCSettingDisplyModelStyle.h"
#include "../Setting/BCSettingMainPanelStyle.h"
#include "../Setting/BCSettingPasswordStyle.h"
#include "../Setting/BCSettingBoardCardDlg.h"
#include "../Setting/BCSettingOutSideCommandDlg.h"
#include "../Setting/BCSettingDisplaySwitchConfigDlg.h"
#include "../Setting/BCSettingAutoReadInputChannelConfigDlg.h"
#include "../Model/BCMGroupScene.h"
#include "../View/BCAutoDateDlg.h"
#include "../Common/BCLocalServer.h"
#include "../Model/BCMRoom.h"
#include "../Setting/BCSettingMatrixFormatDlg.h"
#include "../Setting/BCSettingOutsideInterfaceDlg.h"
#include "DeviceConnectDlg.h"
#include "LightSettingDlg.h"

BCRibbonMainToolBarAction::BCRibbonMainToolBarAction(BCRibbonMainToolBar::BUTTONTYPE eType, const BCRibbonMainToolBar::ButtonInfo &btn,
                                                     QObject *parent)
    :QAction(QIcon(BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+btn.m_qsIconOffPath), btn.m_qsText, parent)
{
    m_eType = eType;
    m_btn = btn;

    init();
}

BCRibbonMainToolBarAction::~BCRibbonMainToolBarAction()
{

}

void BCRibbonMainToolBarAction::init()
{
    MainWindow *pApplication = BCCommon::Application();
    switch ( m_eType ) {
    case BCRibbonMainToolBar::DEVICECONNECT:
        QObject::connect(this, &QAction::triggered, this, [this] {
            auto dlg = new DeviceConnectDlg(BCCommon::Application());
            dlg->exec();
        });
        break;
    case BCRibbonMainToolBar::WINDOWSCENELOOP:
        // 轮训设置
        QObject::connect(this, SIGNAL(triggered(bool)), this, SLOT(onLoopSceneChanged(bool)));
        break;
    case BCRibbonMainToolBar::QUIT:
        // 退出系统
        QObject::connect(this, SIGNAL(triggered(bool)), pApplication, SLOT(onTrayIconQuit()));
        break;
    case BCRibbonMainToolBar::DISPLAYSWITCH:
        // 打开或关闭当前大屏显示与否
        QObject::connect(this, SIGNAL(triggered(bool)), this, SLOT(onDisplaySwitch()));
        break;
    case BCRibbonMainToolBar::WINDOWSCENEADD:
    case BCRibbonMainToolBar::WINDOWSCENEDELETE:
    case BCRibbonMainToolBar::WINDOWSCENESET:
        QObject::connect(this, SIGNAL(triggered(bool)), this, SLOT(onSceneSet(bool)));
        break;
    case BCRibbonMainToolBar::LIGHTSET:
        QObject::connect(this, &QAction::triggered, this, [this] {
            auto dlg = new LightSettingDlg(BCCommon::Application());
            dlg->exec();
        });
        break;
    case BCRibbonMainToolBar::DEVICEFORMAT:
    case BCRibbonMainToolBar::DISPLAYSWITCHCONFIG:
    default:
        break;
    }
}

void BCRibbonMainToolBarAction::onLoopSceneChanged(bool)
{
    MainWindow *pApplication = BCCommon::Application();
    BCToolBar *pToolBar = pApplication->GetToolBar(MainWindow::SIGNALSOURCETOOLBAR);
    if (NULL == pToolBar)
        return;

    BCMRoom *pMRoom = pApplication->GetCurrentSceneMRoom();
    if (NULL != pMRoom) {
        pMRoom->SetLoopWindowScene( !pMRoom->IsLoopWindowScene() );

        QString qsIconPath = pMRoom->IsLoopWindowScene() ? m_btn.m_qsIconOnPath : m_btn.m_qsIconOffPath;
        this->setIcon(QIcon( BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+qsIconPath ));
        this->setText(pMRoom->IsLoopWindowScene() ? tr("停止轮巡") : tr("启动轮巡"));
    } else {
//        BCCommunication *pCommunication = BCCommunication::Application();
//        if ( !pMRoom->IsLoopWindowScene() ) {
//            pCommunication->RequestControlByGroupScene(pMRoom->GetRoomID(), 0);
//        } else {
//            // 如果是关闭轮训则直接结束请求
//            pCommunication->RequestOver( pMRoom->GetRoomID() );

//            // 直接改变图标为未选中
//            this->setIcon(QIcon( BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+m_btn.m_qsIconOffPath ));
//        }
    }
}

void BCRibbonMainToolBarAction::onDisplaySwitch()
{
    MainWindow *pApplication = BCCommon::Application();
    BCMRoom *pRoom = pApplication->GetCurrentMRoom();
    if (NULL == pRoom)
        return;

    bool bIsSwitchOn = pRoom->IsSwitchOn();
    pRoom->SetSwitchOn( !bIsSwitchOn );

    QString qsIconPath = !bIsSwitchOn ? m_btn.m_qsIconOnPath : m_btn.m_qsIconOffPath;
    this->setIcon(QIcon( BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+qsIconPath ));
}

void BCRibbonMainToolBarAction::RefreshCommunication()
{
    QString qsIconPath = BCCommon::g_bConnectStatusOK ? m_btn.m_qsIconOnPath : m_btn.m_qsIconOffPath;
    this->setIcon(QIcon( BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+qsIconPath ));
}

void BCRibbonMainToolBarAction::Refresh(BCMRoom *pRoom)
{
    // 判断类型
    if (BCRibbonMainToolBar::DISPLAYSWITCH != m_eType)
        return;

    QString qsIconPath = pRoom->IsSwitchOn() ? m_btn.m_qsIconOnPath : m_btn.m_qsIconOffPath;
    this->setIcon(QIcon( BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+qsIconPath ));
}

void BCRibbonMainToolBarAction::onSceneSet(bool)
{
    MainWindow *pApplication = BCCommon::Application();
    BCToolBar *pToolBar = pApplication->GetToolBar(MainWindow::SIGNALSOURCETOOLBAR);
    if (NULL == pToolBar)
        return;

    BCFaceWidget* pWidget = dynamic_cast<BCFaceWidget *>( pToolBar->widget() );
    if (NULL == pWidget)
        return;

    BCScene *pScene = dynamic_cast<BCScene *>(pWidget->GetWidget(MainWindow::WINDOWSCENESIGSRC));
    if(pScene == NULL){
        return;
    }
    switch (m_eType) {
    case BCRibbonMainToolBar::WINDOWSCENEADD:
        pScene->SetAction( 1 );
        break;
    case BCRibbonMainToolBar::WINDOWSCENEDELETE:
        pScene->SetAction( 2 );
        break;
    case BCRibbonMainToolBar::WINDOWSCENESET: {
        pApplication->SetWindowSceneSetSwitch( !pApplication->IsWindowSceneSetSwitch() );
        pScene->SetAction( 4 );
        QString icon = pApplication->IsWindowSceneSetSwitch() ? m_btn.m_qsIconOnPath : m_btn.m_qsIconOffPath;
        this->setIcon( QIcon( BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+icon ) );
    }
        break;
    default:
        break;
    }
}


void BCRibbonMainToolBarAction::onShowDialog(bool)
{
    // 点击时创建对话框
    QDialog *pDlg = NULL;
    switch ( m_eType ) {
    case BCRibbonMainToolBar::DEVICEFORMAT:
        //pDlg = new BCSettingDeviceFormatDlg( BCCommon::Application() );
        break;
    case BCRibbonMainToolBar::DISPLAYSWITCHCONFIG:
        pDlg = new BCSettingDisplaySwitchConfigDlg( BCCommon::Application() );
        break;
//    case BCRibbonMainToolBar::MATRIXFORMAT:
//        pDlg = new BCSettingMatrixFormatDlg( BCCommon::Application() );
//        break;
    default:
        break;
    }

    if (NULL == pDlg)
        return;

    pDlg->exec();
}

void BCRibbonMainToolBarAction::RefreshSceneLoop(BCMRoom *pRoom)
{
    // 只有轮训场景按钮设置有效
    if ((m_eType != BCRibbonMainToolBar::WINDOWSCENELOOP) || (NULL == pRoom))
        return;

    QString qsIconPath = pRoom->IsLoopWindowScene() ? m_btn.m_qsIconOnPath : m_btn.m_qsIconOffPath;
    this->setIcon(QIcon( BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+qsIconPath ));
}

