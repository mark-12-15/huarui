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

QString BCLocalServer::commandWithHeader()
{
    QString cmd;
    cmd.append(QChar(0x48));
    cmd.append(QChar(0x55));
    cmd.append(QChar(0x41));
    cmd.append(QChar(0x52));
    cmd.append(QChar(0x59));
    cmd.append(QChar(0x43));
    cmd.append(QChar(0xFF));

    return cmd;
}

uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
    uint32_t crc = crcIn;
    uint32_t in = byte|0x100;
    do
    {
        crc <<= 1;
        in <<= 1;
        if(in&0x100)
        ++crc;
        if(crc&0x10000)
        crc ^= 0x1021;
    } while(!(in&0x10000));
    return crc&0xffffu;
}

uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
    uint32_t crc = 0;
    const uint8_t* dataEnd = data+size;
    while(data<dataEnd)
        crc = UpdateCRC16(crc,*data++);
    crc = UpdateCRC16(crc,0);
    crc = UpdateCRC16(crc,0);
    return crc&0xffffu;
}

QString BCLocalServer::commandWithCheckout(const QString &cmd)
{
    auto res = cmd;
    uint8_t buffer[cmd.length()];
    for (int i = 0; i < cmd.length(); i++) {
        buffer[i] = cmd.at(i).toLatin1();
    }

    unsigned short c = Cal_CRC16(buffer, cmd.length());
    res.append(QChar((unsigned char)(c>>8)));
    res.append(QChar((unsigned char)c));
    return res;
}

QString BCLocalServer::initDeviceCommand()
{
    auto cmd = commandWithHeader();
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x0F));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));

    return cmd;
}

QString BCLocalServer::formatCommand()
{
    auto cmd = commandWithHeader();
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x0E));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x84));

    return cmd;
}

QString BCLocalServer::colorCommand()
{
    auto cmd = commandWithHeader();
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x0E));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x82));

    return cmd;
}

QString BCLocalServer::lightCommand()
{
    auto cmd = commandWithHeader();
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x0E));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x81));

    return cmd;
}

QString BCLocalServer::displayPowerCommand()
{
    auto cmd = commandWithHeader();
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x0E));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x83));

    return cmd;
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
    //_fullScreenMode = docElem.attribute("fullMode") == 1 ? true : false;
    m_qsConnectIPWithoutDLL = docElem.attribute("ConnectIPWithoutDLL");
    m_qsConnectPortWithoutDLL = docElem.attribute("ConnectPortWithoutDLL");
    m_qsCurrentCom = docElem.attribute("CurrentCom");
    m_nCurrentBaudRate = docElem.attribute("CurrentBaudRate").toInt();
    m_nCurrentDataBit = 8;//docElem.attribute("CurrentDataBit").toInt();
    m_nCurrentStopBit = 1;//docElem.attribute("CurrentStopBit").toInt();
    m_qsCurrentCheckBit = "None";//docElem.attribute("CurrentCheckBit") ;
    m_qsCurrentStreamCtrl = "None";//docElem.attribute("CurrentStreamCtrl");
}

void BCLocalServer::updateCommunicationPara()
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
    //docElem.setAttribute("fullMode", _fullScreenMode ? 1 : 0);
    docElem.setAttribute("CurrentCom", m_qsCurrentCom);
    docElem.setAttribute("CurrentBaudRate", m_nCurrentBaudRate);
//    docElem.setAttribute("CurrentDataBit", m_nCurrentDataBit);
//    docElem.setAttribute("CurrentStopBit", m_nCurrentStopBit);
//    docElem.setAttribute("CurrentCheckBit", m_qsCurrentCheckBit);
//    docElem.setAttribute("CurrentStreamCtrl", m_qsCurrentStreamCtrl);

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
        m_serial.close();
        BCCommon::g_bConnectStatusOK = true;
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
    if (BCCommon::g_bConnectStatusOK)
    {
        updateCommunicationPara();

        for (int i = 0; i < 5; i++) _commandAck[i] = 0;

        // 需要分开发送

        // 初始化，获得当前规模、亮度值、RGB值
        SendCmd(initDeviceCommand());

//        SendCmd(formatCommand());
//        SendCmd(lightCommand());
//        SendCmd(colorCommand());
//        SendCmd(displayPowerCommand());
    }
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

    auto info = m_serial.readAll();
    if (info.length() > 5 && info.at(0) == 0x06)
    {
        unsigned char hn = info.at(2);
        unsigned char ln = info.at(3);
        unsigned short len = (hn << 8) | ln;

        unsigned char type = info.at(4);
        if (0x00 == type)
        {
            // init success3
            //m_pOneSecondTimer->start();

            _commandAck[0] = 1;
            SendCmd(formatCommand());
        }
        else if (0x84 == type && len == 14)
        {
            // 规模
            //0x06 0xFF 0x00 0x0E 0x84 0x00 0x04 0x02 0x03 0x20 0x02 0x58 xx xx
            bool fullMode = (unsigned char)info.at(5) == 0x00;
            unsigned short arrX = info.at(6);
            unsigned short arrY = info.at(7);
            unsigned char hw = info.at(8);
            unsigned char lw = info.at(9);
            unsigned short w = (hw << 8) | lw;

            unsigned char hh = info.at(10);
            unsigned char lh = info.at(11);
            unsigned short h = (hh << 8) | lh;

            // 有变化则更新配置文件并刷新教室
            if (isUpdateRoomConfig(fullMode, arrX, arrY, w, h))
            {
                BCCommon::Application()->RefreshMainWindow();
            }

            _commandAck[1] = 1;
            SendCmd(lightCommand());
        }
        else if (0x81 == type && len == 8)
        {
            // 0x06 0xFF 0x00 0x07 0x81 Value xx xx
            _lightValue = info.at(5);

            _commandAck[2] = 1;
            SendCmd(colorCommand());
        }
        else if (0x82 == type && len == 10)
        {
            // 0x06 0xFF 0x00 0x0A 0x82 R G B xx xx
            _rValue = info.at(5);
            _gValue = info.at(6);
            _bValue = info.at(7);

            _commandAck[3] = 1;
            SendCmd(displayPowerCommand());
        }
        else if (0x83 == type && len == 8)
        {
            // 0x06 0xFF 0x00 0x08 0x83 value xx xx
            _displayPower = (info.at(5) == 1);

            _commandAck[4] = 1;

            emit roomStateChanged();
        }
    }
}

void BCLocalServer::onRecvTcpData()
{
    m_nHeartTimes = 0;

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
            sdisplay.id = j*arrX + i;
            sdisplay.name = QString("%1").arg(j*arrX + i, 3, 10, QChar('0'));
            sdisplay.left = i*width;
            sdisplay.top = j*height;
            sdisplay.resolutionX = width;
            sdisplay.resolutionY = height;
            sdisplay.segmentation = 4;

            sgroupdisplay.lstDisplay.append( sdisplay );
        }
    }

    sRoom sroom;
    sroom.id = 0;
    sroom.name = name;
    sroom.width = arrX*width;
    sroom.height = arrY*height;
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
    sroom.fullMode = docElem.attribute("fullMode").toInt() == 1 ? true : false;

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

bool BCLocalServer::isUpdateRoomConfig(bool fullMode, int arrX, int arrY, int width, int height)
{
    QFile file( QString("../xml/BCRoomConfig.xml") );
    if(!file.open(QIODevice::ReadOnly)){
        return false;
    }

    // 将文件内容读到QDomDocument中
    QDomDocument doc;
    bool bLoadFile = doc.setContent(&file);
    file.close();

    if ( !bLoadFile )
        return false;

    // 二级链表
    QDomElement docElem = doc.documentElement();
    auto fmode = docElem.attribute("fullMode").toInt();
    auto farrX = docElem.attribute("arrayX").toInt();
    auto farrY = docElem.attribute("arrayY").toInt();
    auto fwidth = docElem.attribute("width").toInt();
    auto fheight = docElem.attribute("height").toInt();

    if (fmode != (fullMode?1:0)
            || farrX != arrX || farrY != arrY
            || fwidth != width || fheight != height)
    {
        docElem.setAttribute("fullMode", QString::number(fullMode?1:0));
        docElem.setAttribute("arrayX", QString::number(arrX));
        docElem.setAttribute("arrayY", QString::number(arrY));
        docElem.setAttribute("width", QString::number(width));
        docElem.setAttribute("height", QString::number(height));

        // 清空场景和输入通道
        while (!docElem.childNodes().isEmpty())
            docElem.removeChild(docElem.firstChild());

        // 重新添加输入通道
        auto count = arrX * arrY;
        if (count > 0) {
            auto eleCh = doc.createElement(QString("Channel"));

            for (int i = 0; i < count; i++) {
                auto eleNode = doc.createElement(QString("Node"));
                eleNode.setAttribute("id", QString::number(i));
                eleNode.setAttribute("name", QString("电脑%1").arg(i+1));

                eleCh.appendChild(eleNode);
            }

            docElem.appendChild(eleCh);
        }
    }
    else
    {
        return false;
    }

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return false;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();

    return true;
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

void BCLocalServer::updateFormatToDevice(bool fullMode, int arrX, int arrY, int width, int height)
{
    auto cmd = commandWithHeader();
    // 0x48 0x55 0x41 0x52 0x59 0x43 0xFF
    // 0x00 0x15 0x00 0x00 0x04 0x00 0x04 0x02 0x03 0x20 0x02 0x58 xx xx
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x15));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x04));

    cmd.append(QChar(fullMode ? 0x00 : 0x01));
    cmd.append(QChar((unsigned char)arrX));
    cmd.append(QChar((unsigned char)arrY));

    unsigned char hw = (unsigned short)width >> 8;
    unsigned char lw = (unsigned short)width;
    unsigned char hh = (unsigned short)height >> 8;
    unsigned char lh = (unsigned short)height;
    cmd.append(QChar(hw));
    cmd.append(QChar(lw));
    cmd.append(QChar(hh));
    cmd.append(QChar(lh));

    SendCmd(cmd);
}

void BCLocalServer::updateLightToDevice(int value)
{
    _lightValue = value;

    auto cmd = commandWithHeader();
    // 0x48 0x55 0x41 0x52 0x59 0x43 0xFF
    // 0x00 0x0F 0x00 0x00 0x01 Value xx xx
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x0F));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x01));

    cmd.append(QChar((unsigned char)value));

    SendCmd(cmd);
}

void BCLocalServer::updateColorToDevice(int r, int g, int b)
{
    _rValue = r;
    _gValue = g;
    _bValue = b;

    auto cmd = commandWithHeader();
    // 0x48 0x55 0x41 0x52 0x59 0x43 0xFF
    // 0x00 0x12 0x00 0x00 0x02 R G B xx xx
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x12));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x02));

    cmd.append(QChar((unsigned char)r));
    cmd.append(QChar((unsigned char)g));
    cmd.append(QChar((unsigned char)b));

    SendCmd(cmd);
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

void BCLocalServer::winsize(int /*gid*/, int chid, int winid, int l, int t, int r, int b, int /*type*/, int /*copyIndex*/)
{
    auto cmd = commandWithHeader();
    // 开窗1000*800, 使用的信号源是2，窗口序号是3
    // 0x48 0x55 0x41 0x52 0x59 0x43 0xFF
    // 0x00 0x18 0x00 0x00 0x06
    // 0x02 0x03 0x00 0x00 0x00 0x00 0x03 0xE8 0x03 0x20 xx xx

    cmd.append(QChar(0x00));
    cmd.append(QChar(0x18));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x06));

    cmd.append(QChar((unsigned char)chid));
    cmd.append(QChar((unsigned char)winid));

    unsigned char hl = (unsigned short)l >> 8;
    unsigned char ll = (unsigned short)l;
    cmd.append(QChar((unsigned char)hl));
    cmd.append(QChar((unsigned char)ll));

    unsigned char ht = (unsigned short)t >> 8;
    unsigned char lt = (unsigned short)t;
    cmd.append(QChar((unsigned char)ht));
    cmd.append(QChar((unsigned char)lt));

    unsigned char hw = (unsigned short)(r-l) >> 8;
    unsigned char lw = (unsigned short)(r-l);
    cmd.append(QChar((unsigned char)hw));
    cmd.append(QChar((unsigned char)lw));

    unsigned char hh = (unsigned short)(b-t) >> 8;
    unsigned char lh = (unsigned short)(b-t);
    cmd.append(QChar((unsigned char)hh));
    cmd.append(QChar((unsigned char)lh));

    SendCmd(cmd);
}

void BCLocalServer::winswitch(int /*gid*/, int winid, int chid, int /*type*/, int /*copyIndex*/)
{
    auto cmd = commandWithHeader();
    // 关窗, 使用的信号源是2，窗口序号是3
    // 0x48 0x55 0x41 0x52 0x59 0x43 0xFF
    // 0x00 0x10 0x00 0x00 0x08
    // 0x02 0x03 xx xx

    cmd.append(QChar(0x00));
    cmd.append(QChar(0x10));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x08));

    cmd.append(QChar((unsigned char)chid));
    cmd.append(QChar((unsigned char)winid));

    SendCmd(cmd);
}

void BCLocalServer::save(int /*groupID*/, int sceneID)
{
    auto cmd = commandWithHeader();
    // 保存场景
    // 0x48 0x55 0x41 0x52 0x59 0x43 0xFF
    // 0x00 0x0F 0x00 0x00 0x07 0x03 xx xx

    cmd.append(QChar(0x00));
    cmd.append(QChar(0x0F));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x07));
    cmd.append(QChar((unsigned char)sceneID));

    SendCmd(cmd);
}

void BCLocalServer::load(int /*groupID*/, int sceneID)
{
    auto cmd = commandWithHeader();
    // 保存场景
    // 0x48 0x55 0x41 0x52 0x59 0x43 0xFF
    // 0x00 0x0F 0x00 0x00 0x09 0x03 xx xx

    cmd.append(QChar(0x00));
    cmd.append(QChar(0x0F));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x09));
    cmd.append(QChar((unsigned char)sceneID));

    SendCmd(cmd);
}

bool BCLocalServer::isFullScreenMode()
{
    auto room = BCCommon::Application()->GetMRoom(0);
    if (room) {
        return room->isFullScreeMode;
    } else {
        return true;
    }
}

void BCLocalServer::onDelaySendCmd()
{
    if ( m_lstDelayCmd.isEmpty() )
        return;

    SendCmd( m_lstDelayCmd.takeFirst() );
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

void BCLocalServer::updateDisplayPowerToDevice(bool on)
{
    _displayPower = on;

    auto cmd = commandWithHeader();
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x0F));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x00));
    cmd.append(QChar(0x03));
    cmd.append(QChar(on ? 0x01 : 0x00));

    SendCmd(cmd);
}

void BCLocalServer::updateConfigFileToDevice(QByteArray ba)
{
    // 0x48 0x55 0x41 0x52 0x59 0x43 0xFF
    // N1 N2 0x00 0x00 0x05 FileDate xx xx
    auto cmd = commandWithHeader();

    const int singlePacketLen = 257;

    int len = (ba.size() / singlePacketLen) + 1;
    for (int i = 0; i < len; i++)
    {
        auto singleCmd = cmd;

        // 获取文件长度，单次最多发送1024
        unsigned short filelen = (i == len-1) ? ba.size()%singlePacketLen : singlePacketLen;
        // 添加指令部分长度
        unsigned short cmdlen = filelen + 14;

        cmd.append(QChar((unsigned char)(cmdlen>>8)));
        cmd.append(QChar((unsigned char)cmdlen));

        unsigned short index = i;
        cmd.append(QChar((unsigned char)(index>>8)));
        cmd.append(QChar((unsigned char)index));
        cmd.append(QChar(0x05));

        for (int j = i*singlePacketLen; j < i*singlePacketLen+filelen; j++) {
            cmd.append(ba.at(j));
        }
        //cmd.append(ba.mid(i*singlePacketLen, filelen));

        qDebug() << "pack len: " << cmd.length() << ", index: " << i;
        SendCmd(cmd);

        cmd = commandWithHeader();
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
    this->AddLog( "SERIAL: "+cmd );
    if ( !m_serial.isOpen() )
        return;

    // 添加校验后发送
    auto endCmd = commandWithCheckout(cmd);

    qInfo() << "device send serialport command: " << cmd;

    m_serial.write(endCmd.toLatin1(), endCmd.length());
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
