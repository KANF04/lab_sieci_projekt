#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include "client.h"
#include <QSocketNotifier>
#include "grid.h"
#include <vector>
#include <string>
#include <iostream>

using namespace std;

namespace Ui {
class GameWindow;
}

class GameWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GameWindow(int fd, bool isDead, QWidget *parent = nullptr);
    ~GameWindow();

    void setMatrix(const vector<vector<string>>& newMatrix);
    void setColor(QString& color);
    void deadMessage();
    void setIsDead(bool isDead);

private:
    Ui::GameWindow *ui;
    int fd;
    void onQuitButtonClicked();
    QSocketNotifier* notifier;
    Grid *grid;
    void onRestartButtonClicked();
    bool isD;

protected:
    void keyPressEvent(QKeyEvent *event) override {
        const char* key = nullptr;
        switch (event->key()) {
            case Qt::Key_A:
                key = "a\n";
                sendMove(fd, key);
                break;
            case Qt::Key_D:
                key = "d\n";
                sendMove(fd, key);
                break;
            default:
                break;
        }


        QMainWindow::keyPressEvent(event);
    };
};

#endif // GAMEWINDOW_H
