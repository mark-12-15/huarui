#include "BCCommon.h"
#include <QMessageBox>

#include "BCXMLManager.h"

BCCommon::BCCommon()
{
}

BCCommon::~BCCommon()
{
    if (NULL != m_pApplication) {
        delete m_pApplication;
        m_pApplication = NULL;
    }
}

MainWindow *BCCommon::Application()
{
    if (NULL == m_pApplication) {
        m_pApplication = new MainWindow();
    }

    return m_pApplication;
}
/* 设置系统皮肤类
 * 皮肤数据从xml中获得，需要设置如下皮肤
 * 1.工具条背景色，工具条列表可直接从m_pApplication中获得
 * 2.显示墙内皮肤，如单个显示器背景色、信号窗颜色、字体位置等等，设置完成后需要调用WallView->Refresh接口，WallView列表可以从m_pApplication中获得
 * 3.按钮样式，按钮列表可以从主工具条获得
 * 4.按钮对应的action的样式，在按钮内设置
 */
#include <QLinearGradient>
#include "../View/BCRibbonMainToolBar.h"
void BCCommon::SetApplicationSkin(const QString &qsStyle)
{
    // 如果系统还未初始化则直接返回
    if (NULL == m_pApplication)
        return;

    // 得到对应文件名
    QString qsSkinFileName = QString::null;
    if (QString("colorfulStyle") == qsStyle)
        qsSkinFileName = "../xml/SkinDefault.xml";
    else if (QString("retroStyle") == qsStyle)
        qsSkinFileName = "../xml/SkinDefault.xml";
    else if (QString("grayStyle") == qsStyle)
        qsSkinFileName = "../xml/SkinDefault.xml";
    else if (QString("defaultStyle") == qsStyle)
        qsSkinFileName = "../xml/SkinDefault.xml";
    else
        qsSkinFileName = QString("../xml/Skin%1.xml").arg(qsStyle);

    // 如果不是已存在皮肤则直接返回
    if (qsSkinFileName.isEmpty())
        return;

    // 赋值皮肤类型
    g_qsApplicationStyle = qsStyle;

    // 构造xml manager
    BCXMLManager xml;
    if ( !xml.IsExistXmlFile( qsSkinFileName ) )
        return;

    // 单个显示器显示控制
    g_nSingleDisplayLineColorR = xml.GetAttribute("SingleDisplay", "lineR").toInt();
    g_nSingleDisplayLineColorG = xml.GetAttribute("SingleDisplay", "lineG").toInt();
    g_nSingleDisplayLineColorB = xml.GetAttribute("SingleDisplay", "lineB").toInt();
    g_nSingleDisplayLineColorA = xml.GetAttribute("SingleDisplay", "lineA").toInt();
    g_nSingleDisplayLineWidth = xml.GetAttribute("SingleDisplay", "lineWidth").toInt();
    g_nSingleDisplayLineStyle = xml.GetAttribute("SingleDisplay", "lineStyle").toInt();
    g_nSingleDisplayFillColorR = xml.GetAttribute("SingleDisplay", "fillR").toInt();
    g_nSingleDisplayFillColorG = xml.GetAttribute("SingleDisplay", "fillG").toInt();
    g_nSingleDisplayFillColorB = xml.GetAttribute("SingleDisplay", "fillB").toInt();
    g_nSingleDisplayFillColorA = xml.GetAttribute("SingleDisplay", "fillA").toInt();

    // 单屏内虚框显示控制
    g_nVirtualRectFillColorR = xml.GetAttribute("VirtualRect", "fillR").toInt();
    g_nVirtualRectFillColorG = xml.GetAttribute("VirtualRect", "fillG").toInt();
    g_nVirtualRectFillColorB = xml.GetAttribute("VirtualRect", "fillB").toInt();
    g_nVirtualRectFillColorA = xml.GetAttribute("VirtualRect", "fillA").toInt();

    // 信号窗主体线条及背景填充色设置
    g_nSignalWindowBodyLineColorR = xml.GetAttribute("SignalWindowBody", "lineR").toInt();
    g_nSignalWindowBodyLineColorG = xml.GetAttribute("SignalWindowBody", "lineG").toInt();
    g_nSignalWindowBodyLineColorB = xml.GetAttribute("SignalWindowBody", "lineB").toInt();
    g_nSignalWindowBodyLineColorA = xml.GetAttribute("SignalWindowBody", "lineA").toInt();
    g_nSignalWindowBodyFillColorR = xml.GetAttribute("SignalWindowBody", "fillR").toInt();
    g_nSignalWindowBodyFillColorG = xml.GetAttribute("SignalWindowBody", "fillG").toInt();
    g_nSignalWindowBodyFillColorB = xml.GetAttribute("SignalWindowBody", "fillB").toInt();
    g_nSignalWindowBodyFillColorA = xml.GetAttribute("SignalWindowBody", "fillA").toInt();
}

void BCCommon::SetSystemFont(QWidget *pWidget, bool bBold)
{
    if (NULL == pWidget)
        return;

    QFont font = pWidget->font();
    font.setBold( bBold );
    font.setFamily( BCCommon::g_qsDefaultFontFamily );
    font.setPixelSize( BCCommon::g_nDefaultFontPixelSize );
    pWidget->setFont( font );
}

void BCCommon::SetSystemFont(QAction *pAction, bool bBold)
{
    if (NULL == pAction)
        return;

    QFont font = pAction->font();
    font.setBold( bBold );
    font.setFamily( BCCommon::g_qsDefaultFontFamily );
    font.setPixelSize( BCCommon::g_nDefaultFontPixelSize );
    pAction->setFont( font );
}

MainWindow *BCCommon::m_pApplication = NULL;

// 网络连接属性
QString BCCommon::g_qsConnectName = QString::null;
bool BCCommon::g_bConnectStatusOK = false;
bool BCCommon::g_bConnectWithServer = false;
//bool BCCommon::g_bGroupScene = false;
bool BCCommon::g_bSignalWindowCopy = true;
int BCCommon::g_nSignalWindowCopyCount = 2;
int BCCommon::g_nSignalWindowCopyCountOfVideo = 2;
int BCCommon::g_nMonitorCount = -1;
int BCCommon::g_nDeviceType = 0;
bool BCCommon::g_bDefaultWindowOffset = true;
double BCCommon::g_dWallDisplayWidthHeightRatio = 16.0 / 9.0;

int BCCommon::g_nIsContainsMatrix = -1;
//bool BCCommon::g_bSignalWindowByWidget = true;
QString BCCommon::g_qsPreviewIP = "";
int BCCommon::g_nPreviewPort = 8206;
QString BCCommon::g_qsDeviceType2000 = "VP2000";
QString BCCommon::g_qsDeviceType2000A = "VP2000A";
QString BCCommon::g_qsDeviceType4000 = "VP4000";
QString BCCommon::g_qsDeviceTypeMatrix = "Matrix";

int BCCommon::g_nIsUseInputChannelConfig = 0;
int BCCommon::g_npcCount = 0;
int BCCommon::g_nipvCount = 0;
int BCCommon::g_nvedioCount = 0;
int BCCommon::g_nhdCount = 0;
int BCCommon::g_npcBeginID = 0;
int BCCommon::g_nipvBeginID = 0;
int BCCommon::g_nvedioBeginID = 0;
int BCCommon::g_nhdBeginID = 0;
bool BCCommon::g_bContainsRemote = true;

// 工程属性设置
QString BCCommon::g_qsApplicationVersion = QString("V3.11.58 5.9-180815");
//QString BCCommon::g_qsApplicationVersion = QString("  V5.2  ");
QString BCCommon::g_qsApplicationTitle = QString("控制平台");
QString BCCommon::g_qsApplicationCompany = QString("博睿科技");
QString BCCommon::g_qsApplicationIcon = QString("titleicon.ico");
bool BCCommon::g_bApplicationQuitToTrayIcon = false;
int BCCommon::g_bApplicationDefaultDisplayMode = 2;    // 应用启动时显示模式:0：Nomarl需指定宽高；1：最小化显示；2：最大化显示；3：全屏显示
int BCCommon::g_bApplicationNomarlDisplayWidth = 800;    // nomarl显示的宽
int BCCommon::g_bApplicationNomarlDisplayHeight = 600;
QString BCCommon::g_qsTabWidgetStyle =
        "QTabWidget{background-color: rgb(0, 0, 0, 125);}\
        QTabWidget::pane{border-style: outset;border-top: 1px solid #C2C7CB;}\
        QTabWidget::tab-bar{alignment:left;background:rgb(0, 0, 0, 125);}\
        QTabBar::tab{background:rgb(0, 0, 0, 0);color:rgb(0, 0, 0, 255);min-width:20ex;min-height:8ex;border-top-left-radius:3px;border-top-right-radius:3px;padding:6px;}\
        QTabBar::tab:hover{background:rgb(255, 255, 255, 125);color:rgb(0, 0, 0, 255);margin-top:2px;margin-left:2px;margin-right:2px;}\
        QTabBar::tab:selected{border-color:rgb(255, 255, 255, 255);background:rgb(255, 255, 255, 255);color:rgb(0, 0, 0, 255);margin-top:2px;margin-left:2px;margin-right:2px;}";              // tabwidget使用的风格
QString BCCommon::g_qsDefaultFontFamily = QString("微软雅黑");   // 这里不做翻译
int BCCommon::g_nDefaultFontPixelSize = 12;
QString BCCommon::g_qsImageFilePrefix = QString("../resource/image/");
QString BCCommon::g_qsApplicationStyle = QString("defaultStyle");
QString BCCommon::g_qsApplicationDefaultStylePath = BCCommon::g_qsImageFilePrefix+BCCommon::g_qsApplicationStyle;
QString BCCommon::g_qInputChannelPath = BCCommon::g_qsImageFilePrefix+QString("channel/");
QString BCCommon::g_qOtherDevicePath = BCCommon::g_qsImageFilePrefix+QString("otherdevice/");
QString BCCommon::g_qPlayerPath = BCCommon::g_qsImageFilePrefix+QString("player/");

// 托盘设置
bool BCCommon::g_bShowTrayIcon = true;                    // 是否显示托盘
QString BCCommon::g_qsIconPathOfTrayIcon = QString("trayBr16.png");          // 托盘图标路径
QString BCCommon::g_qsTrayIconTooltips = QString("display platform");            // 托盘默认提示内容
QString BCCommon::g_qsTrayIconShowActionIconPath = QString("trayshow16.png");          // 显示图标路径
QString BCCommon::g_qsTrayIconSetActionIconPath = QString("trayset16.png");           // 设置图标路径
QString BCCommon::g_qsTrayIconAboutActionIconPath = QString("trayabout16.png");         // 关于图标路径
QString BCCommon::g_qsTrayIconCheckUpdateActionIconPath = QString("traycheckupdate16.png");   // 检查更新图标路径
QString BCCommon::g_qsTrayIconLogOffActionIconPath = QString("traylogoff16.png");        // 注销图标路径
QString BCCommon::g_qsTrayIconQuitActionIconPath = QString("trayquit16.png");          // 退出图标路径

// 屏幕组全局参数
double BCCommon::g_dDisplayWallMaxScaleValue = 3;
int BCCommon::g_nMaxSizeOfModifyRect = 12;
bool BCCommon::g_bOpenSignalWindowSorption = true;        // 是否开启信号窗吸附功能
int BCCommon::g_nSignalWindowSorptionWidth = 15;

// 单个显示器显示控制
int BCCommon::g_nSingleDisplayLineColorR = 255;         // 单个显示屏边框颜色RGB及透明度
int BCCommon::g_nSingleDisplayLineColorG = 0;
int BCCommon::g_nSingleDisplayLineColorB = 0;
int BCCommon::g_nSingleDisplayLineColorA = 125;
int BCCommon::g_nSingleDisplayLineWidth = 1;            // 单个显示屏边框线条宽度
int BCCommon::g_nSingleDisplayLineStyle = 1;            // 单个显示屏边框线条类型，取值范围0-6，0为无线条，1为实线，其他均为虚线；对应Qt::PenStyle
int BCCommon::g_nSingleDisplayFillColorR = 221;         // 单个显示屏填充颜色RGB及透明度
int BCCommon::g_nSingleDisplayFillColorG = 205;
int BCCommon::g_nSingleDisplayFillColorB = 68;
int BCCommon::g_nSingleDisplayFillColorA = 125;
int BCCommon::g_nSingleDisplayRectRadius = 0;

// 单屏内虚框显示控制
int BCCommon::g_nVirtualRectFillColorR = 90;            // 虚框颜色RGB及透明度
int BCCommon::g_nVirtualRectFillColorG = 173;
int BCCommon::g_nVirtualRectFillColorB = 255;
int BCCommon::g_nVirtualRectFillColorA = 0;

// 信号窗全局变量
int BCCommon::g_nMinResizeSignalWindowSize = 50;
int BCCommon::g_nMinResizeSignalWindowSizeW = 50; //150;
int BCCommon::g_nMinResizeSignalWindowSizeH = 50;
int BCCommon::g_nMinVirtualSizeOfCreateSignalWindow = 25;
QString BCCommon::g_qsSignalWindowMenuActionLocationIconPath = QString("actionlocation16.png");
QString BCCommon::g_qsSignalWindowMenuActionEchoIconPath = QString("actionecho16.png");
QString BCCommon::g_qsSignalWindowMenuActionCutIconPath = QString("actioncut16.png");
QString BCCommon::g_qsSignalWindowMenuActionTopIconPath = QString("actiontop16.png");
QString BCCommon::g_qsSignalWindowMenuActionBottomIconPath = QString("actionbottom16.png");
QString BCCommon::g_qsSignalWindowMenuActionMoveToTopIconPath = QString("actionmovetotop16.png");
QString BCCommon::g_qsSignalWindowMenuActionMoveToBottomIconPath = QString("actionmovetobottom16.png");
QString BCCommon::g_qsSignalWindowMenuActionLockIconPath = QString("actionlock16.png");
QString BCCommon::g_qsSignalWindowMenuActionScaleToSingleDisplayIconPath = QString("actionscaletosingledisplay16.png");
QString BCCommon::g_qsSignalWindowMenuActionScaleToOverlapDisplayIconPath = QString("actionscaletooverlapdisplay16.png");
QString BCCommon::g_qsSignalWindowMenuActionScaleToAllDisplayIconPath = QString("actionscaletoalldisplay.png");
QString BCCommon::g_qsSignalWindowMenuActionCloseIconPath = QString("actionclose16.png");
QString BCCommon::g_qsSignalWindowMenuActionAttributeIconPath = QString("actionattribute16.png");

// 信号窗标题设置
QString BCCommon::g_qsSignalWindowTitleLockButtonImagePath = QString("lock.png");        // 开窗标题锁定按钮图片路径
QString BCCommon::g_qsSignalWindowTitleUnLockButtonImagePath = "unlock.png";      // 开窗标题非锁定按钮图片路径
QString BCCommon::g_qsSignalWindowTitleFullScreenButtonImagePath = "Maximized.png";  // 开窗标题全屏按钮图片路径
QString BCCommon::g_qsSignalWindowTitleUnFullScreenButtonImagePath = "Normal.png";// 开窗标题非全屏按钮图片路径
QString BCCommon::g_qsSignalWindowTitleCloseButtonImagePath = "close.png";       // 开窗标题关闭按钮图片路径

// 信号窗主体线条及背景填充色设置
int BCCommon::g_nSignalWindowBodyLineColorR = 144;          // 单个显示屏边框颜色RGB及透明度
int BCCommon::g_nSignalWindowBodyLineColorG = 208;
int BCCommon::g_nSignalWindowBodyLineColorB = 240;
int BCCommon::g_nSignalWindowBodyLineColorA = 125;
int BCCommon::g_nSignalWindowBodyLineWidth = 1;           // 单个显示屏边框线条宽度
int BCCommon::g_nSignalWindowBodyLineStyle = 1;           // 单个显示屏边框线条类型，取值范围0-6，0为无线条，1为实线，其他均为虚线；对应Qt::PenStyle
int BCCommon::g_nSignalWindowBodyFillColorR = 144;          // 单个显示屏填充颜色RGB及透明度
int BCCommon::g_nSignalWindowBodyFillColorG = 208;
int BCCommon::g_nSignalWindowBodyFillColorB = 240;
int BCCommon::g_nSignalWindowBodyFillColorA = 125;

double BCCommon::g_dPermissionError = 0.01;
