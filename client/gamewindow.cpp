#include "gamewindow.h"
#include "ui_gamewindow.h"
#include "grid.h"
#include "client.h"
#include "mainwindow.h"
#include <QSocketNotifier>
#include <string>
#include <iostream>
#include <vector>
#include <QMessageBox>
#include "dialog.h"

using namespace std;

GameWindow::GameWindow(int fd, bool isDead, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GameWindow)
    , fd(fd)
    , isD(isDead)
{
    ui->setupUi(this);

    ui->restartButton->setEnabled(false);

    // ciągły focus na oknie
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

    // połączenie przycisku do wyjścia z gry
    connect(ui->quitButton, &QPushButton::clicked, this, &GameWindow::onQuitButtonClicked);
    extern void printRecvMsg(int fd);

    notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);

    connect(notifier, &QSocketNotifier::activated, this, [this]() {
        extern void printRecvMsg(int fd, GameWindow *window);
        printRecvMsg(this->fd, this);
    });

    // połączenie przycisku do ponownego dołączania
    connect(ui->restartButton, &QPushButton::clicked, this, &GameWindow::onRestartButtonClicked);

    extern void printRecvMsg(int fd);

    // ustawienie rozmiaru okna
    this->setFixedSize(1600, 1200);
    grid = new Grid(this);
    grid->setGeometry(100, 100, 800, 800);
    grid->show();
}

/*----------------------------------------------------METODY PRZYCISKÓW----------------------------------------------------*/

void GameWindow::onQuitButtonClicked() {
    notifier->setEnabled(false);
    shut_conn(fd);
    close();

    MainWindow *win = new MainWindow();
    win->show();
}

void GameWindow::onRestartButtonClicked() {
    if (isD) {
        sendMove(this->fd, "r");
        ui->restartButton->setEnabled(false);
    }
}

/*----------------------------------------------------POZOSTAŁE METODY----------------------------------------------------*/

void GameWindow::setMatrix(const vector<vector<string>>& newMatrix)
{
    grid->setMatrix(newMatrix);
}

void GameWindow::setIsDead(bool isDead) {
    isD = isDead;
    ui->restartButton->setEnabled(isDead);
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
    else if (color == "żółty") {
        ui->playerColorLabel->setStyleSheet("color: yellow;");
    }
    else {
        ui->playerColorLabel->setStyleSheet("color: black;");
    }
}

void GameWindow::deadMessage() {
    QMessageBox::information(this, "", "Zostałeś pokonany :(");
}

GameWindow::~GameWindow()
{
    delete ui;
}
