#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include "client.h"
#include <QSocketNotifier>

namespace Ui {
class GameWindow;
}

class GameWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GameWindow(int fd, QWidget *parent = nullptr);
    ~GameWindow();

private:
    Ui::GameWindow *ui;
    int fd;
    void onQuitButtonClicked();
    QSocketNotifier* notifier;

protected:
    void keyPressEvent(QKeyEvent *event) override {
        const char* key = nullptr;

        if (event->key() == Qt::Key_A) {
            key = "A\n";
            sendMove(fd, key);
        }
        else if (event->key() == Qt::Key_D) {
            key = "D\n";
            sendMove(fd, key);
        }

        QMainWindow::keyPressEvent(event);
    };
};

#endif // GAMEWINDOW_H
