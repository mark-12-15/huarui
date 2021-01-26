#include "BCSingleDisplayWidget.h"
#include "ui_BCSingleDisplayWidget.h"
#include "../Model/BCMDisplay.h"
#include "../Model/BCMGroupDisplay.h"
#include "../Model/BCMRoom.h"
#include "../Common/BCCommon.h"
#include "BCSingleDisplayVirtualWidget.h"
#include "BCSignalName.h"

BCSingleDisplayWidget::BCSingleDisplayWidget(BCMDisplay *pMDisplay, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BCSingleDisplayWidget)
{
    ui->setupUi(this);

    m_pMDisplay = pMDisplay;
    m_nSegmentation = -1;

    // 房间类型如果是矩阵，则标签显示在下面中间位置，否则在左上角
    m_bMatrixMode = false;
    BCMGroupDisplay *pMGroupDisplay = pMDisplay->GetMGroupDisplay();
    if (NULL != pMGroupDisplay) {
        BCMRoom *pMRoom = pMGroupDisplay->GetRoom();
        if (NULL != pMRoom) {
            if (4 == pMRoom->GetType())
                m_bMatrixMode = true;
        }
    }

//    Qt::Alignment alignment = m_bMatrixMode ? (Qt::AlignHCenter | Qt::AlignBottom) : (Qt::AlignLeft | Qt::AlignTop);
//    ui->m_pNameLabel->setAlignment( alignment );
//    ui->m_pNameLabel->setText( m_pMDisplay->GetDisplayName() );

    // init singledisplayrect
    RefreshSegmentation( m_pMDisplay->GetSegmentation() );
}

BCSingleDisplayWidget::~BCSingleDisplayWidget()
{
    while ( !m_lstSingleDisplayRect.isEmpty() )
        delete m_lstSingleDisplayRect.takeFirst();

    delete ui;
}

QString BCSingleDisplayWidget::GetDisplayName()
{
    return m_pMDisplay->GetDisplayName();
}

void BCSingleDisplayWidget::RefreshSegmentation(int n)
{
    if (n == m_nSegmentation)
        return;

    // 1.赋值分割数
    m_nSegmentation = n;

    // 2.删除所有虚拟矩形
    while ( !m_lstSingleDisplayRect.isEmpty() )
        delete m_lstSingleDisplayRect.takeFirst();

    // 3.根据分割数确定行列数
    int r = 1;
    int c = 1;
    switch ( m_nSegmentation ) {
    case 1:
        r = 1;
        c = 1;
        break;
    case 4:
        r = 2;
        c = 2;
        break;
    case 6:
        r = 2;
        c = 3;
        break;
    case 8:
        r = 2;
        c = 4;
        break;
    case 9:
        r = 3;
        c = 3;
        break;
    case 12:
        r = 3;
        c = 4;
        break;
    default:
        r = 4;
        c = 4;
        break;
    }

    // 单个矩形的尺寸
    int nSingleRectWidth = this->size().width() / r;
    int nSingleRectHeight = this->size().height() / c;

    // 4.重新生成矩形
    for (int j = 0; j < c; j++) {
        for (int i = 0; i < r; i++) {
            BCSingleDisplayVirtualWidget *pDisplayVirWidget = new BCSingleDisplayVirtualWidget(r, c, i, j, this);

            // 移动位置
            pDisplayVirWidget->move(nSingleRectWidth*pDisplayVirWidget->m_i, nSingleRectHeight*pDisplayVirWidget->m_j);

            // 可能出现不能整除的情况
            pDisplayVirWidget->resize(nSingleRectWidth, nSingleRectHeight);

            m_lstSingleDisplayRect.append( pDisplayVirWidget );
        }
    }
}

void BCSingleDisplayWidget::resizeEvent(QResizeEvent *e)
{
    QSize displayVirSize = e->size();

    for (int i = 0; i < m_lstSingleDisplayRect.count(); i++) {
        BCSingleDisplayVirtualWidget *pDisplayVirWidget = m_lstSingleDisplayRect.at( i );

        // 单个矩形的尺寸
        int nSingleRectWidth = displayVirSize.width() / pDisplayVirWidget->m_row;
        int nSingleRectHeight = displayVirSize.height() / pDisplayVirWidget->m_col;

        // 移动位置
        pDisplayVirWidget->move(nSingleRectWidth*pDisplayVirWidget->m_i, nSingleRectHeight*pDisplayVirWidget->m_j);

        // 可能出现不能整除的情况
        pDisplayVirWidget->resize(nSingleRectWidth, nSingleRectHeight);
    }

    e->accept();
}

void BCSingleDisplayWidget::paintEvent(QPaintEvent */*e*/)
{
    QPainter painter(this);

    // 设置pen
    painter.setPen(QPen(Qt::black,1,Qt::SolidLine));
    // 绘制矩形
    painter.drawRect( rect() );

    // 设置字体
    QFont font = painter.font();
    font.setFamily("微软雅黑");
    font.setPointSize( 12 );
    painter.setFont(font);

    // 绘制文字
    if (m_bMatrixMode && (height() > 50)) {
        // 设置颜色
        painter.setBrush( QColor(255, 228, 225) );

        // 绘制矩形和文字
        QRect fontRect = QRect(1, rect().bottom()-50, rect().width()-2, 50);
        painter.setPen(QPen(Qt::black, 1, Qt::NoPen));
        painter.drawRect( fontRect );
        painter.setPen(QPen(Qt::black, 1, Qt::SolidLine));
        painter.drawText(fontRect, Qt::AlignCenter, m_pMDisplay->GetDisplayName());
    } else {
        QRect fontRect = QRect(6, 6, rect().width()-12, rect().height()-12);
        painter.drawText(fontRect, Qt::AlignLeft | Qt::AlignTop, m_pMDisplay->GetDisplayName());
    }
}
