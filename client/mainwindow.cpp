#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QListWidgetItem>
#include <QPushButton>
#include <iostream>
#include "client.h"
#include "gamewindow.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->serverMsgLabel->setStyleSheet("color: red;");
    ui->serverMsgLabel->hide();
    connect(ui->joinButton, &QPushButton::clicked, this, &MainWindow::onJoinButtonClicked);
}

void MainWindow::onJoinButtonClicked() {

    string ipStr = ui->ipLineEdit->text().toStdString();
    const char* ip = ipStr.c_str();

    int port = ui->portLineEdit->text().toInt();

    if (port <= 0 || port >= 62535) {
        ui->serverMsgLabel->setText("Connection error: Wrong port");
        ui->serverMsgLabel->show();
    }
    else {

        if (connectToServer(ip, port) == true) {

            this->hide();

            GameWindow *win = new GameWindow();
            win->show();
        }
        else {
            ui->serverMsgLabel->setText("Connection error: Wrong IP or port");
            ui->serverMsgLabel->show();
        }
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}
