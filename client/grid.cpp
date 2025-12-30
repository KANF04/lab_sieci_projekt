#include "grid.h"
#include <vector>
#include <string>
#include <QPainter>
#include <QPaintEvent>
#include <QColor>

using namespace std;

QColor lightRed("#872b40"); // currently colored cubes
QColor darkRed("#5c202e"); // head of the read player

Grid::Grid(QWidget *parent)
    : QWidget{parent}
{}


void Grid::setMatrix(const vector<vector<string>>& newMatrix)
{
    matrix = newMatrix;
    update();
}

void Grid::paintEvent(QPaintEvent *) {
    if (matrix.empty() || matrix[0].empty())
        return;

    QPainter painter(this);
    painter.setPen(Qt::black);

    int rows = matrix.size();
    int cols = matrix[0].size();

    int w = width();
    int h = height();

    float dx = float(w) / cols;
    float dy = float(h) / rows;

    // rysowanie siatki
    for (int i = 0; i <= cols; i++)
        painter.drawLine(i*dx, 0, i*dx, h);
    for (int i = 0; i <= rows; i++)
        painter.drawLine(0, i*dy, w, i*dy);

    // rysowanie pól
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (matrix[r][c] == "R") painter.fillRect(c*dx, r*dy, dx, dy, Qt::red);
            if (matrix[r][c] == "r")
                painter.fillRect(c*dx, r*dy, dx, dy, lightRed);
            else if (matrix[r][c] == "1")
                painter.fillRect(c*dx, r*dy, dx, dy, darkRed);
        }
    }
}
