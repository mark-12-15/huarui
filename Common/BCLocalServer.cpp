#include "BCLocalServer.h"
#include <QLibrary>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include "../Common/BCCommon.h"
#include "../Model/BCMRoom.h"
#include "../Model/BCMChannel.h"
#include "../Model/BCMGroupDisplay.h"

BCLocalServer::BCLocalServer()
{
    m_pTcpSocket = NULL;
    m_pOneSecondTimer = NULL;

    m_bDeviceWorking    = true;
    m_bSynchronization  = false;

    m_bIsLoadDataOK = false;
    m_nIsDemoMode = 2;
    m_nHeartTimes = 0;

    m_timeOfPreview = QTime::currentTime().addMSecs( -10*1000 );

    // 初始化系统参数
    InitCommunicationPara();

    // 1秒定时器，网络连接并且使用DLL时创建,做两件事：
    // 1.如果规模没加载成功则一直判断，直到加载成功为止
    // 2.控制同步，取默认场景
    m_pOneSecondTimer = new QTimer();
    m_pOneSecondTimer->setInterval( 1000 );
    connect(m_pOneSecondTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

BCLocalServer::~BCLocalServer()
{
    // 关闭串口
    m_serial.close();

    // 关闭定时器
    if (NULL != m_pOneSecondTimer) {
        m_pOneSecondTimer->stop();
        m_pOneSecondTimer->deleteLater();
    }

    if (NULL != m_pTcpSocket) {
        m_pTcpSocket->disconnectFromHost();
        m_pTcpSocket->deleteLater();
    }
}

BCLocalServer *BCLocalServer::m_pLocalServer = NULL;

BCLocalServer *BCLocalServer::Application()
{
    if (NULL == m_pLocalServer) {
        m_pLocalServer = new BCLocalServer();
    }

    return m_pLocalServer;
}

void BCLocalServer::Destroy()
{
    if (NULL == m_pLocalServer)
        return;

    delete m_pLocalServer;
    m_pLocalServer = NULL;
}

void BCLocalServer::InitCommunicationPara()
{
    // 判断文件是否可读
    QFile file( QString("../xml/BCConnectConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    //m_bIsNetConnect = docElem.attribute("NetConnect").toInt();
    _fullScreenMode = docElem.attribute("fullMode") == 1 ? true : false;
    m_qsConnectIPWithoutDLL = docElem.attribute("ConnectIPWithoutDLL");
    m_qsConnectPortWithoutDLL = docElem.attribute("ConnectPortWithoutDLL");
    m_qsCurrentCom = docElem.attribute("CurrentCom");
    m_nCurrentBaudRate = docElem.attribute("CurrentBaudRate").toInt();
    m_nCurrentDataBit = docElem.attribute("CurrentDataBit").toInt();
    m_nCurrentStopBit = docElem.attribute("CurrentStopBit").toInt();
    m_qsCurrentCheckBit = docElem.attribute("CurrentCheckBit") ;
    m_qsCurrentStreamCtrl = docElem.attribute("CurrentStreamCtrl");
}

void BCLocalServer::SetCommunicationPara()
{
    QFile file( QString("../xml/BCConnectConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();
    //docElem.setAttribute("NetConnect", m_bIsNetConnect ? 1 : 0);
    docElem.setAttribute("fullMode", _fullScreenMode ? 1 : 0);
    docElem.setAttribute("CurrentCom", m_qsCurrentCom);
    docElem.setAttribute("CurrentBaudRate", m_nCurrentBaudRate);
    docElem.setAttribute("CurrentDataBit", m_nCurrentDataBit);
    docElem.setAttribute("CurrentStopBit", m_nCurrentStopBit);
    docElem.setAttribute("CurrentCheckBit", m_qsCurrentCheckBit);
    docElem.setAttribute("CurrentStreamCtrl", m_qsCurrentStreamCtrl);

    docElem.setAttribute("ConnectIPWithoutDLL", m_qsConnectIPWithoutDLL);
    docElem.setAttribute("ConnectPortWithoutDLL", m_qsConnectPortWithoutDLL);

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::ConnectSerialPort()
{
    if ( m_serial.isOpen() ) {
        BCCommon::g_bConnectStatusOK = true;
        return;
    }

    connect(&m_serial, SIGNAL(readyRead()), this, SLOT(onRecvSerialData()));
    m_serial.setPortName( m_qsCurrentCom );	//设置COM口

    m_serial.setBaudRate((QSerialPort::BaudRate)m_nCurrentBaudRate,QSerialPort::AllDirections); //设置波特率和读写方向
    m_serial.setDataBits((QSerialPort::DataBits)m_nCurrentDataBit);		//数据位为8位
    m_serial.setStopBits((QSerialPort::StopBits)m_nCurrentStopBit);     //一位停止位

    QSerialPort::Parity eParity = QSerialPort::NoParity;
    if (m_qsCurrentCheckBit == "Odd")
        eParity = QSerialPort::OddParity;
    else if (m_qsCurrentCheckBit == "Even")
        eParity = QSerialPort::EvenParity;
    else if (m_qsCurrentCheckBit == "Mark")
        eParity = QSerialPort::MarkParity;
    else if (m_qsCurrentCheckBit == "Space")
        eParity = QSerialPort::SpaceParity;

    m_serial.setParity( eParity );	//无校验位

    QSerialPort::FlowControl eFlowControl = QSerialPort::NoFlowControl;
    if (m_qsCurrentStreamCtrl == "Hardware")
        eFlowControl = QSerialPort::HardwareControl;
    else if (m_qsCurrentStreamCtrl == "Software")
        eFlowControl = QSerialPort::SoftwareControl;

    m_serial.setFlowControl( eFlowControl );//无流控制
    m_serial.close();                       //先关串口，再打开，可以保证串口不被其它函数占用。
    BCCommon::g_bConnectStatusOK = m_serial.open(QIODevice::ReadWrite);

    emit roomStateChanged();
}

void BCLocalServer::SetLoadDataOK()
{
    m_bIsLoadDataOK = true;

    if ( m_bIsNetConnect ) {
        if (NULL == m_pTcpSocket) {
            m_pTcpSocket = new QTcpSocket();
            m_pTcpSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
            connect(m_pTcpSocket, SIGNAL(readyRead()), this, SLOT(onRecvTcpData()));
        }

        // 每次到这里都重新进行连接
        m_pTcpSocket->connectToHost(QHostAddress(m_qsConnectIPWithoutDLL), m_qsConnectPortWithoutDLL.toInt());
        m_pTcpSocket->waitForConnected( 500 );

        if (m_pTcpSocket->ConnectedState == QAbstractSocket::ConnectedState) {
            BCCommon::g_bConnectStatusOK = true;
        } else {
            BCCommon::g_bConnectStatusOK = false;
        }
    } else {
        // 设置的时候直接连接串口
        ConnectSerialPort();

        this->AddLog( QString("[LOADDATA OK. SERIALPORT CONNECTED.]" ) );
    }

    m_pOneSecondTimer->start();
}

void BCLocalServer::SetDemoLoadDataOK()
{
    m_nIsDemoMode = 1;
    m_bIsLoadDataOK = true;

    if ( m_bIsNetConnect ) {
        if (NULL == m_pTcpSocket) {
            m_pTcpSocket = new QTcpSocket();
            m_pTcpSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 0);
            connect(m_pTcpSocket, SIGNAL(readyRead()), this, SLOT(onRecvTcpData()));
        }

        // 每次到这里都重新进行连接
        m_pTcpSocket->connectToHost(QHostAddress(m_qsConnectIPWithoutDLL), m_qsConnectPortWithoutDLL.toInt());
        m_pTcpSocket->waitForConnected( 500 );

        if (m_pTcpSocket->ConnectedState == QAbstractSocket::ConnectedState) {
            BCCommon::g_bConnectStatusOK = true;
        } else {
            BCCommon::g_bConnectStatusOK = false;
        }

        //qDebug() << m_qsConnectIPWithoutDLL << m_qsConnectPortWithoutDLL << BCCommon::g_bConnectStatusOK;
    } else {
        // 设置的时候直接连接串口
        ConnectSerialPort();

        this->AddLog( QString("[LOADDATA OK. SERIALPORT CONNECTED.]" ) );
    }

    m_pOneSecondTimer->start();
}

void BCLocalServer::DisConnect()
{
    if (0 == m_nIsDemoMode)
        return;

    m_pOneSecondTimer->stop();

    if (NULL != m_pTcpSocket) {
        m_pTcpSocket->disconnectFromHost();
        m_pTcpSocket->waitForDisconnected( 500 );
    }

    m_serial.close();

    BCCommon::g_bConnectStatusOK = false;

    // 刷新连接状态
    emit roomStateChanged();
}

void BCLocalServer::ReConnect()
{
    if (0 == m_nIsDemoMode)
        return;

    if ( m_bIsNetConnect ) {
        m_pTcpSocket->connectToHost(QHostAddress(m_qsConnectIPWithoutDLL), m_qsConnectPortWithoutDLL.toInt());
        m_pTcpSocket->waitForConnected( 500 );

        if (m_pTcpSocket->state() == QAbstractSocket::ConnectedState) {
            BCCommon::g_bConnectStatusOK = true;
        }
    } else {
        ConnectSerialPort();
    }

    // 刷新连接状态
    emit roomStateChanged();

    m_pOneSecondTimer->start();
}

void BCLocalServer::onRecvSerialData()
{
    m_nHeartTimes = 0;
    BCCommon::g_bConnectStatusOK = true;
}

void BCLocalServer::onRecvTcpData()
{
    // 只对2000起作用
//    if ((1 == BCCommon::g_nDeviceType) || (2 == BCCommon::g_nDeviceType))
//        return;

    m_nHeartTimes = 0;
    //qDebug() << "recv ip " << m_nHeartTimes << "~~~~~~~~~~~~~";

    if ( !BCCommon::g_bConnectStatusOK ) {
        m_pTcpSocket->connectToHost(QHostAddress(m_qsConnectIPWithoutDLL), m_qsConnectPortWithoutDLL.toInt());
        m_pTcpSocket->waitForConnected( 500 );

        if (m_pTcpSocket->state() == QAbstractSocket::ConnectedState) {
            BCCommon::g_bConnectStatusOK = true;
        }
    }
}

void BCLocalServer::onTimeout()
{
    // 如果小于10秒钟没必要执行
    int nOffsetTime = m_timeOfPreview.msecsTo( QTime::currentTime() );
    if (nOffsetTime < 2*1000)
        return;

    m_nHeartTimes++;

    if ((1 == BCCommon::g_nDeviceType) || (2 == BCCommon::g_nDeviceType)) {
//        if ( m_bIsNetConnect ) {
//            if (NULL != m_pUdpSocket) {
//                unsigned char ssmsg_user_k2[4];
//                ssmsg_user_k2[0] = 0xff;
//                ssmsg_user_k2[1] = 0x01;
//                ssmsg_user_k2[2] = 0x01;
//                ssmsg_user_k2[3] = 0x02;

//                m_pUdpSocket->writeDatagram((char*)ssmsg_user_k2, 4, QHostAddress::Broadcast, 1500);//将data中的数据发送
//                //qDebug() << "vp4000 send udp " << m_nHeartTimes;
//            }
//        } else {
//            SendCmd("ip\r\n", false);   // 发送id作为串口心跳包
//        }

        SendCmd("ip\r\n", false);   // 发送id作为串口心跳包

        //qDebug() << "send ip " << m_nHeartTimes;

        if (m_nHeartTimes > 5) {
            BCCommon::g_bConnectStatusOK = false;

            // 如果断开则主动关闭
            if ( m_bIsNetConnect ) {
                if ( m_pTcpSocket->state() == QAbstractSocket::ConnectedState ) {
                    m_pTcpSocket->disconnectFromHost();
                    m_pTcpSocket->waitForDisconnected(50);
                }
            } else {
                m_serial.close();
            }

            if ( m_bIsNetConnect ) {
                m_pTcpSocket->connectToHost(QHostAddress(m_qsConnectIPWithoutDLL), m_qsConnectPortWithoutDLL.toInt());
                m_pTcpSocket->waitForConnected( 500 );
            } else {
                ConnectSerialPort();
            }
        }
    }

    if (0 == BCCommon::g_nDeviceType) {
//        if ( m_bIsNetConnect ) {
//            if (NULL != m_pUdpSocketVP2000) {
//                m_pUdpSocketVP2000->write("id\r\n");
//                m_pUdpSocketVP2000->waitForBytesWritten(50);
//            }
//        } else {
//            SendCmd("id\r\n", false);   // 发送id作为串口心跳包
//        }

        SendCmd("id\r\n", false);   // 发送id作为串口心跳包

        if (m_nHeartTimes > 10) {
            BCCommon::g_bConnectStatusOK = false;

            // 如果断开则主动关闭
            if ( m_bIsNetConnect ) {
                if ( m_pTcpSocket->state() == QAbstractSocket::ConnectedState ) {
                    m_pTcpSocket->disconnectFromHost();
                    m_pTcpSocket->waitForDisconnected(50);
                }
            } else {
                m_serial.close();
            }

            if ( m_bIsNetConnect ) {
                m_pTcpSocket->connectToHost(QHostAddress(m_qsConnectIPWithoutDLL), m_qsConnectPortWithoutDLL.toInt());
                m_pTcpSocket->waitForConnected( 500 );
            } else {
                ConnectSerialPort();
            }
        }
    }

    // 刷新连接状态
    emit roomStateChanged();
}

sRoom BCLocalServer::GetRoomConfig()
{
    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return sRoom();
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return sRoom();

    // 二级链表
    QDomElement docElem = doc.documentElement();
    auto name = docElem.attribute("name");
    auto arrX = docElem.attribute("arrayX").toInt();
    auto arrY = docElem.attribute("arrayY").toInt();
    auto width = docElem.attribute("width").toInt();
    auto height = docElem.attribute("height").toInt();

    sGroupDisplay sgroupdisplay;
    sgroupdisplay.id = 0;
    sgroupdisplay.left = 0;
    sgroupdisplay.top = 0;
    sgroupdisplay.resolutionX = width;
    sgroupdisplay.resolutionY = height;
    sgroupdisplay.width = arrX*width;
    sgroupdisplay.height = arrY*height;
    sgroupdisplay.arrayX = arrX;
    sgroupdisplay.arrayY = arrY;
    sgroupdisplay.segmentationX = 2;
    sgroupdisplay.segmentationY = 2;

    for (int i = 0; i < arrX; i++) {
        for (int j = 0; j < arrY; j++) {
            sDisplay sdisplay;
            sdisplay.id = i*arrY + j;
            sdisplay.name = QString("%1").arg(i*arrY + j, 3, 10, QChar('0'));
            sdisplay.left = j*width;
            sdisplay.top = i*height;
            sdisplay.resolutionX = width;
            sdisplay.resolutionY = height;
            sdisplay.segmentation = 4;

            sgroupdisplay.lstDisplay.append( sdisplay );
        }
    }

    sRoom sroom;
    sroom.id = 0;
    sroom.name = name;
    sroom.width = arrY*width;
    sroom.height = arrX*height;
    sroom.lstGroupDisplay.append( sgroupdisplay );
    sroom.isNetConnect = docElem.attribute("connectByNet").toInt();
    sroom.switchip = docElem.attribute("ip");
    sroom.switchport = docElem.attribute("port").toInt();
    sroom.switchCom = docElem.attribute("com");
    sroom.switchBaudRate = docElem.attribute("rate").toInt();
    sroom.switchDataBit = docElem.attribute("databit").toInt();
    sroom.switchStopBit = docElem.attribute("stopbit").toInt();
    sroom.switchCheckBit = docElem.attribute("checkbit");
    sroom.switchStreamCtrl = docElem.attribute("stream");
    sroom.switchtype = docElem.attribute("type").toInt();
    sroom.switchoncmd = docElem.attribute("on");
    sroom.switchoffcmd = docElem.attribute("off");

    return sroom;
}

QList<sWindowScene> BCLocalServer::GetRoomScenes()
{
    QList<sWindowScene> res;

    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return res;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return res;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode screenList = docElem.childNodes().at(i);
        if ( !screenList.isElement() )
            continue;

        if (screenList.nodeName() == "SceneList") {
            auto eleScreenList = screenList.toElement();

            for (int j = 0; j < eleScreenList.childNodes().count(); j++) {
                QDomNode screen = screenList.childNodes().at(j);
                if ( !screen.isElement() )
                    continue;

                QDomElement eleScreen = screen.toElement();
                sWindowScene swindowscene;
                swindowscene.id = eleScreen.attribute("id").toInt();
                swindowscene.cycle = eleScreen.attribute("cycle").toInt();
                swindowscene.loopInterval = eleScreen.attribute("loopInterval").toInt();
                swindowscene.name = eleScreen.attribute("name");

                for (int k = 0; k < eleScreen.childNodes().count(); k++) {
                    auto screenData = eleScreen.childNodes().at(k);
                    if ( !screenData.isElement() )
                        continue;

                    auto eleScreenData = screenData.toElement();

                    sWindowSceneData swindowscenedata;
                    swindowscenedata.chid = eleScreenData.attribute("chid").toInt();
                    swindowscenedata.winid = eleScreenData.attribute("winid").toInt();
                    swindowscenedata.left = eleScreenData.attribute("x").toInt();
                    swindowscenedata.top = eleScreenData.attribute("y").toInt();
                    swindowscenedata.width = eleScreenData.attribute("w").toInt();
                    swindowscenedata.height = eleScreenData.attribute("h").toInt();

                    swindowscene.lstData.append( swindowscenedata );
                }

                res.append( swindowscene );
            }
        }
    }

    return res;
}

QList<sInputChannel> BCLocalServer::GetInputChannels()
{
    QList<sInputChannel> res;

    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return res;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return res;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode channel = docElem.childNodes().at(i);
        if ( !channel.isElement() )
            continue;

        if (channel.nodeName() == "Channel") {
            auto eleChannel = channel.toElement();

            for (int j = 0; j < eleChannel.childNodes().count(); j++) {
                QDomNode ch = eleChannel.childNodes().at(j);
                if ( !ch.isElement() )
                    continue;

                QDomElement eleCh = ch.toElement();

                sInputChannel sinputchannel;
                sinputchannel.id = eleCh.attribute("id").toInt();
                sinputchannel.name = eleCh.attribute("name");
                sinputchannel.type = 0;
                sinputchannel.signalsource = 0;

                res.append( sinputchannel );
            }
        }
    }

    return res;
}

void BCLocalServer::UpdateRoomName(QString str)
{
    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();
    docElem.setAttribute("name", str);

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::UpdateRoomSwitchConfig(BCMRoom *room)
{
    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();
    docElem.setAttribute("connectByNet", room->isConnectByNet);
    docElem.setAttribute("ip", room->switchip);
    docElem.setAttribute("port", room->switchport);
    docElem.setAttribute("com", room->qsCurrentCom);
    docElem.setAttribute("rate", room->nCurrentBaudRate);
    docElem.setAttribute("databit", room->nCurrentDataBit);
    docElem.setAttribute("stopbit", room->nCurrentStopBit);
    docElem.setAttribute("checkbit", room->qsCurrentCheckBit);
    docElem.setAttribute("stream", room->qsCurrentStreamCtrl);
    docElem.setAttribute("type", room->switchtype);
    docElem.setAttribute("on", room->switchoncmd);
    docElem.setAttribute("off", room->switchoffcmd);

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::AddScreen(BCMWindowScene *screen)
{
    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        if ("SceneList" != nodeRoom.nodeName())
            continue;

        for (int j = 0; j < nodeRoom.childNodes().count(); j++) {
            auto nodeScreen = nodeRoom.childNodes().at(j);
            if (!nodeScreen.isElement())
                continue;

            auto eleScreen = nodeScreen.toElement();
            if (screen->GetWindowSceneID() == eleScreen.attribute("id").toInt())
            {
                nodeRoom.removeChild(nodeScreen);
            }
        }

        QDomElement eleScreen = doc.createElement(QString("Screen"));
        eleScreen.setAttribute("id", screen->GetWindowSceneID());
        eleScreen.setAttribute("name", screen->GetWindowSceneName());
        eleScreen.setAttribute("cycle", screen->IsCycled()?"1":"0");
        eleScreen.setAttribute("loopInterval", QString::number(screen->GetWindowSceneLoopInterval()));

        foreach(auto data, screen->GetWindowSceneData()) {
            QDomElement eleNode = doc.createElement(QString("Node"));
            eleNode.setAttribute("winid", QString::number(data->m_nWindowID));
            eleNode.setAttribute("chid", QString::number(data->m_nChannelID));
            eleNode.setAttribute("x", QString::number(data->m_rect.left()));
            eleNode.setAttribute("y", QString::number(data->m_rect.top()));
            eleNode.setAttribute("w", QString::number(data->m_rect.width()));
            eleNode.setAttribute("h", QString::number(data->m_rect.height()));

            eleScreen.appendChild(eleNode);
        }

        nodeRoom.appendChild(eleScreen);
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::RemoveScreen(int id)
{
    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        if ("SceneList" != nodeRoom.nodeName())
            continue;

        for (int j = 0; j < nodeRoom.childNodes().count(); j++) {
            auto nodeScreen = nodeRoom.childNodes().at(j);
            if (!nodeScreen.isElement())
                continue;

            auto eleScreen = nodeScreen.toElement();
            if (id == eleScreen.attribute("id").toInt())
            {
                nodeRoom.removeChild(nodeScreen);
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::UpdateSceneSort(QMap<int, int> map)
{
    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        if ("SceneList" != nodeRoom.nodeName())
            continue;

        for (int j = 0; j < nodeRoom.childNodes().count(); j++) {
            auto nodeScreen = nodeRoom.childNodes().at(j);
            if (!nodeScreen.isElement())
                continue;

            auto eleScreen = nodeScreen.toElement();
            auto id = eleScreen.attribute("id").toInt();
            if (map.contains(id))
            {
                eleScreen.setAttribute("sort", QString::number(map.value(id)));
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::UpdateInputChannel(int id, QString name)
{
    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        if ("Channel" != nodeRoom.nodeName())
            continue;

        for (int j = 0; j < nodeRoom.childNodes().count(); j++) {
            auto nodeScreen = nodeRoom.childNodes().at(j);
            if (!nodeScreen.isElement())
                continue;

            auto eleScreen = nodeScreen.toElement();
            if (id == eleScreen.attribute("id").toInt())
            {
                eleScreen.setAttribute("name", name);
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

QList<sMatrix> BCLocalServer::GetMatrixConfig()
{
    QList<sMatrix> lstRes;

    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return lstRes;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return lstRes;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环添加矩阵
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        sMatrix smatrix;
        QDomElement eleRoom = nodeRoom.toElement();
        smatrix.id = eleRoom.attribute(QString("ID")).toInt();
        smatrix.name = eleRoom.attribute(QString("Name"));
        smatrix.isConnectByNet = eleRoom.attribute(QString("IsConnectByNet")).toInt();
        smatrix.ip = eleRoom.attribute(QString("IP"));
        smatrix.port = eleRoom.attribute(QString("Port")).toInt();
        smatrix.qsCurrentCom = eleRoom.attribute(QString("CurrentCom"));
        smatrix.nCurrentBaudRate = eleRoom.attribute(QString("CurrentBaudRate")).toInt();
        smatrix.nCurrentDataBit = eleRoom.attribute(QString("CurrentDataBit")).toInt();
        smatrix.nCurrentStopBit = eleRoom.attribute(QString("CurrentStopBit")).toInt();
        smatrix.qsCurrentCheckBit = eleRoom.attribute(QString("CurrentCheckBit"));
        smatrix.qsCurrentStreamCtrl = eleRoom.attribute(QString("CurrentStreamCtrl"));
        smatrix.switchFlag = eleRoom.attribute(QString("switchFlag"));
        smatrix.saveFlag = eleRoom.attribute(QString("saveFlag"));
        smatrix.loadFlag = eleRoom.attribute(QString("loadFlag"));
        smatrix.isOn = eleRoom.attribute(QString("isOn")).toInt();
        smatrix.qsOnCmd = eleRoom.attribute(QString("OnCmd"));
        smatrix.qsOffCmd = eleRoom.attribute(QString("OffCmd"));
        smatrix.jointSceneRoomID = eleRoom.attribute(QString("JointSceneRoomID")).toInt();
        smatrix.jointWithVP4000 = eleRoom.attribute(QString("jointWithVP4000")).toInt();
        smatrix.cmdType = eleRoom.attribute(QString("cmdType")).toInt();

        // 循环添加矩阵入口、出口和场景信息
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode node = eleRoom.childNodes().at(j);
            if ( !node.isElement() )
                continue;

            QDomElement ele = node.toElement();
            QString qsNodeName = node.nodeName();
            if (qsNodeName == "Input") {
                for (int k = 0; k < ele.childNodes().count(); k++) {
                    QDomNode nodeInput = ele.childNodes().at(k);
                    if ( !nodeInput.isElement() )
                        continue;

                    sMatrixNode inputNode;
                    QDomElement eleInput = nodeInput.toElement();
                    inputNode.id = eleInput.attribute(QString("ID")).toInt();
                    inputNode.name = eleInput.attribute(QString("Name"));
                    inputNode.cutl = eleInput.attribute(QString("cutl")).toInt();
                    inputNode.cutr = eleInput.attribute(QString("cutr")).toInt();
                    inputNode.cutt = eleInput.attribute(QString("cutt")).toInt();
                    inputNode.cutb = eleInput.attribute(QString("cutb")).toInt();

                    smatrix.lstInputNode.append( inputNode );
                }
            }
            if (qsNodeName == "Output") {
                for (int k = 0; k < ele.childNodes().count(); k++) {
                    QDomNode nodeOutput = ele.childNodes().at(k);
                    if ( !nodeOutput.isElement() )
                        continue;

                    sMatrixNode outputNode;
                    QDomElement eleOutput = nodeOutput.toElement();
                    outputNode.id = eleOutput.attribute(QString("ID")).toInt();
                    outputNode.name = eleOutput.attribute(QString("Name"));
                    outputNode.nSwitch = eleOutput.attribute(QString("IsSwitch")).toInt();
                    outputNode.nSwitchID = eleOutput.attribute(QString("SwitchID")).toInt();
                    outputNode.isOn = eleOutput.attribute(QString("isOn")).toInt();
                    outputNode.qsOnCmd = eleOutput.attribute(QString("OnCmd"));
                    outputNode.qsOffCmd = eleOutput.attribute(QString("OffCmd"));
                    outputNode.jointWithVP4000ChannelID = eleOutput.attribute(QString("jointWithVP4000ChannelID")).toInt();
                    outputNode.jointWithVP2000ChannelType = eleOutput.attribute(QString("jointWithVP2000ChannelType")).toInt();

                    smatrix.lstOutputNode.append( outputNode );
                }
            }
            if (qsNodeName == "SceneList") {
                for (int k = 0; k < ele.childNodes().count(); k++) {
                    QDomNode nodeScene = ele.childNodes().at(k);
                    if ( !nodeScene.isElement() )
                        continue;

                    sMatrixScene sceneNode;
                    QDomElement eleScene = nodeScene.toElement();
                    sceneNode.id = eleScene.attribute(QString("ID")).toInt();
                    sceneNode.name = eleScene.attribute(QString("Name"));

                    for (int m = 0; m < eleScene.childNodes().count(); m++) {
                        QDomNode nodeSwitch = eleScene.childNodes().at(m);
                        if ( !nodeSwitch.isElement() )
                            continue;

                        QDomElement eleSwitch = nodeSwitch.toElement();
                        int inputID = eleSwitch.attribute(QString("InputID")).toInt();
                        int outputID = eleSwitch.attribute(QString("OutputID")).toInt();
                        sceneNode.lstSwitchInfo.append( QPoint(inputID, outputID) );
                    }

                    smatrix.lstScene.append( sceneNode );
                }
            }
        }

        lstRes.append( smatrix );
    }

    return lstRes;
}

void BCLocalServer::SetMatrixConfig(const QList<sMatrix> lstMatrix)
{
    QDomDocument doc;
    QDomElement eleRoot = doc.createElement(QString("BR"));

    // 循环添加矩阵房间
    for (int i = 0; i < lstMatrix.count(); i++) {
        sMatrix smatrix = lstMatrix.at( i );

        // 房间基本属性
        QDomElement eleRoomNode = doc.createElement(QString("Room"));
        eleRoomNode.setAttribute(QString("ID"), smatrix.id);
        eleRoomNode.setAttribute(QString("Name"), smatrix.name);
        eleRoomNode.setAttribute(QString("IsConnectByNet"), smatrix.isConnectByNet);
        eleRoomNode.setAttribute(QString("IP"), smatrix.ip);
        eleRoomNode.setAttribute(QString("Port"), smatrix.port);
        eleRoomNode.setAttribute(QString("CurrentCom"), smatrix.qsCurrentCom);
        eleRoomNode.setAttribute(QString("CurrentBaudRate"), smatrix.nCurrentBaudRate);
        eleRoomNode.setAttribute(QString("CurrentDataBit"), smatrix.nCurrentDataBit);
        eleRoomNode.setAttribute(QString("CurrentStopBit"), smatrix.nCurrentStopBit);
        eleRoomNode.setAttribute(QString("CurrentCheckBit"), smatrix.qsCurrentCheckBit);
        eleRoomNode.setAttribute(QString("CurrentStreamCtrl"), smatrix.qsCurrentStreamCtrl);
        eleRoomNode.setAttribute(QString("IsOn"), smatrix.isOn);
        eleRoomNode.setAttribute(QString("OnCmd"), smatrix.qsOnCmd);
        eleRoomNode.setAttribute(QString("OffCmd"), smatrix.qsOffCmd);
        eleRoomNode.setAttribute(QString("JointSceneRoomID"), smatrix.jointSceneRoomID);
        eleRoomNode.setAttribute(QString("cmdType"), smatrix.cmdType);
        eleRoomNode.setAttribute(QString("switchFlag"), smatrix.switchFlag);
        eleRoomNode.setAttribute(QString("saveFlag"), smatrix.saveFlag);
        eleRoomNode.setAttribute(QString("loadFlag"), smatrix.loadFlag);
        eleRoomNode.setAttribute(QString("jointWithVP4000"), smatrix.jointWithVP4000);

        // 输入通道
        QDomElement eleInput = doc.createElement(QString("Input"));
        for (int j = 0; j < smatrix.lstInputNode.count(); j++) {
            sMatrixNode inputNode = smatrix.lstInputNode.at( j );
            QDomElement eleInputNode = doc.createElement(QString("Node"));
            eleInputNode.setAttribute(QString("ID"), inputNode.id);
            eleInputNode.setAttribute(QString("Name"), inputNode.name);
            eleInputNode.setAttribute(QString("cutl"), inputNode.cutl);
            eleInputNode.setAttribute(QString("cutr"), inputNode.cutr);
            eleInputNode.setAttribute(QString("cutt"), inputNode.cutt);
            eleInputNode.setAttribute(QString("cutb"), inputNode.cutb);

            eleInput.appendChild( eleInputNode );
        }
        eleRoomNode.appendChild( eleInput );

        // 输出通道
        QDomElement eleOutput = doc.createElement(QString("Output"));
        for (int j = 0; j < smatrix.lstOutputNode.count(); j++) {
            sMatrixNode outputNode = smatrix.lstOutputNode.at( j );
            QDomElement eleOutputNode = doc.createElement(QString("Node"));
            eleOutputNode.setAttribute(QString("ID"), outputNode.id);
            eleOutputNode.setAttribute(QString("Name"), outputNode.name);
            eleOutputNode.setAttribute(QString("IsSwitch"), outputNode.nSwitch);
            eleOutputNode.setAttribute(QString("SwitchID"), outputNode.nSwitchID);
            eleOutputNode.setAttribute(QString("IsOn"), outputNode.isOn);
            eleOutputNode.setAttribute(QString("OnCmd"), outputNode.qsOnCmd);
            eleOutputNode.setAttribute(QString("OffCmd"), outputNode.qsOffCmd);
            eleOutputNode.setAttribute(QString("jointWithVP4000ChannelID"), outputNode.jointWithVP4000ChannelID);
            eleOutputNode.setAttribute(QString("jointWithVP2000ChannelType"), outputNode.jointWithVP2000ChannelType);

            eleOutput.appendChild( eleOutputNode );
        }
        eleRoomNode.appendChild( eleOutput );

        // 场景
        QDomElement eleSceneListNode = doc.createElement(QString("SceneList"));
        for (int j = 0; j < smatrix.lstScene.count(); j++) {
            sMatrixScene sceneNode = smatrix.lstScene.at( j );
            QDomElement eleSceneNode = doc.createElement(QString("Scene"));
            eleSceneNode.setAttribute(QString("ID"), sceneNode.id);
            eleSceneNode.setAttribute(QString("Name"), sceneNode.name);

            // 场景切换信息
            for (int k = 0; k < sceneNode.lstSwitchInfo.count(); k++) {
                QPoint switchPt = sceneNode.lstSwitchInfo.at( k );
                QDomElement eleSwitchNode = doc.createElement(QString("Node"));
                eleSwitchNode.setAttribute(QString("InputID"), switchPt.x());
                eleSwitchNode.setAttribute(QString("OutputID"), switchPt.y());

                eleSceneNode.appendChild( eleSwitchNode );
            }

            eleSceneListNode.appendChild( eleSceneNode );
        }
        eleRoomNode.appendChild( eleSceneListNode );

        eleRoot.appendChild( eleRoomNode );
    }

    // 添加根节点
    doc.appendChild( eleRoot );

    // 写入文件
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::SetMatrixName(int roomid, int output, int id, const QString &name)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        QDomElement eleRoom = nodeRoom.toElement();
        int nRoomID = eleRoom.attribute("ID").toInt();
        if (nRoomID != roomid)
            continue;

        // 循环节点
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode nodeIO = eleRoom.childNodes().at(j);
            if ( !nodeIO.isElement() )
                continue;

            QDomElement eleIO = nodeIO.toElement();
            QString qsNodeIOName = (0 == output) ? "Input" : "Output";
            if (qsNodeIOName != eleIO.nodeName())
                continue;

            for (int k = 0; k < eleIO.childNodes().count(); k++) {
                QDomNode node = eleIO.childNodes().at(k);
                if ( !node.isElement() )
                    continue;

                QDomElement ele = node.toElement();
                int nNodeID = ele.attribute("ID").toInt();
                if (nNodeID != id)
                    continue;

                ele.setAttribute("Name", name);
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::AddMatrixScene(int roomid, int id, const QString &name, QList<QPoint> lst)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        QDomElement eleRoom = nodeRoom.toElement();
        int nRoomID = eleRoom.attribute("ID").toInt();
        if (nRoomID != roomid)
            continue;

        // 循环节点
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode nodeSceneList = eleRoom.childNodes().at(j);
            if ( !nodeSceneList.isElement() )
                continue;

            QDomElement eleSceneList = nodeSceneList.toElement();
            if ("SceneList" != eleSceneList.nodeName())
                continue;

            // 查找是否有场景节点，有则删除
            for (int k = 0; k < eleSceneList.childNodes().count(); k++) {
                QDomNode nodeScene = eleSceneList.childNodes().at(k);
                if ( !nodeScene.isElement() )
                    continue;

                QDomElement eleScene = nodeScene.toElement();
                int nSceneID = eleScene.attribute("ID").toInt();
                if (nSceneID != id)
                    continue;

                eleSceneList.removeChild( nodeScene );
            }

            // 新建场景并添加
            QDomElement eleSceneNode = doc.createElement(QString("Scene"));
            eleSceneNode.setAttribute(QString("ID"), id);
            eleSceneNode.setAttribute(QString("Name"), name);
            for (int k = 0; k < lst.count(); k++) {
                QPoint pt = lst.at(k);
                QDomElement eleSceneDataNode = doc.createElement(QString("Node"));
                eleSceneDataNode.setAttribute(QString("InputID"), pt.x());
                eleSceneDataNode.setAttribute(QString("OutputID"), pt.y());

                eleSceneNode.appendChild( eleSceneDataNode );
            }

            eleSceneList.appendChild( eleSceneNode );
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::UpdateMatrixScene(int roomid, int id, QList<QPoint> lst)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        QDomElement eleRoom = nodeRoom.toElement();
        int nRoomID = eleRoom.attribute("ID").toInt();
        if (nRoomID != roomid)
            continue;

        // 循环节点
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode nodeSceneList = eleRoom.childNodes().at(j);
            if ( !nodeSceneList.isElement() )
                continue;

            QDomElement eleSceneList = nodeSceneList.toElement();
            if ("SceneList" != eleSceneList.nodeName())
                continue;

            // 查找是否有场景节点，有则删除
            for (int k = 0; k < eleSceneList.childNodes().count(); k++) {
                QDomNode nodeScene = eleSceneList.childNodes().at(k);
                if ( !nodeScene.isElement() )
                    continue;

                QDomElement eleScene = nodeScene.toElement();
                int nSceneID = eleScene.attribute("ID").toInt();
                if (nSceneID != id)
                    continue;

                // 清空场景数据
                QDomElement eleNullScene = doc.createElement(QString("Scene"));
                eleNullScene.setAttribute("ID", id);
                eleNullScene.setAttribute("Name", eleScene.attribute("Name"));

                // 更新场景数据
                for (int l = 0; l < lst.count(); l++) {
                    QPoint pt = lst.at(l);
                    QDomElement eleSceneDataNode = doc.createElement(QString("Node"));
                    eleSceneDataNode.setAttribute(QString("InputID"), pt.x());
                    eleSceneDataNode.setAttribute(QString("OutputID"), pt.y());

                    eleNullScene.appendChild( eleSceneDataNode );
                }

                eleSceneList.replaceChild(eleNullScene, eleScene);
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::UpdateMatrixScene(int roomid, int id, const QString &name)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        QDomElement eleRoom = nodeRoom.toElement();
        int nRoomID = eleRoom.attribute("ID").toInt();
        if (nRoomID != roomid)
            continue;

        // 循环节点
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode nodeSceneList = eleRoom.childNodes().at(j);
            if ( !nodeSceneList.isElement() )
                continue;

            QDomElement eleSceneList = nodeSceneList.toElement();
            if ("SceneList" != eleSceneList.nodeName())
                continue;

            // 查找是否有场景节点，有则删除
            for (int k = 0; k < eleSceneList.childNodes().count(); k++) {
                QDomNode nodeScene = eleSceneList.childNodes().at(k);
                if ( !nodeScene.isElement() )
                    continue;

                QDomElement eleScene = nodeScene.toElement();
                int nSceneID = eleScene.attribute("ID").toInt();
                if (nSceneID != id)
                    continue;

                eleScene.setAttribute("Name", name);
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::UpdateMatrixRoomName(int roomid, const QString &name)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        QDomElement eleRoom = nodeRoom.toElement();
        int nRoomID = eleRoom.attribute("ID").toInt();
        if (nRoomID != roomid)
            continue;

        eleRoom.setAttribute("Name", name);
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::UpdateMatrixRoomSwitch(int roomid, int isOn)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        QDomElement eleRoom = nodeRoom.toElement();
        int nRoomID = eleRoom.attribute("ID").toInt();
        if (nRoomID != roomid)
            continue;

        eleRoom.setAttribute("IsOn", isOn);
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::UpdateMatrixOutputSwitch(int roomid, int id, int isOn)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        // 查找房间
        QDomElement eleRoom = nodeRoom.toElement();
        int nCurrentID = eleRoom.attribute("ID").toInt();
        if (nCurrentID != roomid)
            continue;

        // 查找输出节点
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode nodeOutput = eleRoom.childNodes().at(j);
            if ( !nodeOutput.isElement() )
                continue;

            if (nodeOutput.nodeName() != "Output")
                continue;

            QDomElement eleOutput = nodeOutput.toElement();
            for (int k = 0; k < eleOutput.childNodes().count(); k++) {
                QDomNode node = eleOutput.childNodes().at(k);
                if ( !node.isElement() )
                    continue;

                QDomElement ele = node.toElement();
                int nCurrentID = ele.attribute("ID").toInt();
                if (nCurrentID != id)
                    continue;

                ele.setAttribute("IsOn", isOn);
                break;
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::UpdateMatrixInputCut(int roomid, int id, int l, int r, int t, int b)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        // 查找房间
        QDomElement eleRoom = nodeRoom.toElement();
        int nCurrentID = eleRoom.attribute("ID").toInt();
        if (nCurrentID != roomid)
            continue;

        // 查找输出节点
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode nodeOutput = eleRoom.childNodes().at(j);
            if ( !nodeOutput.isElement() )
                continue;

            if (nodeOutput.nodeName() != "Input")
                continue;

            QDomElement eleOutput = nodeOutput.toElement();
            for (int k = 0; k < eleOutput.childNodes().count(); k++) {
                QDomNode node = eleOutput.childNodes().at(k);
                if ( !node.isElement() )
                    continue;

                QDomElement ele = node.toElement();
                int nCurrentID = ele.attribute("ID").toInt();
                if (nCurrentID != id)
                    continue;

                ele.setAttribute("cutl", l);
                ele.setAttribute("cutr", r);
                ele.setAttribute("cutt", t);
                ele.setAttribute("cutb", b);
                break;
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::UpdateMatrixJointChannel(int id, QList<QPoint> lst)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        // 查找房间
        QDomElement eleRoom = nodeRoom.toElement();
        int roomid = eleRoom.attribute("ID").toInt();
        if (roomid != id)
            continue;

        // 查找输出节点
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode nodeOutput = eleRoom.childNodes().at(j);
            if ( !nodeOutput.isElement() )
                continue;

            if (nodeOutput.nodeName() != "Output")
                continue;

            QDomElement eleOutput = nodeOutput.toElement();
            for (int k = 0; k < eleOutput.childNodes().count(); k++) {
                QDomNode node = eleOutput.childNodes().at(k);
                if ( !node.isElement() )
                    continue;

                QDomElement ele = node.toElement();
                int nCurrentID = ele.attribute("ID").toInt();
                for (int m = 0; m < lst.count(); m++) {
                    QPoint pt = lst.at( m );
                    if (pt.x() != nCurrentID)
                        continue;

                    ele.setAttribute("jointWithVP4000ChannelID", pt.y());
                    break;
                }
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::RemoveMatrixScene(int roomid, int id)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        QDomElement eleRoom = nodeRoom.toElement();
        int nRoomID = eleRoom.attribute("ID").toInt();
        if (nRoomID != roomid)
            continue;

        // 循环节点
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode nodeSceneList = eleRoom.childNodes().at(j);
            if ( !nodeSceneList.isElement() )
                continue;

            QDomElement eleSceneList = nodeSceneList.toElement();
            if ("SceneList" != eleSceneList.nodeName())
                continue;

            // 查找是否有场景节点，有则删除
            for (int k = 0; k < eleSceneList.childNodes().count(); k++) {
                QDomNode nodeScene = eleSceneList.childNodes().at(k);
                if ( !nodeScene.isElement() )
                    continue;

                QDomElement eleScene = nodeScene.toElement();
                int nSceneID = eleScene.attribute("ID").toInt();
                if (nSceneID != id)
                    continue;

                eleSceneList.removeChild( nodeScene );
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::ClearMatrixScene(int roomid)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        QDomElement eleRoom = nodeRoom.toElement();
        int nRoomID = eleRoom.attribute("ID").toInt();
        if (nRoomID != roomid)
            continue;

        // 循环节点
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode nodeSceneList = eleRoom.childNodes().at(j);
            if ( !nodeSceneList.isElement() )
                continue;

            QDomElement eleSceneList = nodeSceneList.toElement();
            if ("SceneList" != eleSceneList.nodeName())
                continue;

            QDomElement eleNullSceneList = doc.createElement(QString("SceneList"));
            eleRoom.replaceChild(eleNullSceneList, eleSceneList);
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::SetMatrixSwitch(int id, int inputid, int outputid)
{
    QFile file( QString("../xml/BCMatrixConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return;

    // 二级链表
    QDomElement docElem = doc.documentElement();

    // 循环矩阵房间
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeRoom = docElem.childNodes().at(i);
        if ( !nodeRoom.isElement() )
            continue;

        // 查找房间
        QDomElement eleRoom = nodeRoom.toElement();
        int nCurrentID = eleRoom.attribute("ID").toInt();
        if (nCurrentID != id)
            continue;

        // 查找输出节点
        for (int j = 0; j < eleRoom.childNodes().count(); j++) {
            QDomNode nodeOutput = eleRoom.childNodes().at(j);
            if ( !nodeOutput.isElement() )
                continue;

            if (nodeOutput.nodeName() != "Output")
                continue;

            QDomElement eleOutput = nodeOutput.toElement();
            for (int k = 0; k < eleOutput.childNodes().count(); k++) {
                QDomNode node = eleOutput.childNodes().at(k);
                if ( !node.isElement() )
                    continue;

                QDomElement ele = node.toElement();
                int nCurrentID = ele.attribute("ID").toInt();
                if (nCurrentID != outputid)
                    continue;

                ele.setAttribute("IsSwitch", (-1 == inputid) ? 0 : 1);
                ele.setAttribute("SwitchID", inputid);
                break;
            }
        }
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLocalServer::reset(const QString &qsGroupDisplayIDs)
{
    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("greset %1\r\n").arg(qsGroupDisplayIDs);
    } else if (BCCommon::g_nDeviceType == 0) {  // VP2000
        cmd = QString("reset\r\n");
    }

    SendCmd( cmd );
}

void BCLocalServer::winsize(int gid, int chid, int winid, int l, int t, int r, int b, int type, int copyIndex)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    // 添加偏移量
    MainWindow *pMainWindow = BCCommon::Application();
    QSize size = pMainWindow->GetWinsizeOffset( gid );
    l += size.width();
    r += size.width();
    t += size.height();
    b += size.height();

    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("gwinsize %1 %2 %3 %4 %5 %6 %7\r\n")
            .arg(gid)
            .arg(winid)
            .arg(chid)
            .arg(l)
            .arg(t)
            .arg(r)
            .arg(b);
    } else if (BCCommon::g_nDeviceType == 0) {  // VP2000
        if (-1 == copyIndex) {  // 没有子窗口则正常开窗
            QString qsHeader;
            if (0 == type)
                qsHeader = "Dwinsize";
            else if (3 == type)
                qsHeader = "Vwinsize";
            else if (2 == type)
                qsHeader = "Bwinsize";

            cmd = QString("%1 %2 %3 %4 %5 %6\r\n")
                    .arg( qsHeader )
                    .arg( chid )
                    .arg(l)
                    .arg(t)
                    .arg(r)
                    .arg(b);
        } else { // 有子窗口则复制开窗
            type = (3 == type) ? 1 : type;  // VP2000的类型3是video
            cmd = QString("Cwinsize %1 %2 %3 %4 %5 %6 %7\r\n")
                    .arg( type )
                    .arg( chid )
                    .arg( copyIndex )
                    .arg(l)
                    .arg(t)
                    .arg(r)
                    .arg(b);
        }
    }

    SendCmd( cmd );
}

void BCLocalServer::winswitch(int gid, int winid, int chid, int type, int copyIndex)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("gwinswitch %1 %2\r\n")
                .arg(gid)
                .arg(winid);
    } else if (BCCommon::g_nDeviceType == 0) { // VP2000
        if (-1 == copyIndex) {  // 没有子窗口则正常开窗
            QString qsHeader;
            if (0 == type)
                qsHeader = "Dwinswitch";
            else if (3 == type)
                qsHeader = "Vwinswitch";
            else if (2 == type)
                qsHeader = "Bwinswitch";

            cmd = QString("%1 %2\r\n")
                    .arg( qsHeader )
                    .arg( chid );
        } else {
            type = (3 == type) ? 1 : type;
            cmd = QString("wincopyswitch %1 %2 %3\r\n")
                    .arg( type )
                    .arg( chid )
                    .arg( copyIndex );
        }
    }

    SendCmd( cmd );
}

void BCLocalServer::save(int groupID, int sceneID)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("gsave %1 %2\r\n")
                .arg(groupID)
                .arg(sceneID);
    } else if (BCCommon::g_nDeviceType == 0) {  // VP2000
        cmd = QString("save %1\r\n").arg(sceneID);
    }

    SendCmd( cmd );
}

void BCLocalServer::load(int groupID, int sceneID)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("gload %1 %2\r\n")
                .arg(groupID)
                .arg(sceneID);
    } else if (BCCommon::g_nDeviceType == 0) {  // VP2000
        cmd = QString("load %1\r\n").arg(sceneID);
    }

    SendCmd( cmd );
}

void BCLocalServer::winup(int gid, int winid)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("winup %1 %2\r\n")
                .arg(gid)
                .arg(winid);
    } else if (BCCommon::g_nDeviceType == 0) {  // VP2000
        //cmd = QString("load %1\r\n").arg(sceneID);
    }

    SendCmd( cmd );
}

void BCLocalServer::windown(int gid, int winid)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("windown %1 %2\r\n")
                .arg(gid)
                .arg(winid);
    } else if (BCCommon::g_nDeviceType == 0) {  // VP2000
        //cmd = QString("load %1\r\n").arg(sceneID);
    }

    SendCmd( cmd );
}

void BCLocalServer::onDelaySendCmd()
{
    if ( m_lstDelayCmd.isEmpty() )
        return;

    SendCmd( m_lstDelayCmd.takeFirst() );
}

void BCLocalServer::SetRstGroup()
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    SendCmd( QString("rstgroup\r\n") );
}
QString BCLocalServer::GetRstGroup()
{
    return QString("rstgroup\r\n");
}
void BCLocalServer::SetGroup(int gid, int schid, int echid)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    QString cmd = QString("setgroup %1 %2 %3\r\n").arg(gid).arg(schid).arg(echid);

    SendCmd( cmd );
}
QString BCLocalServer::GetGroup(int gid, int schid, int echid)
{
    QString cmd = QString("setgroup %1 %2 %3\r\n").arg(gid).arg(schid).arg(echid);

    return cmd;
}

void BCLocalServer::RemoveGroup(int gid)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    QString cmd = QString("setgroup %1 255\r\n").arg(gid);

    SendCmd( cmd );
}

void BCLocalServer::SetCustomResolution(int liveW, int liveH, int preW, int preH, int syncW, int syncH, int totalW, int totalH, int /*polarityW*/, int /*polarityH*/, int hertz)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    QString cmd = QString("newddcmodetype 8 %1 %2 %3 %4 %5 %6 %7 %8 %9 %10\r\n")
            .arg(preW).arg(totalW-liveW-preW-syncW).arg(syncW).arg(liveW)
            .arg(preH).arg(totalH-liveH-preH-syncH).arg(syncH).arg(liveH)
            .arg(0).arg(hertz);

    SendCmd( cmd );
}
QString BCLocalServer::GetCustomResolution(int liveW, int liveH, int preW, int preH, int syncW, int syncH, int totalW, int totalH, int /*polarityW*/, int /*polarityH*/, int hertz)
{
    QString cmd = QString("newddcmodetype 8 %1 %2 %3 %4 %5 %6 %7 %8 %9 %10\r\n")
            .arg(preW).arg(totalW-liveW-preW-syncW).arg(syncW).arg(liveW)
            .arg(preH).arg(totalH-liveH-preH-syncH).arg(syncH).arg(liveH)
            .arg(0).arg(hertz);

    return cmd;
}
void BCLocalServer::SetResolution(int schid, int echid, int resolution)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("resolution %1 %2 %3\r\n").arg(schid).arg(echid).arg(resolution);
    } else {
        switch ( resolution ) { // VP2000分辨率和VP4000有差异
        case 0: // 1920*1080
            resolution = 3;
            break;
        case 1: // 1400*1050
            resolution = 2;
            break;
        case 2: // 1366*768
            resolution = 1;
            break;
        default:// 1024*768
            resolution = 0;
            break;
        }
        cmd = QString("resolution %1\r\n").arg(resolution);
    }

    SendCmd( cmd );
}
QString BCLocalServer::GetResolution(int schid, int echid, int resolution)
{
    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("resolution %1 %2 %3\r\n").arg(schid).arg(echid).arg(resolution);
    } else {
        switch ( resolution ) { // VP2000分辨率和VP4000有差异
        case 0: // 1920*1080
            resolution = 3;
            break;
        case 1: // 1400*1050
            resolution = 2;
            break;
        case 2: // 1366*768
            resolution = 1;
            break;
        default:// 1024*768
            resolution = 0;
            break;
        }
        cmd = QString("resolution %1\r\n").arg(resolution);
    }

    return cmd;
}
void BCLocalServer::SetGFormartxy(int gid, int x, int y, int resolutionX, int resolutionY)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("setgformatxy %1 %2 %3 %4 %5\r\n").arg(gid).arg(x).arg(y).arg(resolutionX).arg(resolutionY);
    } else if (BCCommon::g_nDeviceType == 0) {   // VP2000
        cmd = QString("setformatxy %1 %2 %3 %4\r\n").arg(x).arg(y).arg(resolutionX).arg(resolutionY);
    }

    SendCmd( cmd );
}
QString BCLocalServer::GetGFormartxy(int gid, int x, int y, int resolutionX, int resolutionY)
{
    QString cmd;
    if ((BCCommon::g_nDeviceType == 1) || (BCCommon::g_nDeviceType == 2)) { // VP2000A, VP4000
        cmd = QString("setgformatxy %1 %2 %3 %4 %5\r\n").arg(gid).arg(x).arg(y).arg(resolutionX).arg(resolutionY);
    } else if (BCCommon::g_nDeviceType == 0) {   // VP2000
        cmd = QString("setformatxy %1 %2 %3 %4\r\n").arg(x).arg(y).arg(resolutionX).arg(resolutionY);
    }

    return cmd;
}

void BCLocalServer::SendCmd(const QString &cmd, bool bAddDebug)
{
    // 如果是演示模式直接返回
    if (0 == m_nIsDemoMode)
        return;

    // 更新执行时间
    m_timeOfPreview = QTime::currentTime();

    if ( m_bIsNetConnect ) {
        SendTcpData( cmd );
    } else {
        // 串口
        SendSerialPortData( cmd );
    }

    // 添加到调试窗口
    if ( bAddDebug ) {
        MainWindow *pMainWindow = BCCommon::Application();
        pMainWindow->AddDebugCmd( cmd );
    }
}

void BCLocalServer::SendTcpData(const QString &cmd, int cmdLen)
{
    // 判断是否包含汉字
    qDebug() << "TCP: " << cmd;
    this->AddLog( "TCP: "+cmd );
    if (NULL != m_pTcpSocket) {
        cmdLen = (-1 == cmdLen) ? cmd.length() : cmdLen;
        m_pTcpSocket->write(cmd.toLatin1(), cmdLen);
        m_pTcpSocket->flush();
    }
}

void BCLocalServer::SendSerialPortData(const QString &cmd)
{
    //qDebug() << "SERIAL: " << cmd;
    this->AddLog( "SERIAL: "+cmd );
    if ( !m_serial.isOpen() )
        return;

    m_serial.write(cmd.toLatin1(), cmd.length());
    m_serial.waitForBytesWritten( 50 );
    m_serial.flush();
}

void BCLocalServer::AddLog(QString text)
{
    QFile file( "log.txt" );
    QTextStream stream( &file);
    if ( file.open(QFile::ReadWrite |QIODevice::Append) ) {
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << ": " << text << "\r\n";
        file.close();
    }
}
