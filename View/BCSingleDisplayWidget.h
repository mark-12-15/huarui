#ifndef BCSINGLEDISPLAYWIDGET_H
#define BCSINGLEDISPLAYWIDGET_H

#include <QWidget>

namespace Ui {
class BCSingleDisplayWidget;
}

class BCMDisplay;
class BCSingleDisplayVirtualWidget;
class BCSingleDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BCSingleDisplayWidget(BCMDisplay *pMDisplay, QWidget *parent = 0);
    ~BCSingleDisplayWidget();

    BCMDisplay *GetMDisplay();

    // 返回单屏内的虚拟矩形
    const QList<BCSingleDisplayVirtualWidget *> &GetSingleDisplayVirtualWidget();

    // 设置分割数
    void RefreshSegmentation(int n);
    int GetSegmentation();

    QString GetDisplayName();

protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *e);

    // 菜单事件
    void contextMenuEvent(QContextMenuEvent *e);

private:
    Ui::BCSingleDisplayWidget *ui;

    BCMDisplay  *m_pMDisplay;

    bool        m_bMatrixMode;      // 是否是矩阵模式

    int         m_nSegmentation;                // 单屏的分割数，可以不与数据同步
                                                // 1 4 6 8 9 12 16

    QList<BCSingleDisplayVirtualWidget *>   m_lstSingleDisplayRect;
};

inline BCMDisplay *BCSingleDisplayWidget::GetMDisplay()
{
    return m_pMDisplay;
}

inline int BCSingleDisplayWidget::GetSegmentation()
{
    return m_nSegmentation;
}

inline const QList<BCSingleDisplayVirtualWidget *> &BCSingleDisplayWidget::GetSingleDisplayVirtualWidget()
{
    return m_lstSingleDisplayRect;
}

#endif // BCSINGLEDISPLAYWIDGET_H
