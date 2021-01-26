/*********************************************************************************************************************************
* 作    者：liuwl
* 摘    要：ribbon风格的主工具条
*********************************************************************************************************************************/
#ifndef BCRIBBONMAINTOOLBAR_H
#define BCRIBBONMAINTOOLBAR_H

#include <QObject>
#include <QMap>

#include <QtitanRibbon.h>

class QAction;
class BCRibbonMainToolBarAction;
class BCRibbonMainToolBar : public QObject
{
    Q_OBJECT
public:
    // 主工具条中的button
    enum BUTTONTYPE {
        // 常规模式
        // 主功能
        DEVICECONNECT,          // 系统连接

        WINDOWSCENELOOP,        // 场景轮训开关
        WINDOWSCENEADD,         // 添加场景
        WINDOWSCENEDELETE,      // 删除场景
        WINDOWSCENESET,         // 场景设置

        LIGHTSET,               // 亮度调节
        COLORSET,               // 色彩调节

        DISPLAYSWITCH,          // 屏幕开关
        DISPLAYSWITCHCONFIG,    // 屏幕开关设置

        QUIT,                   // 退出系统

        AUTHORITY,              // 管理员密码
        IMPORTFILE,             // 导入
        EXPORTFILE,             // 导出

        DEVICEFORMAT,           // 设备规模
    };

    // 记录按钮信息，使用类是因为重写构造函数，能在创建时传入多个值
    class ButtonInfo {
    public:
        ButtonInfo(const QString &text = QString::null,
                   const QString &toolTip = QString::null,
                   const QString &iconoff = QString::null,
                   const QString &iconon = QString::null) {
            m_qsText = text;
            m_qsTooltip = toolTip;
            m_qsIconOffPath = iconoff;
            m_qsIconOnPath = iconon;
        }

        QString m_qsIconOffPath;    // 按钮icon off
        QString m_qsIconOnPath;     // 按钮icon on
        QString m_qsText;
        QString m_qsTooltip;
    };

    BCRibbonMainToolBar();
    ~BCRibbonMainToolBar();

    void Build();   // 构造主工具条

    void addDevice();

    // 刷新map
    void RefreshMap();

    BCRibbonMainToolBarAction *GetButtonAction(BUTTONTYPE type);    // 根据类型返回按钮action

    // 根据类型找名字
    QString GetButtonName(BUTTONTYPE type);

private:
    RibbonPage          *pExtendPage;
    Qtitan::RibbonGroup *pExtendGroup;

    QMap<BUTTONTYPE, QAction*>              m_mapButtonAction;  // 定义了按钮类型和对应的action

    QMap<BUTTONTYPE, ButtonInfo>    m_mapMainButtonTypeName;    // 定义类型和名字对应关系，供翻译使用

    RibbonToolBarControl* _toolBar2{nullptr};
    bool _addDevice{false};
};

#endif // BCRIBBONMAINTOOLBAR_H
