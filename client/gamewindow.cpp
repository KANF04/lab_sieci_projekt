#include "gamewindow.h"
#include "ui_gamewindow.h"
#include "grid.h"
#include "client.h"
#include "mainwindow.h"
#include <QSocketNotifier>
#include <string>
#include <iostream>
#include <vector>

using namespace std;

GameWindow::GameWindow(int fd, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GameWindow)
    , fd(fd)
{
    ui->setupUi(this);

    this->setFocusPolicy(Qt::StrongFocus); // focus on window
    this->setFocus();

    connect(ui->quitButton, &QPushButton::clicked, this, &GameWindow::onQuitButtonClicked);
    extern void printRecvMsg(int fd);

    notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this]() {
        extern void printRecvMsg(int fd, GameWindow *window);
        printRecvMsg(this->fd, this);
    });

    this->setFixedSize(1600, 1200);
    grid = new Grid(this);
    grid->setGeometry(100, 100, 800, 800);
    grid->show();
}

void GameWindow::onQuitButtonClicked() {
    notifier->setEnabled(false);
    shut_conn(fd);
    close();

    MainWindow *win = new MainWindow();
    win->show();
}

void GameWindow::setMatrix(const vector<vector<string>>& newMatrix)
{
    grid->setMatrix(newMatrix);
}

void GameWindow::setColor(QString& color) {
    ui->playerColorLabel->setText("Twój kolor: " + color);
    if (color == "czerwony") {
        ui->playerColorLabel->setStyleSheet("color: red;");
    }
    else if (color == "niebieski") {
        ui->playerColorLabel->setStyleSheet("color: blue;");
    }
    else if (color == "zielony") {
        ui->playerColorLabel->setStyleSheet("color: green;");
    }
    else {
        ui->playerColorLabel->setStyleSheet("color: yellow;");
    }
}

GameWindow::~GameWindow()
{
    delete ui;
}
