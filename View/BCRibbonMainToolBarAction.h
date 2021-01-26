/*********************************************************************************************************************************
* 作    者：liuwl
* 摘    要：ribbon风格的主工具条上的按钮类
*********************************************************************************************************************************/
#ifndef BCRIBBONMAINTOOLBARACTION_H
#define BCRIBBONMAINTOOLBARACTION_H

#include <QAction>
#include "BCRibbonMainToolBar.h"

class BCMRoom;
class BCMGroupScene;
class BCOtherDeviceControlDlg;
class BCSettingOtherDeviceControlDlg;
class BCLocationDlg;
class BCRibbonMainToolBarAction : public QAction
{
    Q_OBJECT

public:
    BCRibbonMainToolBarAction(BCRibbonMainToolBar::BUTTONTYPE eType,
                              const BCRibbonMainToolBar::ButtonInfo &btn,
                              QObject *parent = 0);
    ~BCRibbonMainToolBarAction();

    void RefreshSceneLoop(BCMRoom *pRoom);      // 刷新是否轮训
    void Refresh(BCMRoom *pRoom);               // 刷新是否打开开关
    void RefreshCommunication();                // 刷新通讯状态

    void DestroyLocationDlg();

private slots:
    void onLoopSceneChanged(bool);
    void onDisplaySwitch(); // 屏幕开关

    void onSceneSet(bool);

    void onShowDialog(bool);

private:
    void init();

    BCRibbonMainToolBar::BUTTONTYPE m_eType;
    BCRibbonMainToolBar::ButtonInfo m_btn;
};

#endif // BCRIBBONMAINTOOLBARACTION_H
