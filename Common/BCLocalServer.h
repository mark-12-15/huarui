/*********************************************************************************************************************************
* 作    者：liuwl
* 摘    要：本地服务器类，用于连接应用和db通讯，全局唯一
*********************************************************************************************************************************/
#ifndef BCLOCALSERVER_H
#define BCLOCALSERVER_H

#include <QMap>
#include <QHostAddress>
#include <QLibrary>
#include <QTimer>
#include <QDateTime>
#include <QtSerialPort/QSerialPort>
#include "BCMRoom.h"
#include "BCMWindowScene.h"

// 长春电力，控制的是接收外部指令
//#define CHANGCHUNPOWER

// main toolbar struct
struct sGroup {
    int group;
    QList<int> lstButton;
};

struct sTab{
    int tab;
    QList<sGroup> lstGroup;
};

// matrix struct
struct sMatrixNode {
    int id;
    QString name;
    int nSwitch;        // 是否被切换信号
    int nSwitchID;      // 切换的信号ID

    int jointWithVP4000ChannelID;   // 级联设备的输入通道ID
    int jointWithVP2000ChannelType; // 级联设备的输入通道Type

    int isOn;                       // 大屏开关状态
    QString qsOnCmd;                // 打开指令
    QString qsOffCmd;               // 关闭指令

    int cutl;
    int cutr;
    int cutt;
    int cutb;
};

struct sMatrixScene {
    int id;
    QString name;

    QList<QPoint> lstSwitchInfo;        // 点的X代表入口ID，Y代表出口ID
};

struct sMatrix {
    int id;
    QString name;

    int isConnectByNet;             // 是否网络通信
    QString ip;                     // 网络通信IP
    int port;                       // 网络通信端口
    QString qsCurrentCom;           // 串口号
    int nCurrentBaudRate;           // 波特率
    int nCurrentDataBit;            // 数据位
    int nCurrentStopBit;            // 停止位
    QString qsCurrentCheckBit;      // 校验位
    QString qsCurrentStreamCtrl;    // 控制流

    int isOn;                       // 大屏开关状态
    QString qsOnCmd;                // 打开指令
    QString qsOffCmd;               // 关闭指令

    int cmdType;        // 指令类型，0：16进制，1：Ascii码
    QString switchFlag; // 切换指令表达式，如SW %1 %2...
    QString loadFlag;   // 调取指令，如%1.
    QString saveFlag;   // 保存指令，如%1,

    int jointSceneRoomID;           // 是否关联调用场景，关联的拼控调用场景时将调用当前矩阵的场景

    int jointWithVP4000;            // 是否联控设备，如果联控设备矩阵输出口直接对应设备的输入口

    QList<sMatrixNode>  lstInputNode;   // 输入节点
    QList<sMatrixNode>  lstOutputNode;  // 输出节点
    QList<sMatrixScene> lstScene;       // 场景列表
};

// inputchannel
struct sInputChannel {
    int id;
    int type;
    QString deviceip;
    QString basename;
    QString name;
    QString remoteIP;
    int signalsource;
    int subtitlePower;  // 字幕开关
    int cutleft;
    int cutright;
    int cuttop;
    int cutbottom;
    int boardcardid;    // 板卡ID
    int boardcardpos;   // 板卡位置
};

// 用户自定义输入通道
struct sCustomInputChannel {
    int id;
    QString name;
    QList<sInputChannel> lstData;
};

// device config
struct sDisplay {
    int id;
    QString name;
    int left;
    int top;
    int resolutionX;
    int resolutionY;
    int segmentation;
    QString switchoncmd;
    QString switchoffcmd;
    int switchStatus;           // 打开状态
    int ledresolutionX;
    int ledresolutionY;
};

struct sGroupDisplay {
    int id;
    QString name;
    int left;
    int top;
    int width;
    int height;
    int arrayX;
    int arrayY;
    int isUseVir;
    int virX;
    int virY;
    int resolutionX;
    int resolutionY;
    int segmentationX;
    int segmentationY;

    QList<sDisplay> lstDisplay;
};

struct sRoom {
    int id;
    QString name;
    int type;
    int width;
    int height;

    int isNetConnect;           // 是否是网络通讯
    QString switchip;           // ip
    int switchport;             // port
    QString switchCom;          // com口
    int switchBaudRate;         // 波特率
    int switchDataBit;          // 数据位
    int switchStopBit;          // 停止位
    QString switchCheckBit;     // 校验位
    QString switchStreamCtrl;   // 数据流
    int switchtype;             // 0:16 1:text
    QString switchoncmd;
    QString switchoffcmd;
    int switchStatus;           // 打开状态

    bool fullMode;

    QList<sGroupDisplay> lstGroupDisplay;
};

// 通过软件修改设备规模
struct sRoomConfig {
    int id;             // 屏组ID
    QString name;
    int type;           // 类型，0:LCD 1:LED 2:融合 3:4K
    int arrayX;         // 排列X
    int arrayY;         // 排列Y
    int resolutionX;    // 单屏分辨率X
    int resolutionY;    // 单屏分辨率Y
    int isUseVir;       // 是否使用虚拟分辨率
    int virWidth;
    int virHeight;
    int isUseVirSeg;    // 是否需用虚拟分割
    int virArrX;
    int virArrY;
};

// 场景数据结构体
struct sWindowSceneData {
    int chid;           // 信号ID
    int chtype;         // 信号type
    int winid;          // window id
    int copyIndex;      // 第几个复制窗口
    int groupdisplayid; // 屏幕ID
    int left;           // 实际位置
    int top;
    int width;
    int height;

    int ipvSegmentation;// IPV分割数
    QStringList lstIP;  // IPV使用的IPV链表，共16个
};

// 场景
struct sWindowScene {
    int id;
    int cycle;
    int loopInterval;
    QString name;

    QList<sWindowSceneData> lstData;
};

// 场景组
struct sGroupScene {
    int id;
    int normalloop;
    int loop;
    int loopInterval;
    QString name;

    QList<sWindowScene> lstData;
};

// 计划任务
struct sTaskPlanning {
    int id;                 // 任务ID，从0排序
    int taskType;           // 任务类型，0：开机，1：关机，2：打开轮训，3：关闭轮训，4：调用场景
    QString cycle;          // 循环周期，从1-7，中间用空格分开，如"1 2 7"
    QString time;           // 执行时间
    int creatorID;          // 创建人ID
    QString createTime;     // 创建时间
    int roomType;           // 房间类型，0：拼接，1：矩阵
    int roomID;             // 房间ID
    int sceneID;            // 场景ID

    bool bExec;             // 是否执行
};

// ------------------------------------------------------------------------------------------------------------------------
// DLL结构体

// 单屏结构体
struct BSDisplay {
    int		id;
    char*	name;		// 名称
    int		resolutionX;	// 单屏分辨率X
    int		resolutionY;	// 单屏分辨率Y

    int		left;			// 以下四个属性计算单屏在屏组内的相对位置，是实际坐标；如果是标准分割也可以不提供
    int		top;
    int		width;
    int		height;

    BSDisplay	*pNext;		// 单链表
};

// 屏组结构体
struct BSGroupDisplay {
    int		id;
    char*	name;		// 名称
    int		arrayX;			// 屏幕排列X
    int		arrayY;			// 屏幕排列Y
    int		resolutionX;	// 单屏分辨率X
    int		resolutionY;	// 单屏分辨率Y
    int		left;			// 以下四个属性计算屏组在房间内的相对位置，是实际坐标
    int		top;
    int		width;
    int		height;

    BSGroupDisplay	*pNext;				// 单链表

    BSDisplay		*pFirstDisplays;	// 单屏链表
};

// 输入通道结构体
struct BSInputChannel {
    int		id;
    char	*name;			// 名称
    int		type;			// 通道类型，0电脑 1视频 2高清
    int		signalSource;	// 信号源 0 VGA  1 DVI  2 CVBS  3 YPbPr  4 HDMI  5 S-V  6 HDVI  7 SDI  8 DP  9 IPV  10 HDBaseT  11 FIBER  注：CVBS 视频  HDVI 高清  其余都是电脑

    // 说明：通道中不存IPV数据，这里涉及到物理链表和窗口使用链表
    //		1.物理链表存储在系统本地数据库内，数量没有限制
    //		2.窗口使用的链表存在场景数据中
    //int				nIPVSegmentation;	// IPVedio使用，也就是只有当signalSource=9时才生效，数据的分割数
    //BSIPVedioIP		*pCurrentIP;		// IPVedio使用，也就是只有当signalSource=9时才生效，当前使用的IP

    BSInputChannel	*pNext;	// 单链表
};

// 场景的数据类
struct BSWindowSceneData
{
    int		cid;	// 信号ID
    int     ctype;
    int		winid;	// window id
    int		left;
    int		top;
    int		width;
    int		height;

    // 是否是IPV根据channelID到物理链表中去查找
    int		nIPVSegmentation;		// IPV的分割数
    char	cIPVeioIP[16][255];		// IPV的IP链表

    BSWindowSceneData *pNext;	// 单链表指针
};

// 板卡上的节点
struct BSBoardCardNode {
    int		boardCardPos;	// 板卡的位置，因为索引可能有重复的，这里代表板卡的唯一标识
    int		group;			// 组号
    int		port;			// 端口，如0，2，4，用来放置节点图片的实际位置
    int		portName;		// 显示的端口号
    int		valid;			// 是否有效，如0，1
    int		xx;				// 分辨率
    int		yy;
    int		arrayX;			// 排列
    int		arrayY;
    int		arrayPosX;		// 排列的位置
    int		arrayPosY;
    int     cardType;		// 板卡的类型，1:B_Input,2:B_Output,3:B_Blank
    int     type;			// 类型，1:p_DVI,2:p_HDBT

    BSBoardCardNode	*pNext;		// 单链表
};

// 板卡结构体
struct BSBoardCard {
    int		pos;		// 板卡的位置，如0，1，2...28
    int		index;		// 板卡的索引，如0，1，2，255
    int     type;		// 板卡的类型，1:B_Input,2:B_Output,3:B_Blank

    BSBoardCardNode	*pNode;		// 板卡内节点

    BSBoardCard		*pNext;		// 单链表
};

// 板卡状态结构体
struct BSBoardCardStatus {
    int		pos;						// 板卡的位置，如0，1，2...28，定位板卡使用

    int		nCoreVoltageOfChip1;		// 芯片1核心电压
    int		nRoundVoltageOfChip1;		// 芯片1外围电压
    int		nCoreElectricityOfChip1;	// 芯片1核心电流
    int		nRoundElectricityOfChip1;	// 芯片1外围电流

    int		nCoreVoltageOfChip2;		// 芯片2核心电压
    int		nRoundVoltageOfChip2;		// 芯片2外围电压
    int		nCoreElectricityOfChip2;	// 芯片2核心电流
    int		nRoundElectricityOfChip2;	// 芯片2外围电流

    int		nVoltageOfMemorizer;		// 存储器电压
    int		nElectricityOfMemorizer;	// 存储器电流

    int		nVoltageOfInput;			// 入口电压
    int		nElectricityOfInput;		// 入口电流

    BSBoardCardStatus	*pNext;			// 单链表，存储多个板卡信息
};

// 设备状态结构体
struct BSDeviceStatus {
    int		nVoltageOfDevice;			// 设备电压
    int		nElectricityOfDevice;		// 设备电流

    int		nRotationSpeedOfFan1;		// 风扇1转速
    int		nRotationSpeedOfFan2;		// 风扇2转速
    int		nRotationSpeedOfFan3;		// 风扇3转速
    int		nRotationSpeedOfFan4;		// 风扇4转速
    int		nRotationSpeedOfFan5;		// 风扇5转速
    int		nRotationSpeedOfFan6;		// 风扇6转速
    int		nRotationSpeedOfFan7;		// 风扇7转速
    int		nRotationSpeedOfFan8;		// 风扇8转速
};

// 设备数据
struct BSDevice {
    int		selectFlag;	// 是否被选择，1为选择，0为不选择

    char    name[6];	// 设备name
    char    ip[4];		// 设备IP
    int     port;       // 设备端口
    char	mask[4];	// 子网掩码
    char	gateway[4];	// 网关
    char	mac[6];		// mac地址

    BSDevice	*pNext;	// 单链表
};

class BCLocalDatabase;
class QTcpSocket;
class QUdpSocket;
class BCLocalServer : public QObject
{
    Q_OBJECT

public:
    // 静态接口构造和销毁
    static BCLocalServer *Application();
    static void Destroy();

    // 在bin目录下生成log.txt
    void AddLog(QString text);

    // 房间配置
    sRoom GetRoomConfig();
    QList<sWindowScene> GetRoomScenes();
    QList<sInputChannel> GetInputChannels();

    void UpdateRoomName(QString str);
    void UpdateRoomSwitchConfig(BCMRoom *room);
    bool isUpdateRoomConfig(bool fullMode, int arrX, int arrY, int width, int height);

    void AddScreen(BCMWindowScene *screen);
    void RemoveScreen(int id);
    void UpdateSceneSort(QMap<int, int> map);

    void UpdateInputChannel(int id, QString name);

    // 发送指令
    void updateFormatToDevice(bool fullMode, int arrX, int arrY, int width, int height);
    void updateLightToDevice(int value);
    void updateColorToDevice(int r, int g, int b);
    void updateDisplayPowerToDevice(bool on);
    void updateConfigFileToDevice(QByteArray ba);

    // 矩阵配置
    QList<sMatrix> GetMatrixConfig();
    void SetMatrixConfig(const QList<sMatrix> lstMatrix);
    void SetMatrixName(int roomid, int output, int id, const QString &name);    // 参数2表示是否是输出口，0输入，1输出；
    void SetMatrixSwitch(int id, int inputid, int outputid);
    void AddMatrixScene(int roomid, int id, const QString &name, QList<QPoint> lst);
    void UpdateMatrixScene(int roomid, int id, QList<QPoint> lst);
    void UpdateMatrixScene(int roomid, int id, const QString &name);
    void RemoveMatrixScene(int roomid, int id);
    void ClearMatrixScene(int roomid);
    void UpdateMatrixRoomSwitch(int roomid, int isOn);
    void UpdateMatrixRoomName(int roomid, const QString &name);
    void UpdateMatrixOutputSwitch(int roomid, int id, int isOn);
    void UpdateMatrixJointChannel(int id, QList<QPoint> lst);   // 更新矩阵的级联通道
    void UpdateMatrixInputCut(int roomid, int id, int l, int r, int t, int b);
    // -------------------------------------------------------------------------------------------------------------------------------------

    // connect with dll
    // --------------------------------------------------------------------- 指令
    void reset(const QString &qsGroupDisplayIDs);
    void winsize(int gid, int chid, int winid, int l, int t, int r, int b, int type, int copyIndex);
    void winswitch(int gid, int winid, int chid, int type, int copyIndex);
    void save(int groupID, int sceneID);
    void load(int groupID, int sceneID);

    bool isFullScreenMode();

    // 获取配置的类型
    // 0:正常获取
    bool        m_bIsNetConnect{false};     // 是否是网络通信
    QString     m_qsConnectIPWithoutDLL;    // 如果不使用DLL时，与当前IP通信
    QString     m_qsConnectPortWithoutDLL;  // 如果不使用DLL时，与当前IP通信
    QString     m_qsConnectMacWithoutDLL;    // 如果不使用DLL时，与当前IP通信
    QString     m_qsCurrentCom;             // 当前COM口
    int         m_nCurrentBaudRate;         // 当前波特率
    int         m_nCurrentDataBit;          // 当前数据位
    int         m_nCurrentStopBit;          // 当前停止位
    QString     m_qsCurrentCheckBit;        // 当前校验位
    QString     m_qsCurrentStreamCtrl;      // 当前流控制

    unsigned int         _lightValue{20};             // 亮度值和颜色值
    unsigned int         _rValue{20};
    unsigned int         _gValue{20};
    unsigned int         _bValue{20};
    bool                _displayPower{false};

    bool        m_bIsLoadDataOK;            // 是否加载完数据，只在不使用DLL时有效
    int         m_nIsDemoMode;              // 是否是演示模式，0：演示模式不能发送指令，1：演示模式可以发送指令，2：不是演示模式

    void SetLoadDataOK();                   // 设置加载完数据，连接通讯
    void DisConnect();                      // 断开连接
    void ReConnect();                       // 重新连接

    // 初始化通信参数
    void InitCommunicationPara();
    void updateCommunicationPara();
    void ConnectSerialPort();

    void SendCmd(const QString &cmd, bool bAddDebug = true);

signals:
    void roomStateChanged();

private slots:
    void onTimeout();
    void SendTcpData(const QString &cmd, int cmdLen=-1);
    void SendSerialPortData(const QString &cmd);
    void onRecvSerialData();
    void onRecvTcpData();

    void onDelaySendCmd();

private:
    BCLocalServer();
    ~BCLocalServer();

    QString commandWithHeader();
    QString commandWithCheckout(const QString &cmd);

    QString initDeviceCommand();
    QString formatCommand();
    QString colorCommand();
    QString lightCommand();
    QString displayPowerCommand();

    static BCLocalServer    *m_pLocalServer;    // 全局变量

    BCLocalDatabase         *m_pDB;             // 数据库

    QTcpSocket      *m_pTcpSocket;              // TCP通讯
    QSerialPort     m_serial;                   // 串口通讯

    int             m_nHeartTimes;              // 心跳包的次数，串口和网口共享变量

    // 一秒定时器，做两件事
    // 1.如果设备运行不正常则反复读取设备信息，直到正常
    // 2.如果打开同步功能，则反复读取窗口信息
    QTimer      *m_pOneSecondTimer;
    bool        m_bDeviceWorking;       // 标记设备是否正常工作
    bool        m_bSynchronization;     // 标记是否同步

    QTime       m_timeOfPreview;        // 10ms内执行过指令则不发送心跳包

    QStringList m_lstDelayCmd;          // 延时发送的指令
};

#endif // BCLOCALSERVER_H
