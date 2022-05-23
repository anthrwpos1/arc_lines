#ifndef LABEL_FOR_PAINT_H
#define LABEL_FOR_PAINT_H

#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <list>
#include <cmath>
#include <complex>

struct selectable{
    selectable() {}
    virtual double distance(int x, int y) = 0;
    virtual ~selectable() = default;
};

struct path_point : public QPoint, public selectable
{
    path_point(int x, int y) : QPoint(x, y), selectable() {}
    double distance(int x, int y) override;
    double _angle = M_PI / 4;
};

class label_for_paint : public QLabel
{
    Q_OBJECT;

    struct path_line : public selectable
    {
        path_point* p1 = nullptr;
        path_point* p2 = nullptr;
        path_line(path_point* p1, path_point* p2) : p1(p1), p2(p2) {}
        double distance(int x, int y) override;
    };
    std::list <path_point> points;
    path_point* dragged = nullptr;
    selectable* near_cursor = nullptr;
    path_point* chosen = nullptr;
    std::_List_iterator<path_point> nearest_line = points.end();

    path_point* getNearestPoint(int mouse_x, int mouse_y, double* distance);
    std::_List_iterator<path_point> getNearestLine(int mouse_x, int mouse_y, double* distance);
    void paintDots(QPainter &painter);
    void paintLines(QPainter &painter);
    void paintArc(QPainter &painter, const path_point& from, const path_point& to, const double angle, const int pnum = 36);
    void paintArcs(QPainter &painter);
    void calcAngles();
public:
    bool closed = false;
    bool circled = false;
    label_for_paint(QWidget *parent = nullptr, const Qt::WindowFlags &f = Qt::WindowFlags());

Q_SIGNALS:
    void show_dist_to_line(double arg);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
};

#endif // LABEL_FOR_PAINT_H
