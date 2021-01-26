#include "BCToolBar.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include "../Common/BCCommon.h"
#include "BCFaceWidget.h"

BCToolBar::BCToolBar(MainWindow::TOOLBARTYPE eToolBarType, QWidget * parent)
    :QDockWidget(parent)
{
    // 初始化
    m_eToolBarType = eToolBarType;

    QFont font = this->font();
    font.setFamily( BCCommon::g_qsDefaultFontFamily );
    this->setFont( font );

    // input and scene need left or right
    switch ( m_eToolBarType ) {
    case MainWindow::MATRIXTOOLBAR: {
        m_qsName = tr("矩阵切换窗口");
        //
    }
        break;
    case MainWindow::SIGNALSOURCETOOLBAR: {
        m_qsName = tr("信号源窗口");
        BCFaceWidget* pWidget = new BCFaceWidget(this);
        this->setWidget(pWidget);

        // 设置可停靠位置，设置最小宽度是因为当视频链表为小视频时正好可以显示两列
        this->setMinimumWidth( 300 );
        this->setMaximumWidth( 400 );
        setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
    }
        break;
    case MainWindow::EXTENDTOOLBAR: {
        m_qsName = tr("扩展窗口");

        // 设置可停靠位置
        setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
    }
        break;
    default:
        m_qsName = tr("工具窗口");
        break;
    }

    // 设置dockwidgetTitle
    this->setWindowTitle( m_qsName );
}

BCToolBar::~BCToolBar()
{

}

void BCToolBar::onVisibel(bool /*b*/)
{
    if ( this->isVisible() )
        this->hide();
    else
        this->show();
}

void BCToolBar::closeEvent(QCloseEvent *e)
{
    this->hide();
    e->ignore();
}

void BCToolBar::RefreshLanguage()
{
    // 刷新内容
    switch ( m_eToolBarType ) {
    case MainWindow::SIGNALSOURCETOOLBAR: {
        m_qsName = tr("信号源窗口");
        BCFaceWidget *pWidget = dynamic_cast<BCFaceWidget *>( this->widget() );
        if (NULL != pWidget) {
            pWidget->Refresh();
        }
    }
    default:
        break;
    }

    // 刷新标题
    this->setWindowTitle( m_qsName );
}

