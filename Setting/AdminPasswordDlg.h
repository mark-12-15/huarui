#ifndef ADMINPASSWORDDLG_H
#define ADMINPASSWORDDLG_H

#include <QDialog>

namespace Ui {
class AdminPasswordDlg;
}

class AdminPasswordDlg : public QDialog
{
    Q_OBJECT

public:
    explicit AdminPasswordDlg(QWidget *parent = nullptr);
    ~AdminPasswordDlg();

private slots:
    void on_btnCancel_clicked();

    void on_btnOk_clicked();

    void on_btnModify_clicked();

private:
    Ui::AdminPasswordDlg *ui;
};

#endif // ADMINPASSWORDDLG_H
