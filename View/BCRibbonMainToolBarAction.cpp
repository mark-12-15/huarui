#include "BCRibbonMainToolBarAction.h"

#include <QFileIconProvider>
#include <QAction>
#include "../Common/BCCommon.h"
#include "BCLocationDlg.h"
#include "BCToolBar.h"
#include "../Setting/BCSettingDisplayInfoDlg.h"
#include "../View/BCScene.h"
#include "../View/BCFaceWidget.h"
#include "../Setting/BCSettingMainPanelStyle.h"
#include "../Setting/BCSettingPasswordStyle.h"
#include "../Setting/BCSettingBoardCardDlg.h"
#include "../Setting/BCSettingOutSideCommandDlg.h"
#include "../Setting/BCSettingDisplaySwitchConfigDlg.h"
#include "../Setting/BCSettingAutoReadInputChannelConfigDlg.h"
#include "../Model/BCMGroupScene.h"
#include "../Common/BCLocalServer.h"
#include "../Model/BCMRoom.h"
#include "../Setting/BCSettingMatrixFormatDlg.h"
#include "../Setting/BCSettingOutsideInterfaceDlg.h"
#include "DeviceConnectDlg.h"
#include "LightSettingDlg.h"
#include "ColorSettingDlg.h"
#include "AdminPasswordDlg.h"
#include "DeviceFormatDlg.h"
#include <QFileDialog>
#include "BCMGroupDisplay.h"

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
    case BCRibbonMainToolBar::COLORSET:
        QObject::connect(this, &QAction::triggered, this, [this] {
            auto dlg = new ColorSettingDlg(BCCommon::Application());
            dlg->exec();
        });
        break;
    case BCRibbonMainToolBar::DISPLAYSWITCHCONFIG:
        QObject::connect(this, &QAction::triggered, this, [this] {
            auto dlg = new BCSettingDisplaySwitchConfigDlg(BCCommon::Application());
            dlg->exec();
        });
        break;
    case BCRibbonMainToolBar::MATRIXFORMAT:
        QObject::connect(this, &QAction::triggered, this, [this] {
            auto dlg = new BCSettingMatrixFormatDlg(BCCommon::Application());
            dlg->exec();
        });
        break;
    case BCRibbonMainToolBar::DEVICEFORMAT:
        QObject::connect(this, &QAction::triggered, this, [this] {
            auto dlg = new DeviceFormatDlg(BCCommon::Application());
            dlg->exec();
        });
        break;
    case BCRibbonMainToolBar::AUTHORITY:
        QObject::connect(this, &QAction::triggered, this, [this] {
            auto dlg = new AdminPasswordDlg(BCCommon::Application());
            dlg->exec();
        });
        break;
    case BCRibbonMainToolBar::IMPORTFILE:
        QObject::connect(this, &QAction::triggered, this, [] {
            auto fileName = QFileDialog::getOpenFileName(BCCommon::Application(),
                                                         tr("选择配置文件"),
                                                         ".",
                                                         tr("接收卡配置文件(*.mvcfg)"));
             if (!fileName.isEmpty()) {
                 QFile file(fileName);
                 if (file.open(QIODevice::ReadOnly)) {
                     auto data = file.readAll();
                     file.close();

                     data = QByteArray::fromBase64(data);
                     auto err = new QJsonParseError;
                     auto doc = QJsonDocument::fromJson(data, err);
                     if (err->error != QJsonParseError::NoError)
                     {
                         qWarning() << "read mvcfg error, desc: " << err->errorString();
                         return;
                     }

                     auto obj = doc.object();
                     if (obj.isEmpty())
                     {
                         qWarning() << "mvcfg file format error, not a json obj.";
                         return;
                     }

                     if (!obj.contains("mode")
                             || !obj.contains("arrX") || !obj.contains("arrY")
                             || !obj.contains("width") || !obj.contains("height"))
                     {
                         qWarning() << "mvcfg file format error, miss key.";
                         return;
                     }

                     auto mode = obj.value("mode").toInt() == 1;
                     auto arrX = obj.value("arrX").toInt();
                     auto arrY = obj.value("arrY").toInt();
                     auto width = obj.value("width").toInt();
                     auto height = obj.value("height").toInt();

                     if (BCLocalServer::Application()->isUpdateRoomConfig(mode, arrX, arrY, width, height))
                     {
                         BCLocalServer::Application()->updateFormatToDevice(mode, arrX, arrY, width, height);
                     }
                 }
             }
        });
        break;
    case BCRibbonMainToolBar::EXPORTFILE:
        QObject::connect(this, &QAction::triggered, this, [] {
            auto fileName = QFileDialog::getSaveFileName(BCCommon::Application(),
                                                         tr("保存配置文件"),
                                                         ".",
                                                         tr("接收卡配置文件(*.mvcfg)"));
            if (!fileName.isEmpty()) {
                QFile file(fileName);
                if (file.open(QIODevice::WriteOnly)) {
                    QJsonObject obj;
                    auto room = BCCommon::Application()->GetMRoom(0);
                    if (room) {
                        obj.insert("mode", room->isFullScreeMode ? 1 : 0);

                        auto group = room->GetGroupDisplay(0);
                        if (group) {
                            auto arr = group->GetArraySize();
                            auto size = group->GetResolutionSize();

                            obj.insert("arrX", arr.width());
                            obj.insert("arrY", arr.height());
                            obj.insert("width", size.width());
                            obj.insert("height", size.height());
                        }
                    }
                    QJsonDocument doc(obj);
                    auto data = doc.toJson(QJsonDocument::Indented);

                    file.write(data.toBase64());
                    file.close();
                }
            }
        });
        break;
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

void BCRibbonMainToolBarAction::RefreshSceneLoop(BCMRoom *pRoom)
{
    // 只有轮训场景按钮设置有效
    if ((m_eType != BCRibbonMainToolBar::WINDOWSCENELOOP) || (NULL == pRoom))
        return;

    QString qsIconPath = pRoom->IsLoopWindowScene() ? m_btn.m_qsIconOnPath : m_btn.m_qsIconOffPath;
    this->setIcon(QIcon( BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle+"/"+qsIconPath ));
}

