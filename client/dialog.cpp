#include "dialog.h"
#include "ui_dialog.h"
#include "gamewindow.h"
#include "client.h"
#include <QPushButton>
#include <QLabel>
#include <QString>

Dialog::Dialog(int fd, QString info, QString statistic, QWidget *parent)
    : QDialog(parent)
    , fd(fd)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    connect(ui->voteYesButton, &QPushButton::clicked, this, &Dialog::onYesVoteButtonClicked);
    connect(ui->voteNoButton, &QPushButton::clicked, this, &Dialog::onNoVoteButtonClicked);

    setWindowTitle("Koniec gry");

    ui->winLoseLabel->setText(info);
    ui->statisticsLabel->setText("Udało Ci się zająć " + statistic + "% " + "mapy.");
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::onYesVoteButtonClicked() {
    sendMove(this->fd, "y");
    close();
}

void Dialog::onNoVoteButtonClicked() {
    close();
}
