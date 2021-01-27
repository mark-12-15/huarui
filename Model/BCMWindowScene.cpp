#include "BCMWindowScene.h"
#include <QtGlobal>

#include "BCMGroupScene.h"
#include "../Common/BCCommon.h"
#include "../Common/BCLocalServer.h"
#include "../Model/BCMRoom.h"
#include "../Model/BCMChannel.h"
#include "../Model/BCMGroupDisplay.h"
#include "../View/BCSignalWindowDisplayWidget.h"
#include "../View/BCGroupDisplayWidget.h"
#include "../View/BCRoomWidget.h"
#include "BCMMatrix.h"

BCMWindowScene::BCMWindowScene(BCMGroupScene *pGroupScene)
{
    m_pRoom = NULL;
    m_pGroupScene = pGroupScene;
    m_bIsCycled = false;
    m_nLoopInterval = 3;
}

BCMWindowScene::BCMWindowScene(BCMRoom *pRoom)
{
    m_pRoom = pRoom;
    m_pGroupScene = NULL;
    m_bIsCycled = false;
    m_nLoopInterval = 3;
}

BCMWindowScene::~BCMWindowScene()
{
    while ( !m_lstData.isEmpty() )
        delete m_lstData.takeFirst();
}

void BCMWindowScene::AddWindowSceneData(BCWindowSceneData *pData)
{
    m_lstData.append( pData );
}

bool BCMWindowScene::Save(bool bConnectServer)
{
    // 归属不可同时为空
    if ((NULL == m_pRoom) && (NULL == m_pGroupScene))
        return false;

    // 取信号窗管理类和房间尺寸
    BCMRoom *pMRoom = (NULL == m_pRoom) ? m_pGroupScene->GetRoom() : m_pRoom;

    BCRoomMainWidget *pSignalManager = pMRoom->GetSignalWidgetManager();
    if (NULL == pSignalManager)
        return false;

    QList<BCSignalWindowDisplayWidget *> lstSignalWindows = pSignalManager->GetSignalWindows();
    QListIterator<BCSignalWindowDisplayWidget *> it( lstSignalWindows );
    while (it.hasNext()) {
        BCSignalWindowDisplayWidget *pSignalWindow = it.next();
        if (NULL == pSignalWindow)
            continue;

        BCMChannel *pChannel = pSignalWindow->GetInputChannel();
        if (NULL == pChannel)
            continue;

        // 构造场景数据
        BCWindowSceneData *pWindowData = new BCWindowSceneData();
        pWindowData->m_nChannelID = pChannel->GetChannelID();
        pWindowData->m_nWindowID = pSignalWindow->GetWindowID();
        pWindowData->m_rect = pSignalWindow->GetFactRect();

        // 将场景数据添加到场景链表中
        this->AddWindowSceneData( pWindowData );
    }

    if ( !bConnectServer )
        return true;

    // 发送指令
    BCLocalServer *pServer = BCLocalServer::Application();
    pServer->save(1, m_id);

    // 存本地数据库
    pServer->AddScreen(this);

    return true;
}

bool BCMWindowScene::Show(bool bConnectServer)
{
    // 归属不可同时为空
    if ((NULL == m_pRoom) && (NULL == m_pGroupScene))
        return false;

    MainWindow *pApplication = BCCommon::Application();

    // 取信号窗管理类和房间尺寸
    BCMRoom *pMRoom = (NULL == m_pRoom) ? m_pGroupScene->GetRoom() : m_pRoom;
    //QSizeF wallSize = pMRoom->GetWallSize();

    // 清空对话框，但不发送指令
    BCRoomMainWidget *pSignalManager = pMRoom->GetSignalWidgetManager();
    pSignalManager->ClearSignalWindow(false);

    // 循环场景内数据类
    QListIterator<BCWindowSceneData *> it( m_lstData );
    while (it.hasNext()) {
        BCWindowSceneData *pWindowData = it.next();
        if (NULL == pWindowData)
            continue;

        // 根据ID找到屏组
        BCMGroupDisplay *pMGroupDisplay = pMRoom->GetGroupDisplay( 0 );
        if (NULL == pMGroupDisplay)
            continue;

        // 根据ID找到输入通道
        BCMChannel *pChannel = pApplication->GetInputChannel(pWindowData->m_nChannelID, 0);
        if (NULL == pChannel)
            continue;

        BCRoomMainWidget *pSignalManager = pMRoom->GetSignalWidgetManager();
        BCSignalWindowDisplayWidget *pSignalWindowItem = pSignalManager->AddSignalWindow(pWindowData->m_rect.left(),
                                                                                         pWindowData->m_rect.top(),
                                                                                         pWindowData->m_rect.width(),
                                                                                         pWindowData->m_rect.height(),
                                                                                         pMGroupDisplay->GetDisplayWidgetManager(), pChannel, pWindowData->m_nWindowID, false);
    }

    // 全局刷新下信号窗文字显示，因为信号窗先构造，后加入链表，而构造时调用resize，所以最后一个信号窗文字不会被刷新，需要手动调用一次
    pSignalManager->RefreshSignalWindowTextDisplay();

    // 发送指令
    if ( !bConnectServer )
        return true;

    // 循环房间内屏组，多个屏组使用load，因为可能一个房间内多个屏组同时保存成一个场景，需要同时保存和同时调用
    QList<BCMGroupDisplay *> lstGroupDisplays = pMRoom->GetGroupDisplay();
    QListIterator<BCMGroupDisplay *> itGroupDisplay( lstGroupDisplays );
    while ( itGroupDisplay.hasNext() ) {
        BCMGroupDisplay *pGroupDisplay = itGroupDisplay.next();
        if (NULL == pGroupDisplay)
            continue;

        int nGroupDisplayID = pGroupDisplay->GetGroupDisplayID();

        BCLocalServer *pServer = BCLocalServer::Application();
        pServer->load(nGroupDisplayID, m_id);

        // 判断是否有关联矩阵，如果有需要联控
        if (!BCLocalServer::Application()->isFullScreenMode()) {
            BCMMatrix *pMatrix = BCCommon::Application()->GetMMatrix();
            if (NULL != pMatrix) {
                pMatrix->Load( m_id );
            }
        }
    }

    return true;
}

bool BCMWindowScene::Update()
{
    this->Clear();
    return this->Save();
}

void BCMWindowScene::Clear(bool bConnectServer)
{
    while ( !m_lstData.isEmpty() )
        delete m_lstData.takeFirst();

    // 如果不通信则直接返回
    if ( !bConnectServer )
        return;

    // 删除原数据
    BCMRoom *pMRoom = (NULL == m_pRoom) ? m_pGroupScene->GetRoom() : m_pRoom;
    int nGroupSceneID = (NULL == m_pGroupScene) ? 0 : m_pGroupScene->GetGroupSceneID();

    if ( !BCCommon::g_bConnectWithServer ) {
        BCLocalServer *pServer = BCLocalServer::Application();
        pServer->RemoveScreen(m_id);
    } else {
        // 服务器不需要做操作
    }
}

void BCMWindowScene::SetWindowSceneName(const QString &qs, bool bSave)
{
    m_name = qs;

    if ( bSave ) {
        if ( !BCCommon::g_bConnectWithServer ) {
            // 本地数据库
            BCLocalServer *pServer = BCLocalServer::Application();

            // 更新数据
            if (NULL != m_pGroupScene) {
                pServer->AddScreen(this);
            }
        }
    }
}

void BCMWindowScene::SetWindowSceneLoopInterval(int interval, bool bSave)
{
    m_nLoopInterval = interval;

    if ( bSave ) {
        if ( !BCCommon::g_bConnectWithServer ) {
            // 本地数据库
            BCLocalServer *pServer = BCLocalServer::Application();

            // 更新数据
            if (NULL != m_pGroupScene) {
                pServer->AddScreen(this);
            }
        } else {
            // 服务器不需要做操作
        }
    }
}

void BCMWindowScene::SetIsCycled(bool b, bool bSave)
{
    m_bIsCycled = b;

    if ( bSave ) {
        if ( !BCCommon::g_bConnectWithServer ) {
            // 本地数据库
            BCLocalServer *pServer = BCLocalServer::Application();

            // 更新数据
            if (NULL != m_pGroupScene) {
                pServer->AddScreen(this);
            }
        } else {
            // 服务器不需要做操作
        }
    }
}
