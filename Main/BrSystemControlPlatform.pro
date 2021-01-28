#-------------------------------------------------
#
# Project created by QtCreator 2016-05-06T13:49:55
#
#-------------------------------------------------

QT       += core gui network xml axcontainer
QT += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = markView
TEMPLATE = app
INCLUDEPATH += $$PWD/../Setting
INCLUDEPATH += $$PWD/../Common
INCLUDEPATH += $$PWD/../Model
INCLUDEPATH += $$PWD/../View


SOURCES +=\
    ../Setting/AdminPasswordDlg.cpp \
    ../Setting/ColorSettingDlg.cpp \
    ../Setting/DeviceConnectDlg.cpp \
    ../Setting/DeviceFormatDlg.cpp \
    ../Setting/LightSettingDlg.cpp \
        MainWindow.cpp \
    ../Common/BCCommon.cpp \
    ../Model/BCMChannel.cpp \
    ../Model/BCMDisplay.cpp \
    ../Model/BCMGroupDisplay.cpp \
    ../Model/BCMGroupScene.cpp \
    ../Model/BCMWindowScene.cpp \
    ../View/BCToolBar.cpp \
    ../View/BCScreenName.cpp \
    ../Model/BCMRoom.cpp \
    ../View/BCScreenDlg.cpp \
    ../Setting/BCSettingMainPanelStyle.cpp \
    ../Setting/BCSettingPasswordStyle.cpp \
    ../View/BCSystemplan.cpp \
    ../View/BCSignal.cpp \
    ../View/BCScene.cpp \
    ../View/BCControl.cpp \
    ../View/BCWidgetBtn.cpp \
    ../View/BCFaceWidget.cpp \
    ../View/BCSignalTreeWidget.cpp \
    ../View/BCRibbonMainToolBar.cpp \
    ../View/BCRibbonMainToolBarAction.cpp \
    ../View/BCRibbonBar.cpp \
    ../View/BCSignalListWidgetData.cpp \
    ../View/BCAutoDateDlg.cpp \
    ../View/BCSignalName.cpp \
    ../Setting/BCLoginDlg.cpp \
    ../Common/BCXMLManager.cpp \
    ../Common/BCLocalServer.cpp \
    main.cpp \
    ../Model/BCMGroupChannel.cpp \
    ../Setting/BCSettingDisplayInfoDlg.cpp \
    ../Setting/BCSettingDisplaySetNetSetDlg.cpp \
    ../View/BCLocationDlg.cpp \
    ../View/BCLocationSegmentationWidget.cpp \
    ../Setting/BCSettingOutOfDateDlg.cpp \
    ../Setting/BCSettingSignalWindowPropertyDlg.cpp \
    ../Setting/BCSceneListWidgetData.cpp \
    ../Setting/BCSettingWaitingDlg.cpp \
    ../Common/BCLocalTcpSocket.cpp \
    ../Setting/BCSettingBoardCardDlg.cpp \
    ../Setting/BCSettingOutSideCommandDlg.cpp \
    ../Setting/BCSettingAddIPVedioTreeWidgetItemDlg.cpp \
    ../Common/BCLocalSerialPort.cpp \
    ../Setting/BCSettingDeviceCustomResolutionDlg.cpp \
    ../Setting/BCSettingDisplaySwitchConfigDlg.cpp \
    ../Setting/BCSettingDisplaySwitchRoomWidget.cpp \
    ../View/BCMatrixRoomWidget.cpp \
    ../View/BCMatrixInputNodeWidget.cpp \
    ../View/BCMatrixOutputNodeWidget.cpp \
    ../View/BCSignalWindowDisplayWidget.cpp \
    ../View/BCRoomWidget.cpp \
    ../View/BCSingleDisplayWidget.cpp \
    ../View/BCGroupDisplayWidget.cpp \
    ../View/BCSingleDisplayVirtualWidget.cpp \
    ../Setting/BCSettingAutoReadInputChannelConfigDlg.cpp \
    ../Setting/BCSettingDebugDlg.cpp \
    ../View/BCControlTreeWidget.cpp \
    ../View/BCSceneMatrixTreeWidget.cpp \
    ../Setting/BCSettingModifyNameDlg.cpp \
    ../Setting/BCSettingMatrixFormatDlg.cpp \
    ../Setting/BCSettingDisplayMakerConfigDlg.cpp \
    ../Model/BCMMatrix.cpp \
    ../Setting/BCSettingJointMatrixChannelConfigDlg.cpp \
    ../Setting/BCSettingMatrixCutInputChannelDlg.cpp \
    ../View/BCSignalSrouceSceneViewWidget.cpp \
    ../Common/BCOutsideInterfaceServer.cpp \
    ../Setting/BCSettingOutsideInterfaceDlg.cpp \
    ../Setting/BCSettingRepeatApplicationWarningDlg.cpp \
    ../View/BCLocationGroupWidget.cpp


HEADERS  += MainWindow.h \
    ../Common/BCCommon.h \
    ../Model/BCMChannel.h \
    ../Model/BCMDisplay.h \
    ../Model/BCMGroupDisplay.h \
    ../Model/BCMGroupScene.h \
    ../Model/BCMWindowScene.h \
    ../Setting/AdminPasswordDlg.h \
    ../Setting/ColorSettingDlg.h \
    ../Setting/DeviceConnectDlg.h \
    ../Setting/DeviceFormatDlg.h \
    ../Setting/LightSettingDlg.h \
    ../View/BCToolBar.h \
    ../View/BCScreenName.h \
    ../Model/BCMRoom.h \
    ../View/BCScreenDlg.h \
    ../Setting/BCSettingMainPanelStyle.h \
    ../Setting/BCSettingPasswordStyle.h \
    ../View/BCSystemplan.h \
    ../View/BCSignal.h \
    ../View/BCScene.h \
    ../View/BCControl.h \
    ../View/BCWidgetBtn.h \
    ../View/BCFaceWidget.h \
    ../View/BCSignalTreeWidget.h \
    ../View/BCRibbonMainToolBar.h \
    ../View/BCRibbonMainToolBarAction.h \
    ../View/BCRibbonBar.h \
    ../View/BCSignalListWidgetData.h \
    ../View/BCAutoDateDlg.h \
    ../View/BCSignalName.h \
    ../Setting/BCLoginDlg.h \
    ../Common/BCXMLManager.h \
    ../Common/BCLocalServer.h \
    ../Model/BCMGroupChannel.h \
    ../Setting/BCSettingDisplayInfoDlg.h \
    ../Setting/BCSettingDisplaySetNetSetDlg.h \
    ../View/BCLocationDlg.h \
    ../View/BCLocationSegmentationWidget.h \
    ../Setting/BCSettingOutOfDateDlg.h \
    ../Setting/BCSettingSignalWindowPropertyDlg.h \
    ../Setting/BCSceneListWidgetData.h \
    ../Setting/BCSettingWaitingDlg.h \
    ../Common/BCLocalTcpSocket.h \
    ../Setting/BCSettingBoardCardDlg.h \
    ../Setting/BCSettingOutSideCommandDlg.h \
    ../Setting/BCSettingAddIPVedioTreeWidgetItemDlg.h \
    ../Common/BCLocalSerialPort.h \
    ../Setting/BCSettingDeviceCustomResolutionDlg.h \
    ../Setting/BCSettingDisplaySwitchConfigDlg.h \
    ../Setting/BCSettingDisplaySwitchRoomWidget.h \
    ../View/BCMatrixRoomWidget.h \
    ../View/BCMatrixInputNodeWidget.h \
    ../View/BCMatrixOutputNodeWidget.h \
    ../View/BCSignalWindowDisplayWidget.h \
    ../View/BCRoomWidget.h \
    ../View/BCSingleDisplayWidget.h \
    ../View/BCGroupDisplayWidget.h \
    ../View/BCSingleDisplayVirtualWidget.h \
    ../Setting/BCSettingAutoReadInputChannelConfigDlg.h \
    ../Setting/BCSettingDebugDlg.h \
    ../View/BCControlTreeWidget.h \
    ../View/BCSceneMatrixTreeWidget.h \
    ../Setting/BCSettingModifyNameDlg.h \
    ../Setting/BCSettingMatrixFormatDlg.h \
    ../Setting/BCSettingDisplayMakerConfigDlg.h \
    ../Model/BCMMatrix.h \
    ../Setting/BCSettingJointMatrixChannelConfigDlg.h \
    ../Setting/BCSettingMatrixCutInputChannelDlg.h \
    ../View/BCSignalSrouceSceneViewWidget.h \
    ../Common/BCOutsideInterfaceServer.h \
    ../Setting/BCSettingOutsideInterfaceDlg.h \
    ../Setting/BCSettingRepeatApplicationWarningDlg.h \
    ../View/BCLocationGroupWidget.h

FORMS    += \
    ../Setting/AdminPasswordDlg.ui \
    ../Setting/ColorSettingDlg.ui \
    ../Setting/DeviceConnectDlg.ui \
    ../Setting/DeviceFormatDlg.ui \
    ../Setting/LightSettingDlg.ui \
    ../View/BCScreenName.ui \
    ../View/BCScreenDlg.ui \
    ../Setting/BCSettingMainPanelStyle.ui \
    ../Setting/BCSettingPasswordStyle.ui \
    ../View/BCFaceWidget.ui \
    ../View/BCSignal.ui \
    ../View/BCSystemplan.ui \
    ../View/BCScene.ui \
    ../View/BCWidgetBtn.ui \
    ../View/BCControl.ui \
    ../View/BCSignalListWidgetData.ui \
    ../View/BCAutoDateDlg.ui \
    ../View/BCSignalName.ui \
    ../Setting/BCLoginDlg.ui \
    ../Setting/BCSettingDisplayInfoDlg.ui \
    ../Setting/BCSettingDisplaySetNetSetDlg.ui \
    ../View/BCLocationDlg.ui \
    ../Setting/BCSettingOutOfDateDlg.ui \
    ../Setting/BCSettingSignalWindowPropertyDlg.ui \
    ../Setting/BCSettingWaitingDlg.ui \
    ../Setting/BCSettingBoardCardDlg.ui \
    ../Setting/BCSettingOutSideCommandDlg.ui \
    ../Setting/BCSettingAddIPVedioTreeWidgetItemDlg.ui \
    ../Setting/BCSettingDeviceCustomResolutionDlg.ui \
    ../Setting/BCSettingDisplaySwitchConfigDlg.ui \
    ../Setting/BCSettingDisplaySwitchRoomWidget.ui \
    ../View/BCMatrixRoomWidget.ui \
    ../View/BCMatrixInputNodeWidget.ui \
    ../View/BCMatrixOutputNodeWidget.ui \
    ../View/BCSignalWindowDisplayWidget.ui \
    ../View/BCSingleDisplayWidget.ui \
    ../Setting/BCSettingAutoReadInputChannelConfigDlg.ui \
    ../Setting/BCSettingDebugDlg.ui \
    ../Setting/BCSettingModifyNameDlg.ui \
    ../Setting/BCSettingMatrixFormatDlg.ui \
    ../Setting/BCSettingDisplayMakerConfigDlg.ui \
    ../Setting/BCSettingJointMatrixChannelConfigDlg.ui \
    ../Setting/BCSettingMatrixCutInputChannelDlg.ui \
    ../Setting/BCSettingOutsideInterfaceDlg.ui \
    ../Setting/BCSettingRepeatApplicationWarningDlg.ui

MOC_DIR = ../temp/moc
RCC_DIR = ../temp/rcc
UI_DIR = ../temp/ui
OBJECTS_DIR = ../temp/obj

DESTDIR = ../bin

RESOURCES += \
    ../resource/image.qrc

#exe icon
win32:RC_FILE = icon.rc

#ribbon style
#C:\Program Files (x86)\Developer Machines\QtitanRibbon
include($$(QTITANDIR)/src/shared/qtitanribbon.pri)
