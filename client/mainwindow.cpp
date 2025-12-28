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

    connect(ui->joinButton, &QPushButton::clicked, this, &MainWindow::onJoinButtonClicked);
}

void MainWindow::onJoinButtonClicked() {

    string ipStr = ui->ipLineEdit->text().toStdString();
    const char* ip = ipStr.c_str();

    int port = ui->portLineEdit->text().toInt();

    if (connectToServer(ip, port) == true) {

        this->hide();

        GameWindow *win = new GameWindow();
        win->show();
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}
