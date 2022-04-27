#include "label_for_paint.h"

//определяет ближайшую точку, так-же возвращает расстояние до неё
QPoint *label_for_paint::getNearestPoint(int mouse_x, int mouse_y, double* distance)
{
    double min_dist = -1;
    QPoint* result = nullptr;
    for (auto& point : points)
    {
        int dx = mouse_x - point.x();
        int dy = mouse_y - point.y();
        double dist = dx * dx + dy * dy;
        if ((dist < min_dist) || (min_dist == -1))
        {
            min_dist = dist;
            result = &point;
        }
    }
    if (distance) *distance = min_dist;
    return result;
}

//конструктор
label_for_paint::label_for_paint(QWidget *parent, const Qt::WindowFlags &f) : QLabel(parent, f)
{}

//точечки
void label_for_paint::paintDots(QPainter& painter)
{
    for (auto& p : points)
    {
        if (&p == chosen) painter.setPen(QPen(Qt::red, 10));
        else if (&p == near_cursor) painter.setPen(QPen(Qt::green, 10));
        else painter.setPen(QPen(Qt::black, 10));
        painter.drawPoint(p);
    }
}

//линии
void label_for_paint::paintLines(QPainter &painter)
{
    QPoint* first = nullptr;
    QPoint* last = nullptr;
    painter.setPen(QPen(Qt::black, 1));
    for (auto& p : points)
    {
        if (!first) first = &p;
        if (last)
        {
            painter.drawLine(p, *last);
        }
        last = &p;
    }
    if (closed && first && last) painter.drawLine(*first, *last);
}

//рисовалка
void label_for_paint::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    paintDots(painter);
    paintLines(painter);
}

void label_for_paint::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        //курсор в свободном месте - добавить новую
        if (!near_cursor)
        {
            points.push_back(QPoint(ev->x(), ev->y()));
            chosen = &*points.rbegin();
            return;
        }
        //курсор наведен в район существующей точки - захватить
        dragged = near_cursor;
        chosen = near_cursor;
    }
    if (ev->button() == Qt::RightButton)
    {
        if (near_cursor && !dragged)
        {
            points.remove(*near_cursor);
            near_cursor = nullptr;
        }
        chosen = nullptr;
    }
    repaint();
}

void label_for_paint::mouseMoveEvent(QMouseEvent *ev)
{
    auto mouse_x = ev->x();
    auto mouse_y = ev->y();
    double dist;
    bool need_update = false;
    QPoint* nearest = getNearestPoint(mouse_x, mouse_y, &dist);
    if (dist < 100)//есть точка в районе 10 клеток от мышки - подсветить
    {
        if (nearest != near_cursor) need_update = true;
        near_cursor = nearest;
    }
    else//нет точки в районе 10 клеток от мышки - потушить
    {
        if (near_cursor) need_update = true;
        near_cursor = nullptr;
    }
    //есть захваченная точка - двигаем
    if (dragged)
    {
        dragged->setX(ev->x());
        dragged->setY(ev->y());
        need_update = true;
    }
    if (need_update) repaint();
}

void label_for_paint::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        dragged = nullptr;
        repaint();
    }
}
