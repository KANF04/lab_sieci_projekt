#ifndef GRID_H
#define GRID_H

#include <QWidget>
#include <QPainter>
#include <vector>
#include <string>

using namespace std;

class Grid : public QWidget
{
    Q_OBJECT
public:
    explicit Grid(QWidget *parent = nullptr);

    void setMatrix(const vector<vector<string>>& newMatrix);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    vector<vector<string>> matrix;
};

#endif // GRID_H
