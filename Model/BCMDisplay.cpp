#include "BCMDisplay.h"
#include "BCMGroupDisplay.h"
#include "../Common/BCLocalServer.h"
#include "BCMRoom.h"

BCMDisplay::BCMDisplay(BCMGroupDisplay *pGroupDisplay)
{
    m_pGroupDisplay = pGroupDisplay;
    m_nSegmentation = 1;
}

BCMDisplay::~BCMDisplay()
{

}

void BCMDisplay::SetDisplayName(const QString &qs, bool bSendCmd)
{
    m_name = qs;

    if ( !bSendCmd )
        return;

    //
}
