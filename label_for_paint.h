#ifndef LABEL_FOR_PAINT_H
#define LABEL_FOR_PAINT_H

#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <list>

class label_for_paint : public QLabel
{

    std::list <QPoint> points;
    QPoint* getNearestPoint(int mouse_x, int mouse_y, double* distance);
    QPoint* dragged = nullptr;
    QPoint* near_cursor = nullptr;
    QPoint* chosen = nullptr;
public:
    bool closed = false;
    bool circled = false;
    label_for_paint(QWidget *parent = nullptr, const Qt::WindowFlags &f = Qt::WindowFlags());
    void paintDots(QPainter &painter);
    void paintLines(QPainter &painter);
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
};

#endif // LABEL_FOR_PAINT_H
