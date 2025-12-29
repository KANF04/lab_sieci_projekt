#ifndef GRID_H
#define GRID_H

#include <QWidget>
#include <QPainter>

class Grid : public QWidget
{
    Q_OBJECT
public:
    explicit Grid(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override{
        QPainter painter(this);

        painter.setPen(Qt::black);

        int rowCol = 20;

        int w = width(); // returns widget width
        int h = height(); // returns widghet height

        float dx = w / float(rowCol);
        float dy = h / float(rowCol);

        for (int i=0; i <= rowCol; i++) {
            painter.drawLine(i*dx, 0, i*dx, h); // prints line from x1, y1 to x2, y2 - vertical lines
        }

        for (int i=0; i<= rowCol; i++) {
            painter.drawLine(0, i*dy, w, i*dy); // horizontal line
        }

    }
signals:
};

#endif // GRID_H
