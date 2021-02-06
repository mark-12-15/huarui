#include "BCMGroupDisplay.h"
#include "BCMChannel.h"
#include "BCMDisplay.h"
#include <QDebug>

BCMGroupDisplay::BCMGroupDisplay(BCMRoom *pRoom)
{
    m_pRoom = pRoom;
    m_pGroupDisplayWidget = NULL;
}

BCMGroupDisplay::~BCMGroupDisplay()
{
    // 清空屏幕链表
    while ( !m_lstDisplay.isEmpty() )
        delete m_lstDisplay.takeFirst();
}

void BCMGroupDisplay::AddDisplay(BCMDisplay *pDisplay)
{
    if (NULL == pDisplay)
        return;

    m_lstDisplay.append( pDisplay );
}

BCMDisplay *BCMGroupDisplay::GetDisplay(int id)
{
    for (int i = 0; i < m_lstDisplay.count(); i++) {
        BCMDisplay *pDisplay = m_lstDisplay.at( i );
        if (pDisplay->GetDisplayID() == id)
            return pDisplay;
    }

    return NULL;
}

QMap<int, QRect> BCMGroupDisplay::getDisplayRect(QRect rect)
{
    QMap<int, QRect> map;
    for (int i = 0; i < m_nArrayY; i++) {
        for (int j = 0; j < m_nArrayX; j++) {
            auto id = i*m_nArrayX + j;
            auto left = j*m_nResolutionX;
            auto top = i*m_nResolutionY;
            auto displayRect = QRect(left, top, m_nResolutionX, m_nResolutionY);

            if (rect.intersects(displayRect)) {
                auto interRect = rect.intersected(displayRect);
                interRect.translate(-1*left, -1*top);

                map.insert(id, interRect);
            }
        }
    }

    return map;
}

