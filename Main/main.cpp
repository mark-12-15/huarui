/*********************************************************************************************************************************
* 作    者：liuwl
* 摘    要：主函数，需要与服务器通讯，并完成系统初始化工作
*********************************************************************************************************************************/
#include "MainWindow.h"
#include <QApplication>
#include <QMessageBox>

#include "../Common/BCCommon.h"
#include "../Common/BCLocalServer.h"
#include "../Setting/BCLoginDlg.h"
#include "../Setting/BCSettingRepeatApplicationWarningDlg.h"

using namespace std;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(new RibbonStyle);

    // 设置编码
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    // 创建共享内存，保证只启动一次应用程序
    QSharedMemory shared_memory;
    shared_memory.setKey(QString("markView"));
    if(shared_memory.attach()) {
        BCSettingRepeatApplicationWarningDlg dlg;
        dlg.exec();

        return 0;
    }

    shared_memory.create(1);

    // 登录界面
    BCLoginDlg loginDlg;
    // 登录不成功则弹窗提示并且返回
    if(loginDlg.exec() != QDialog::Accepted) {
        // 删除本地服务器
        BCLocalServer::Destroy();
        return 0;
    }

    // 公共接口调用应用程序
    MainWindow *pApplication = BCCommon::Application();
    pApplication->LoadGenaralConfig();
    pApplication->RefreshMainWindow();

    // TODO timer
    auto date = QDate::currentDate();
    auto endDate = QDate(2021, 6, 1);
    if (date > endDate)
    {
        QTimer::singleShot(5000, [] {
            qApp->exit();
        });
    }

    return a.exec();
}
