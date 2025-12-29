#include "gamewindow.h"
#include "ui_gamewindow.h"
#include "grid.h"

GameWindow::GameWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GameWindow)
{
    ui->setupUi(this);
    this->setFixedSize(1600, 1200);

    Grid *grid = new Grid(this);
    grid->setGeometry(100, 100, 800, 800);
    grid->show();
}

GameWindow::~GameWindow()
{
    delete ui;
}
