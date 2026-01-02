#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(int fd, QString info, QWidget *parent = nullptr);
    ~Dialog();

private:
    void onYesVoteButtonClicked();
    void onNoVoteButtonClicked();
    int fd;
    QString info;
    Ui::Dialog *ui;
};

#endif // DIALOG_H
