/*********************************************************************************************************************************
* 作    者：liuwl
* 摘    要：用户登录对话框
*********************************************************************************************************************************/
#ifndef BCLoginDlg_H
#define BCLoginDlg_H

#include <QDialog>

namespace Ui {
class BCLoginDlg;
}

class BCLoginDlg : public QDialog
{
    Q_OBJECT

public:
    explicit BCLoginDlg(QWidget *parent = 0);
    ~BCLoginDlg();

    // 返回服务器验证结果
    void SetServerRes(int userid);

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);

    // 从xml中读取已经登录过的用户
    void initUsersFromXml();

    // 更新xml中的用户信息
    void updateUsersOfXml();

private slots:
    void on_m_pLoginBtn_clicked();

    void on_m_pCloseBtn_clicked();

    void on_btnQuit_clicked();

private:
    Ui::BCLoginDlg *ui;

    bool    m_bPress;
    QPoint  m_pt;

    bool    m_bServerReturn;    // 服务器是否返回指令

    QString _password;
};

#endif // BCLoginDlg_H
