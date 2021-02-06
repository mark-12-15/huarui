#ifndef BCSIGNALWINDOWDISPLAYWIDGET_H
#define BCSIGNALWINDOWDISPLAYWIDGET_H

#include <QWidget>

namespace Ui {
class BCSignalWindowDisplayWidget;
}

class BCMChannel;
class BCRoomMainWidget;
class BCGroupDisplayWidget;
class BCSignalWindowDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BCSignalWindowDisplayWidget(BCGroupDisplayWidget *pGroupDisplayWidget, int x, int y, int w, int h, BCMChannel *pChannel, int winid, BCRoomMainWidget *pSignalWindowMgr, bool bSendCmd = false);
    ~BCSignalWindowDisplayWidget();

    // 返回实际尺寸
    QRect GetFactRect();

    // 重置矩形框尺寸，坐标为父窗口坐标;
    // bSendCmd表示是否发送指令;
    // bReSend在吸附时使用，防止指令丢失;
    // bMapToFact表示是否进行坐标转换，因为精确定位时不需要进行转换，可能会出现误差
    void ResizeRect(int x, int y, int w, int h, bool bSendCmd = false, bool bReSend = false, bool bMapToFact = true);

    // 服务器同意控制
    void ServerRequestControlResult();

    // 服务器对关窗请求的回复
    void ServerWinSwitchResult(bool b);

    // 刷新信号窗文字
    void RefreshTextDisplay();

    // 返回归属屏组
    BCGroupDisplayWidget *GetGroupDisplay();

    // 设置信号窗输入通道
    void SetInputChannel(BCMChannel *pChannel);
    BCMChannel *GetInputChannel();

    void SetWindowID(int id);
    int GetWindowID();

    // 设置是否加锁
    void SetLock(bool bLock);
    bool IsLocked();

    // 设置全屏
    bool IsFullScene();
    void SetFullScene(bool b);
    // 缩放到单屏
    void ScaleToSingleDisplay();
    // 缩放到所占屏（铺满当前屏）
    void ScaleToOverlapDisplay();

    // 设置信号窗属性
    void SetSignalWindowProperty();

    void SetSignalWindowTitle(const QString &title);
    QString GetSignalWindowTitle();

    // 信号窗属性菜单设置，参数需要进行转换
    void SetSignalWindowResize(int x, int y, int w, int h, bool bSendCmd = true);

    // 通讯接口
    void Winsize();

    bool    m_bPress;                   // 鼠标左键单击事件需要记录的值

protected:
    // 根据吸附值resize rect
    void ResizeRectBySorption();

    /* 修改信号窗相对位置
     * 0：置顶
     * 1：置底
     * 2：上移
     * 3：下移
     * 4：关闭
     */
    void SetSignalPosition(int type);

    // 返回当前通道
    QString GetCurrentInputChannel();

    // 处理悬停事件
    bool event(QEvent *event);

    // 菜单事件
    void contextMenuEvent(QContextMenuEvent *e);

    // 鼠标事件拖动内部窗体
    void mousePressEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseDoubleClickEvent(QMouseEvent *event);

    //void closeEvent(QCloseEvent *e);
    void paintEvent(QPaintEvent *e);

    // 修改矩形的起始点
    enum RESIZEPOS {
        UNRESIZE,   // 不可拉伸
        RESIZELT,   // 左上角
        RESIZEL,    // 左侧
        RESIZELB,   // 左下角
        RESIZET,    // 上
        RESIZEB,    // 下
        RESIZERT,   // 右上角
        RESIZER,    // 右
        RESIZERB    // 右下角
    };

private slots:
    void on_m_pLockBtn_clicked();

    void on_m_pFullscreenBtn_clicked();

    void on_m_pWindowShowBtn_clicked();

    void on_m_pCloseBtn_clicked();

    void onConstructionTimeout();
    void onDestructionTimeout();

private:
    Ui::BCSignalWindowDisplayWidget *ui;

    QRect                   m_rectFact;                 // 实际尺寸
    BCRoomMainWidget        *m_pSignalWindowMgr;        // 信号窗管理类
    BCMChannel              *m_pInputChannel;           // 输入通道数据类
    BCGroupDisplayWidget    *m_pGroupDisplayWidget;     // 归属屏组
    int                     m_nWindowID;                // 窗口ID
    int                     m_nCopyIndex;               // 只对VP2000生效，默认-1，从0开始编号

    double  m_x;                    // 鼠标左键单击事件需要记录的值
    double  m_y;

    bool    m_bLock;                // 移动及缩放锁
    bool    m_bFullScene;           // 是否全屏
    QRectF  m_rectBeforeFullScene;  // 全屏前的尺寸
    int     m_transparent;          // 透明度

    RESIZEPOS m_eResizePos;
};

inline QRect BCSignalWindowDisplayWidget::GetFactRect()
{
    return m_rectFact;
}

inline bool BCSignalWindowDisplayWidget::IsLocked()
{
    return m_bLock;
}

inline bool BCSignalWindowDisplayWidget::IsFullScene()
{
    return m_bFullScene;
}

inline BCMChannel *BCSignalWindowDisplayWidget::GetInputChannel()
{
    return m_pInputChannel;
}

inline BCGroupDisplayWidget *BCSignalWindowDisplayWidget::GetGroupDisplay()
{
    return m_pGroupDisplayWidget;
}

inline void BCSignalWindowDisplayWidget::SetWindowID(int id)
{
    m_nWindowID = id;
}

inline int BCSignalWindowDisplayWidget::GetWindowID()
{
    return m_nWindowID;
}

#endif // BCSIGNALWINDOWDISPLAYWIDGET_H
