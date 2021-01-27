/*********************************************************************************************************************************
* 作    者：liuwl
* 摘    要：全局类，记录共有设置信息及API函数
*********************************************************************************************************************************/
#ifndef BCCOMMON_H
#define BCCOMMON_H

#include <QObject>
#include <QApplication>
#include <QtGui>
#include <QTextCodec>
#include <QDesktopWidget>
#include "../Main/MainWindow.h"

class BCCommon
{
public:
    BCCommon();
    ~BCCommon();

    // mainwindow
    static MainWindow *Application();

    // 设置应用皮肤
    static void SetApplicationSkin(const QString &qsStyle);

    // 系统字体设置
    static void SetSystemFont(QWidget *pWidget, bool bBold = false);
    static void SetSystemFont(QAction *pAction, bool bBold = false);

    // ******************************************************************************************************************************* begin
    // 通讯
    static QString g_qsConnectName;                 // 连接名称，默认为空，当双网卡判断不出来使用哪个网卡时使用该字段
    static bool g_bConnectStatusOK;                 // 连接状态
    static bool g_bConnectWithServer;               // 是否连接服务器
    //static bool g_bGroupScene;                      // 是否显示场景组，默认不显示
    static bool g_bSignalWindowCopy;                // 是否支持信号窗复制
    static int g_nSignalWindowCopyCount;            // 允许信号窗复制的个数，专指PC
    static int g_nSignalWindowCopyCountOfVideo;     // 允许信号窗复制的个数，专指Video
    static int g_nMonitorCount;                     // 预监窗口的数量，-1为输入通道的数量
    static int g_nDeviceType;                       // 设备类型，0：VP2000,1:VP2000A,2:VP4000,3:矩阵

    static bool g_bDefaultWindowOffset;             // 获取默认窗口时是否有误差

    static QString g_qsDeviceType2000;              // 设备类型对应名称
    static QString g_qsDeviceType2000A;
    static QString g_qsDeviceType4000;
    static QString g_qsDeviceTypeMatrix;

    static int g_nIsUseInputChannelConfig;          // 使用配置文件内的输入通道
    static int g_npcCount;
    static int g_nipvCount;
    static int g_nvedioCount;
    static int g_nhdCount;
    static int g_npcBeginID;
    static int g_nipvBeginID;
    static int g_nvedioBeginID;
    static int g_nhdBeginID;

    static bool g_bContainsRemote;                  // 是否包含穿透

    // 工程属性设置
    static QString g_qsApplicationVersion;          // 工程版本号
    static QString g_qsApplicationTitle;            // 工程标题
    static QString g_qsApplicationCompany;          // 工程标题
    static QString g_qsApplicationIcon;             // 工程图标
    static bool g_bApplicationQuitToTrayIcon;       // 系统是否退出到托盘
    static int g_bApplicationDefaultDisplayMode;    // 应用启动时显示模式:0：Nomarl需指定宽高；1：最小化显示；2：最大化显示；3：全屏显示
    static int g_bApplicationNomarlDisplayWidth;    // nomarl显示的宽
    static int g_bApplicationNomarlDisplayHeight;   // nomarl显示的高
    static QString g_qsTabWidgetStyle;              // tabwidget使用的风格
    static QString g_qsDefaultFontFamily;           // 系统默认使用的字体
    static int g_nDefaultFontPixelSize;             // 系统默认使用的字体大小
    static QString g_qsImageFilePrefix;             // 系统使用image的前缀
    static QString g_qsApplicationStyle;            // 系统使用image的风格
    static QString g_qsApplicationDefaultStylePath; // 系统使用image的风格
    static QString g_qInputChannelPath;             // 系统使用的输入通道路径
    static QString g_qOtherDevicePath;             // 系统使用的输入通道路径
    static QString g_qPlayerPath;                   // 系统使用的录像播放路径

    static double g_dWallDisplayWidthHeightRatio;   // 显示比例

    // ******************************************************************************************************************************* begin
    // 托盘显示设置
    static bool g_bShowTrayIcon;                    // 是否显示托盘
    static QString g_qsIconPathOfTrayIcon;          // 托盘图标路径
    static QString g_qsTrayIconTooltips;            // 托盘默认提示内容
    static QString g_qsTrayIconShowActionIconPath;          // 显示图标路径
    static QString g_qsTrayIconSetActionIconPath;           // 设置图标路径
    static QString g_qsTrayIconAboutActionIconPath;         // 关于图标路径
    static QString g_qsTrayIconCheckUpdateActionIconPath;   // 检查更新图标路径
    static QString g_qsTrayIconLogOffActionIconPath;        // 注销图标路径
    static QString g_qsTrayIconQuitActionIconPath;          // 退出图标路径
    // ******************************************************************************************************************************* end

    // ******************************************************************************************************************************* begin
    // 屏幕组全局参数
    static double g_dDisplayWallMaxScaleValue;      // 显示墙最大被放大倍数
    static int g_nMaxSizeOfModifyRect;              // 修改矩形框时鼠标距离边框的最大距离(如通过矩形左上角拉伸窗体，以左上角为圆心，该值为半径范围内都可进行操作)
    static bool g_bOpenSignalWindowSorption;        // 是否开启信号窗吸附功能
    static int g_nSignalWindowSorptionWidth;        // 吸附值

    // 单个显示器显示控制
    static int g_nSingleDisplayLineColorR;          // 单个显示屏边框颜色RGB及透明度
    static int g_nSingleDisplayLineColorG;
    static int g_nSingleDisplayLineColorB;
    static int g_nSingleDisplayLineColorA;
    static int g_nSingleDisplayLineWidth;           // 单个显示屏边框线条宽度
    static int g_nSingleDisplayLineStyle;           // 单个显示屏边框线条类型，取值范围0-6，0为无线条，1为实线，其他均为虚线；对应Qt::PenStyle
    static int g_nSingleDisplayFillColorR;          // 单个显示屏填充颜色RGB及透明度
    static int g_nSingleDisplayFillColorG;
    static int g_nSingleDisplayFillColorB;
    static int g_nSingleDisplayFillColorA;
    static int g_nSingleDisplayRectRadius;          // 单个显示屏圆角值，0为无圆角

    // 单屏内虚框显示控制
    static int g_nVirtualRectFillColorR;            // 虚框颜色RGB及透明度
    static int g_nVirtualRectFillColorG;
    static int g_nVirtualRectFillColorB;
    static int g_nVirtualRectFillColorA;

    // 信号窗全局变量
    static int g_nMinResizeSignalWindowSize;                // 信号窗的宽高最小值
    static int g_nMinResizeSignalWindowSizeW;                // 信号窗的宽高最小值
    static int g_nMinResizeSignalWindowSizeH;                // 信号窗的宽高最小值
    static int g_nMinVirtualSizeOfCreateSignalWindow;       // 创建信号窗的最小虚拟矩形尺寸
    static QString g_qsSignalWindowMenuActionLocationIconPath;          // 信号窗右键快速定位菜单图片路径
    static QString g_qsSignalWindowMenuActionEchoIconPath;              // 信号窗右键回显菜单图片路径
    static QString g_qsSignalWindowMenuActionCutIconPath;               // 信号窗右键裁剪菜单图片路径
    static QString g_qsSignalWindowMenuActionTopIconPath;               // 信号窗右键裁剪菜单图片路径
    static QString g_qsSignalWindowMenuActionBottomIconPath;            // 信号窗右键裁剪菜单图片路径
    static QString g_qsSignalWindowMenuActionMoveToTopIconPath;         // 信号窗右键裁剪菜单图片路径
    static QString g_qsSignalWindowMenuActionMoveToBottomIconPath;      // 信号窗右键裁剪菜单图片路径
    static QString g_qsSignalWindowMenuActionLockIconPath;      // 信号窗右键裁剪菜单图片路径
    static QString g_qsSignalWindowMenuActionScaleToSingleDisplayIconPath;      // 信号窗右键裁剪菜单图片路径
    static QString g_qsSignalWindowMenuActionScaleToOverlapDisplayIconPath;      // 信号窗右键裁剪菜单图片路径
    static QString g_qsSignalWindowMenuActionScaleToAllDisplayIconPath;      // 信号窗右键裁剪菜单图片路径
    static QString g_qsSignalWindowMenuActionCloseIconPath;      // 信号窗右键裁剪菜单图片路径
    static QString g_qsSignalWindowMenuActionAttributeIconPath;      // 信号窗右键裁剪菜单图片路径

    // 信号窗标题设置
    static QString g_qsSignalWindowTitleLockButtonImagePath;        // 开窗标题锁定按钮图片路径
    static QString g_qsSignalWindowTitleUnLockButtonImagePath;      // 开窗标题非锁定按钮图片路径
    static QString g_qsSignalWindowTitleFullScreenButtonImagePath;  // 开窗标题全屏按钮图片路径
    static QString g_qsSignalWindowTitleUnFullScreenButtonImagePath;// 开窗标题非全屏按钮图片路径
    static QString g_qsSignalWindowTitleCloseButtonImagePath;       // 开窗标题关闭按钮图片路径

    // 信号窗主体线条及背景填充色设置
    static int g_nSignalWindowBodyLineColorR;          // 单个显示屏边框颜色RGB及透明度
    static int g_nSignalWindowBodyLineColorG;
    static int g_nSignalWindowBodyLineColorB;
    static int g_nSignalWindowBodyLineColorA;
    static int g_nSignalWindowBodyLineWidth;           // 单个显示屏边框线条宽度
    static int g_nSignalWindowBodyLineStyle;           // 单个显示屏边框线条类型，取值范围0-6，0为无线条，1为实线，其他均为虚线；对应Qt::PenStyle
    static int g_nSignalWindowBodyFillColorR;          // 单个显示屏填充颜色RGB及透明度
    static int g_nSignalWindowBodyFillColorG;
    static int g_nSignalWindowBodyFillColorB;
    static int g_nSignalWindowBodyFillColorA;

    // ******************************************************************************************************************************* end
    // ******************************************************************************************************************************* begin
    static double g_dPermissionError;       // double值计算时允许的误差
    // ******************************************************************************************************************************* end

    // ******************************************************************************************************************************* begin
    // 公共API

    //设置为开机启动
    static void AutoRunWithSystem(bool IsAutoRun, QString AppName, QString AppPath)
    {
        QSettings *reg = new QSettings(
            "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
            QSettings::NativeFormat);

        if (IsAutoRun) {
            reg->setValue(AppName, AppPath);
        } else {
            reg->setValue(AppName, "");
        }
    }

    //设置皮肤样式
    //参数为资源名称
    //SetStyle("blue");
    static void SetStyle(const QString &styleName)
    {
        QFile file(QString(":/css/%1.css").arg(styleName));
        file.open(QFile::ReadOnly);
        QString qss = QLatin1String(file.readAll());
        qApp->setStyleSheet(qss);
        qApp->setPalette(QPalette(QColor("#F0F0F0")));
    }

    static char* getTextStr(QString string)
    {
//        QByteArray ba = string.toLatin1();
//        char* text = strdup(ba.data()); //直接拷贝出来就不会乱码了
//        return text;

        const char *pSrc = string.toStdString().c_str();
        return strdup( pSrc );
    }

    // ******************************************************************************************************************************* end

private:
    static MainWindow *m_pApplication;
};
#endif // BCCOMMON_H
