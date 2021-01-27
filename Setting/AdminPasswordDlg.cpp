#include "AdminPasswordDlg.h"
#include "ui_AdminPasswordDlg.h"
#include "BCSettingPasswordStyle.h"
#include "BCCommon.h"
#include "BCRibbonMainToolBar.h"
#include <QMessageBox>

AdminPasswordDlg::AdminPasswordDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AdminPasswordDlg)
{
    ui->setupUi(this);
}

AdminPasswordDlg::~AdminPasswordDlg()
{
    delete ui;
}

void AdminPasswordDlg::on_btnCancel_clicked()
{
    close();
}

void AdminPasswordDlg::on_btnOk_clicked()
{
    BCSUser *pUser = BCCommon::Application()->GetCurrentUser();
    if (ui->lePwd->text() != pUser->password) {
        QMessageBox::warning(this,
                             tr("警告"),
                             tr("密码不对，请重新输入!"),
                             QMessageBox::Ok);
        return;
    }

    // show device
    BCCommon::Application()->GetRibbonMainToolBar()->addDevice();

    close();
}

void AdminPasswordDlg::on_btnModify_clicked()
{
    auto dlg = new BCSettingPasswordStyle(this);
    dlg->exec();
}
