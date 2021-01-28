#include "BCRibbonMainToolBar.h"

#include "../Main/MainWindow.h"
#include "../Common/BCCommon.h"
#include "BCRibbonMainToolBarAction.h"
#include "../Common/BCLocalServer.h"
#include "../Model/BCMRoom.h"

class BCRibbonPage : public RibbonPage
{
public:
    BCRibbonPage();
};

BCRibbonMainToolBar::BCRibbonMainToolBar()
    :QObject(0)
{
    // 扩展page和group专用
    pExtendPage = NULL;
    pExtendGroup = NULL;

    RefreshMap();
}

BCRibbonMainToolBar::~BCRibbonMainToolBar()
{
}

void BCRibbonMainToolBar::RefreshMap()
{
    m_mapMainButtonTypeName.clear();
    m_mapMainButtonTypeName.insert(DEVICECONNECT, ButtonInfo(QObject::tr("系统连接"), QObject::tr("系统连接"), QString("devicepara32.png")));
    m_mapMainButtonTypeName.insert(WINDOWSCENELOOP, ButtonInfo(QObject::tr("启动轮巡"), QObject::tr("场景轮巡开关"), QString("loopoff132.png"), QString("loopon132.png")));
    m_mapMainButtonTypeName.insert(WINDOWSCENEADD, ButtonInfo(QObject::tr("添加场景"), QObject::tr("添加场景"), QString("addscene32.png")));
    m_mapMainButtonTypeName.insert(WINDOWSCENEDELETE, ButtonInfo(QObject::tr("删除场景"), QObject::tr("删除场景"), QString("deletescene32.png")));
    m_mapMainButtonTypeName.insert(WINDOWSCENESET, ButtonInfo(QObject::tr("轮巡设置"), QObject::tr("轮巡设置"), QString("scenesetoff32.png"), QString("sceneseton32.png")));
    m_mapMainButtonTypeName.insert(LIGHTSET, ButtonInfo(QObject::tr("亮度调节"), QObject::tr("亮度调节"), QString("displayinfo32.png")));
    m_mapMainButtonTypeName.insert(COLORSET, ButtonInfo(QObject::tr("色彩调节"), QObject::tr("色彩调节"), QString("floorplanningsync32.png")));
    m_mapMainButtonTypeName.insert(DISPLAYSWITCH, ButtonInfo(QObject::tr("屏幕开关"), QObject::tr("屏幕开关"), QString("ScreenOn.png"), QString("ScreenOff.png")));
    m_mapMainButtonTypeName.insert(QUIT, ButtonInfo(QObject::tr("退出系统"), QObject::tr("退出系统"), QString("exit32.png")));

    m_mapMainButtonTypeName.insert(AUTHORITY, ButtonInfo(QObject::tr("管理员密码"), QObject::tr("管理员密码"), QString("lisence32.png")));
    m_mapMainButtonTypeName.insert(IMPORTFILE, ButtonInfo(QObject::tr("导入文件"), QObject::tr("导入文件"), QString("edidin32.png")));
    m_mapMainButtonTypeName.insert(EXPORTFILE, ButtonInfo(QObject::tr("导出文件"), QObject::tr("导出文件"), QString("edidout32.png")));
    m_mapMainButtonTypeName.insert(DISPLAYSWITCHCONFIG, ButtonInfo(QObject::tr("屏幕开关设置"), QObject::tr("屏幕开关设置"), QString("displayset32.png")));
    m_mapMainButtonTypeName.insert(MATRIXFORMAT, ButtonInfo(QObject::tr("矩阵设置"), QObject::tr("矩阵设置"), QString("matrix32.png")));
    m_mapMainButtonTypeName.insert(DEVICEFORMAT, ButtonInfo(QObject::tr("设备"), QObject::tr("设备"), QString("runbackground32.png")));
}

void BCRibbonMainToolBar::Build()
{
    if (_init) {
        return;
    }

    // 常用功能 ： 系统连接，添加场景，删除场景，轮巡设置，停止轮巡，亮度调节，色彩调节，屏幕开关，退出系统
    RibbonPage* page1 = BCCommon::Application()->ribbonBar()->addPage("常用功能");
    Qtitan::RibbonGroup* group1 = page1->addGroup("");
    RibbonToolBarControl* toolBar1 = new RibbonToolBarControl(group1);
    toolBar1->addAction(new BCRibbonMainToolBarAction(DEVICECONNECT, m_mapMainButtonTypeName.value(DEVICECONNECT), this), Qt::ToolButtonTextUnderIcon);
    toolBar1->addAction(new BCRibbonMainToolBarAction(WINDOWSCENEADD, m_mapMainButtonTypeName.value(WINDOWSCENEADD), this), Qt::ToolButtonTextUnderIcon);
    toolBar1->addAction(new BCRibbonMainToolBarAction(WINDOWSCENEDELETE, m_mapMainButtonTypeName.value(WINDOWSCENEDELETE), this), Qt::ToolButtonTextUnderIcon);
    toolBar1->addAction(new BCRibbonMainToolBarAction(WINDOWSCENESET, m_mapMainButtonTypeName.value(WINDOWSCENESET), this), Qt::ToolButtonTextUnderIcon);
    toolBar1->addAction(new BCRibbonMainToolBarAction(WINDOWSCENELOOP, m_mapMainButtonTypeName.value(WINDOWSCENELOOP), this), Qt::ToolButtonTextUnderIcon);
    toolBar1->addAction(new BCRibbonMainToolBarAction(LIGHTSET, m_mapMainButtonTypeName.value(LIGHTSET), this), Qt::ToolButtonTextUnderIcon);
    toolBar1->addAction(new BCRibbonMainToolBarAction(COLORSET, m_mapMainButtonTypeName.value(COLORSET), this), Qt::ToolButtonTextUnderIcon);
    toolBar1->addAction(new BCRibbonMainToolBarAction(DISPLAYSWITCH, m_mapMainButtonTypeName.value(DISPLAYSWITCH), this), Qt::ToolButtonTextUnderIcon);
    toolBar1->addAction(new BCRibbonMainToolBarAction(QUIT, m_mapMainButtonTypeName.value(QUIT), this), Qt::ToolButtonTextUnderIcon);
    group1->addControl(toolBar1);

    // 系统设置 ： 管理员密码，导入文件，导出文件，屏幕开关指令设置，添加矩阵，设备（默认隐藏）
    RibbonPage* page2 = BCCommon::Application()->ribbonBar()->addPage("系统设置");
    Qtitan::RibbonGroup* group2 = page2->addGroup("");
    _toolBar2 = new RibbonToolBarControl(group2);
    _toolBar2->addAction(new BCRibbonMainToolBarAction(AUTHORITY, m_mapMainButtonTypeName.value(AUTHORITY), this), Qt::ToolButtonTextUnderIcon);
    _toolBar2->addAction(new BCRibbonMainToolBarAction(IMPORTFILE, m_mapMainButtonTypeName.value(IMPORTFILE), this), Qt::ToolButtonTextUnderIcon);
    _toolBar2->addAction(new BCRibbonMainToolBarAction(EXPORTFILE, m_mapMainButtonTypeName.value(EXPORTFILE), this), Qt::ToolButtonTextUnderIcon);
    _toolBar2->addAction(new BCRibbonMainToolBarAction(DISPLAYSWITCHCONFIG, m_mapMainButtonTypeName.value(DISPLAYSWITCHCONFIG), this), Qt::ToolButtonTextUnderIcon);
    _toolBar2->addAction(new BCRibbonMainToolBarAction(MATRIXFORMAT, m_mapMainButtonTypeName.value(MATRIXFORMAT), this), Qt::ToolButtonTextUnderIcon);
    group2->addControl(_toolBar2);

    _init = true;
}

void BCRibbonMainToolBar::addDevice()
{
    if (!_addDevice)
    {
        _toolBar2->addAction(new BCRibbonMainToolBarAction(DEVICEFORMAT, m_mapMainButtonTypeName.value(DEVICEFORMAT), this), Qt::ToolButtonTextUnderIcon);
        _addDevice = true;
    }
}

BCRibbonMainToolBarAction *BCRibbonMainToolBar::GetButtonAction(BUTTONTYPE type)
{
    return dynamic_cast<BCRibbonMainToolBarAction *>( m_mapButtonAction.value( type ) );
}

QString BCRibbonMainToolBar::GetButtonName(BUTTONTYPE type)
{
    return m_mapMainButtonTypeName.value( type ).m_qsText;
}
