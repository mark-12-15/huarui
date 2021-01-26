/*********************************************************************************************************************************
* 作    者：liuwl
* 摘    要：通道数据类
*********************************************************************************************************************************/
#ifndef BCMCHANNEL_H
#define BCMCHANNEL_H

#include <QString>
#include <QRectF>
#include <QMap>

class QUdpSocket;
class BCSignalWindowDisplayWidget;
class BCMSubTitlesSceneData;
class BCMGroupChannel;
class BCMGroupDisplay;
class BCMChannel
{
public:
    BCMChannel(int id);
    ~BCMChannel();

    void SetChannelID(int n);
    int GetChannelID();

    void SetChannelBaseName(const QString &qs);
    const QString &GetChannelBaseName();

    void SetChannelName(const QString &qs, bool bSendCmd = false);
    const QString &GetChannelName();

    // 分辨率
    QSize GetResolution();
    void SetResolution(QSize size);

    // 是否可用
    void SetIsEnable(bool b);
    bool IsEnable();

    void SetSignalSource(int n);
    int GetSignalSource();

    void SetChannelType(int type);
    int GetChannelType();

    void SetCutArea(int left, int right, int top, int bottom, bool bSendCmd = false);
    QRect GetCutArea();

    void SetBoardCardID(int id);
    int GetBoardCardID();

    void SetBoardCardPos(int pos);
    int GetBoardCardPos();

    // 设置通道自定义名称和最后一次位置
    void SetChannelCustomName(BCMGroupChannel *pGroupChannel, const QString &qs);
    QString GetChannelCustomName(BCMGroupChannel *pGroupChannel);

    void SetChannelLastRect(BCMGroupDisplay *pGroupDisplay, const QRectF &rect);
    QRectF GetChannelLastRect(BCMGroupDisplay *pGroupDisplay);

    // 在屏组内是否有最后位置
    bool IsHaveLastRect(BCMGroupDisplay *pGroupDisplay);

    // 信号窗显示类
    void AddSignalWindowDisplayWidget(BCSignalWindowDisplayWidget *pItem);
    void RemoveSignalWindowDisplayWidget(BCSignalWindowDisplayWidget *pItem);

    int GetSignalWindowDisplayWidgetCount();

    // VP2000时使用，获得赋值窗口的索引
    int getCopyIndex();

    // 是否允许开窗
    bool IsOpenWindowKey(int groupdisplayid);

    void SendRemoteCmd(const QString &cmd);

protected:
    QRectF  m_rectSignalCuttingRange;   // 信号切割范围

    int m_id;
    QString m_name;         // 允许修改的名字
    QString m_qsBaseName;   // 基础名字

    int m_nChanneltype{0};     // 通道类型(0 PC  1 IPV  2 HDVI 3 Vedio)  WindowID 打开的窗口在设备上的逻辑ID
    int m_nSignalSource;    // 0 VGA  1 DVI  2 CVBS  3 YPbPr  4 HDMI  5 S-V  6 HDVI  7 SDI  8 DP  9 IPV  10 HDBaseT  11 FIBER  注：CVBS 视频  HDVI 高清  其余都是电脑
    int m_nAudoFlag;        // 0 无音频  1 有音频

    QSize m_resolution;     // 输入通道的分辨率
    bool m_bIsEnable;       // 是否可用

    int m_nBoardCardID;     // 板卡ID
    int m_nBoardCardPos;    // 板卡位置

    BCMSubTitlesSceneData   *m_pSubTitles;  // 字幕

    QMap<BCMGroupDisplay *, QRectF>     m_mapLastRect;     // 最后一次开窗位置，每个屏组对应一个矩形
    QMap<BCMGroupChannel *, QString>    m_mapCustomName;   // 用户自定义名称，每个通道组对应一个

    QList<BCSignalWindowDisplayWidget *>  m_lstSignalDisplayWidget; // 信号窗显示类
};

inline void BCMChannel::SetBoardCardID(int id)
{
    m_nBoardCardID = id;
}

inline int BCMChannel::GetBoardCardID()
{
    return m_nBoardCardID;
}

inline void BCMChannel::SetBoardCardPos(int pos)
{
    m_nBoardCardPos = pos;
}

inline int BCMChannel::GetBoardCardPos()
{
    return m_nBoardCardPos;
}

inline void BCMChannel::SetChannelID(int n)
{
    m_id = n;
}

inline int BCMChannel::GetChannelID()
{
    return m_id;
}

inline QSize BCMChannel::GetResolution()
{
    return m_resolution;
}

inline void BCMChannel::SetResolution(QSize size)
{
    m_resolution = size;
}

inline void BCMChannel::SetIsEnable(bool b)
{
    m_bIsEnable = b;
}

inline bool BCMChannel::IsEnable()
{
    return m_bIsEnable;
}

inline void BCMChannel::SetChannelBaseName(const QString &qs)
{
    m_qsBaseName = qs;
}

inline const QString &BCMChannel::GetChannelBaseName()
{
    return m_qsBaseName;
}

inline const QString &BCMChannel::GetChannelName()
{
    return m_name;
}

inline void BCMChannel::SetSignalSource(int n)
{
    m_nSignalSource = n;
}

inline int BCMChannel::GetSignalSource()
{
    return m_nSignalSource;
}

inline void BCMChannel::SetChannelType(int type)
{
    //m_nChanneltype = type;
}

inline int BCMChannel::GetChannelType()
{
    return m_nChanneltype;
}

#endif // BCMCHANNEL_H
