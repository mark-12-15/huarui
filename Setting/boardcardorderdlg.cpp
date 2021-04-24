#include "boardcardorderdlg.h"
#include "ui_boardcardorderdlg.h"
#include "BCCommon.h"
#include "BCMRoom.h"
#include "BCMGroupDisplay.h"

boardCardOrderDlg::boardCardOrderDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::boardCardOrderDlg)
{
    ui->setupUi(this);

    auto room = BCCommon::Application()->GetMRoom(0);
    auto group = room->GetGroupDisplay(0);
    auto size = group->GetArraySize();
}

boardCardOrderDlg::~boardCardOrderDlg()
{
    delete ui;
}

void boardCardOrderDlg::on_pushButton_clicked()
{

}

void boardCardOrderDlg::on_pushButton_2_clicked()
{
    this->close();
}
