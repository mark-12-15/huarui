#include "BCLoginDlg.h"
#include "ui_BCLoginDlg.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <QNetworkInterface>

#include "../Common/BCCommon.h"
#include "../Common/BCLocalServer.h"

BCLoginDlg::BCLoginDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BCLoginDlg)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    m_bPress = false;
    m_pt = QPoint(0, 0);

    m_bServerReturn = false;

    this->setWindowIcon(QIcon(BCCommon::g_qsImageFilePrefix + BCCommon::g_qsApplicationIcon));

    // 设置用户名可编辑，密码隐藏
    ui->m_pUserCombox->setEditable(true);
    ui->m_pPasswordLineEdit->setEchoMode(QLineEdit::Password);

    // 用户名、密码、关闭按钮图片
    ui->m_pUserLabel->setPixmap(QPixmap(BCCommon::g_qsApplicationDefaultStylePath+"/loginuser24.png"));
    ui->m_pPasswordLabel->setPixmap(QPixmap(BCCommon::g_qsApplicationDefaultStylePath+"/loginpwd24.png"));
    ui->m_pCloseBtn->setIcon(QIcon(BCCommon::g_qsApplicationDefaultStylePath+"/loginclose24.png"));

    QPalette palette;
    palette.setBrush(QPalette::Background, QBrush(QPixmap(BCCommon::g_qsApplicationDefaultStylePath+"/loginbackground.png")));
    ui->m_pBackGroudPixmapWidget->setPalette(palette);

    // 初始化用户控件内容
    initUsersFromXml();
}

BCLoginDlg::~BCLoginDlg()
{
    delete ui;
}

void BCLoginDlg::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton) {
        m_bPress = true;
        m_pt = e->pos();
    }
}

void BCLoginDlg::mouseMoveEvent(QMouseEvent *e)
{
    if ( m_bPress ) {
        this->move(e->globalPos().x()-m_pt.x(),
                   e->globalPos().y()-m_pt.y());
    }
}

void BCLoginDlg::mouseReleaseEvent(QMouseEvent *)
{
    m_bPress = false;
}

void BCLoginDlg::initUsersFromXml()
{
    QFile file( QString("../xml/BCLoginRecord.xml") );
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
    auto savePwd = 1 == docElem.attribute("savePwd").toInt() ? true : false;
    ui->chbSavePwd->setChecked(savePwd);
    _password = docElem.attribute("password");

    // 循环添加用户
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeUser = docElem.childNodes().at(i);
        if ( !nodeUser.isElement() )
            continue;

        QDomElement eleUser = nodeUser.toElement();
        QString qsUser = eleUser.attribute(QString("usr"));
        QString pwd = eleUser.attribute(QString("pwd"));

        ui->m_pUserCombox->addItem( qsUser );

        if (savePwd)
        {
            ui->m_pPasswordLineEdit->setText(pwd);
        }
    }

}

void BCLoginDlg::updateUsersOfXml()
{
    QFile file( QString("../xml/BCLoginRecord.xml") );
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

    docElem.setAttribute("savePwd", ui->chbSavePwd->isChecked() ? 1 : 0);

    QString qsUser = ui->m_pUserCombox->currentText();

    // 删除已有用户
    for (int i = 0; i < docElem.childNodes().count(); i++) {
        QDomNode nodeUser = docElem.childNodes().at(i);
        if ( !nodeUser.isElement() )
            continue;

        QDomElement eleUser = nodeUser.toElement();
        QString usr = eleUser.attribute(QString("usr"));
        if (usr != qsUser)
            continue;

        // 找到则移动到第一个位置
        docElem.removeChild( nodeUser );

        break;
    }

    // 将用户插入到第一个位置
    QDomElement eleUser = doc.createElement(QString("User"));
    eleUser.setAttribute(QString("usr"), qsUser);
    eleUser.setAttribute(QString("pwd"), QString(""));

    docElem.insertBefore(eleUser, docElem.firstChild());

    // 写入文件
    if( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return;
    QTextStream out(&file);
    doc.save(out,4);
    file.close();
}

void BCLoginDlg::on_m_pCloseBtn_clicked()
{
    this->close();
}

void BCLoginDlg::on_m_pLoginBtn_clicked()
{
    QString qsUser = ui->m_pUserCombox->currentText();
    QString qsPwd = ui->m_pPasswordLineEdit->text();
    if ( qsUser.isEmpty() ) {
        QMessageBox::warning(this,
                             tr("警告"),
                             tr("用户名不可以为空!"),
                             QMessageBox::Ok);
        return;
    }

    if (qsUser == "admin" && qsPwd == _password)
        SetServerRes(1);
    else
        SetServerRes(-1);
}

void BCLoginDlg::SetServerRes(int userid)
{
    m_bServerReturn = true;

    if (-1 == userid) {
        QString qsWarning = tr("用户名或密码错误!");
        QMessageBox::warning(this,
                             tr("警告"),
                             qsWarning,
                             QMessageBox::Ok);
    } else {
        MainWindow *pApplication = BCCommon::Application();

        BCSUser *pUser = new BCSUser;
        pUser->id = userid;
        pUser->loginName = ui->m_pUserCombox->currentText();
        pUser->password = ui->m_pPasswordLineEdit->text();
        pUser->level = 1;
        pApplication->SetCurrentUser( pUser );

        // 登录成功修改xml文件，不存储root用户
        updateUsersOfXml();

        accept();
    }
}

void BCLoginDlg::on_btnQuit_clicked()
{
    on_m_pCloseBtn_clicked();
}
