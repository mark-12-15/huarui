#ifndef BOARDCARDORDERDLG_H
#define BOARDCARDORDERDLG_H

#include <QDialog>

namespace Ui {
class boardCardOrderDlg;
}

class boardCardOrderDlg : public QDialog
{
    Q_OBJECT

public:
    explicit boardCardOrderDlg(QWidget *parent = nullptr);
    ~boardCardOrderDlg();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::boardCardOrderDlg *ui;
};

#endif // BOARDCARDORDERDLG_H
