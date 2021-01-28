/*********************************************************************************************************************************
* 作    者：liuwl
* 摘    要：屏幕组数据类
*********************************************************************************************************************************/
#ifndef BCMGROUPDISPLAY_H
#define BCMGROUPDISPLAY_H

#include <QList>
#include <QSizeF>
#include <QRectF>
#include <QMap>

class BCGroupDisplayWidget;
class BCMRoom;
class BCMDisplay;
class BCMInputDevice;
class BCMChannel;
class BCMGroupDisplay
{
public:
    BCMGroupDisplay(BCMRoom *pRoom);
    ~BCMGroupDisplay();

    void SetDisplayWidgetManager(BCGroupDisplayWidget *pManager);
    BCGroupDisplayWidget *GetDisplayWidgetManager();

    BCMRoom *GetRoom();

    void SetGroupDisplayID(int id);
    int GetGroupDisplayID();

    void SetGroupDisplayName(const QString &qs);
    const QString &GetGroupDisplayName();

    void SetRect(QRectF rect);
    QRectF GetRect();

    void SetGroupDisplaySize(double w, double h);
    QSizeF GetGroupDisplaySize();

    const QList<BCMDisplay *> &GetDisplays();
    void AddDisplay(BCMDisplay *pDisplay);
    BCMDisplay *GetDisplay(int id);

    void SetArraySize(int x, int y);
    QSize GetArraySize();

    void SetResolutionSize(int x, int y);
    QSize GetResolutionSize();

    // 根据实际矩形区域，返回单屏ID和单屏内矩形的MAP
    QMap<int, QRect> getDisplayRect(QRect rect);

protected:
    QList<BCMDisplay *> m_lstDisplay;   // 屏幕链表

    BCGroupDisplayWidget *  m_pGroupDisplayWidget;// 屏组显示类

    BCMRoom *m_pRoom;   // 归属房间

    int m_id;
    QString m_name;
    QRectF   m_rect; // 尺寸

    double m_dWidth;
    double m_dHeight;

    int m_nDisplayMode; // 0 标准拼接方式，用户输入标准拼接方式时需要动态创建标准单屏及分割位置  1 非标准拼接方式
    int m_nArrayX;      // 屏幕排列，如4*2
    int m_nArrayY;

    int m_nResolutionX; // 单屏分辨率
    int m_nResolutionY;
};

inline BCMRoom *BCMGroupDisplay::GetRoom()
{
    return m_pRoom;
}

inline void BCMGroupDisplay::SetDisplayWidgetManager(BCGroupDisplayWidget *pManager)
{
    m_pGroupDisplayWidget = pManager;
}

inline BCGroupDisplayWidget *BCMGroupDisplay::GetDisplayWidgetManager()
{
    return m_pGroupDisplayWidget;
}

inline void BCMGroupDisplay::SetGroupDisplayID(int id)
{
    m_id = id;
}

inline int BCMGroupDisplay::GetGroupDisplayID()
{
    return m_id;
}
inline void BCMGroupDisplay::SetGroupDisplayName(const QString &qs)
{
    m_name = qs;
}

inline const QString &BCMGroupDisplay::GetGroupDisplayName()
{
    return m_name;
}

inline void BCMGroupDisplay::SetRect(QRectF rect)
{
    m_rect = rect;
}

inline QRectF BCMGroupDisplay::GetRect()
{
    return m_rect;
}

inline const QList<BCMDisplay *> &BCMGroupDisplay::GetDisplays()
{
    return m_lstDisplay;
}

inline void BCMGroupDisplay::SetGroupDisplaySize(double w, double h)
{
    m_dWidth = w;
    m_dHeight = h;
}

inline QSizeF BCMGroupDisplay::GetGroupDisplaySize()
{
    return QSizeF(m_dWidth, m_dHeight);
}

inline void BCMGroupDisplay::SetArraySize(int x, int y)
{
    m_nArrayX = x;
    m_nArrayY = y;
}

inline QSize BCMGroupDisplay::GetArraySize()
{
    return QSize(m_nArrayX, m_nArrayY);
}

inline void BCMGroupDisplay::SetResolutionSize(int x, int y)
{
    m_nResolutionX = x;
    m_nResolutionY = y;
}

inline QSize BCMGroupDisplay::GetResolutionSize()
{
    return QSize(m_nResolutionX, m_nResolutionY);
}

#endif // BCMGROUPDISPLAY_H
