#include "gamewindow.h"
#include "ui_gamewindow.h"
#include "grid.h"
#include "client.h"

GameWindow::GameWindow(int fd, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GameWindow)
    , fd(fd)
{
    ui->setupUi(this);
    connect(ui->quitButton, &QPushButton::clicked, this, &GameWindow::onQuitButtonClicked);
    this->setFixedSize(1600, 1200);

    Grid *grid = new Grid(this);
    grid->setGeometry(100, 100, 800, 800);
    grid->show();
}

void GameWindow::onQuitButtonClicked() {
    shut_conn(fd);
}

GameWindow::~GameWindow()
{
    delete ui;
}
