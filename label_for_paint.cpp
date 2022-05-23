#include "label_for_paint.h"

std::complex<double> normalize(const std::complex<double>& in)
{
    double re = in.real();
    double im = in.imag();
    double norm = 1 / sqrt(re * re + im * im);
    return {re * norm, im * norm};
}

//определяет ближайшую точку, так-же возвращает расстояние до неё
path_point *label_for_paint::getNearestPoint(int mouse_x, int mouse_y, double* distance)
{
    double min_dist = -1;
    path_point* result = nullptr;
    for (auto& point : points)
    {
        //int dx = mouse_x - point.x();
        //int dy = mouse_y - point.y();
        double dist = point.distance(mouse_x, mouse_y);//dx * dx + dy * dy;
        if ((dist < min_dist) || (min_dist == -1))
        {
            min_dist = dist;
            result = &point;
        }
    }
    if (distance) *distance = min_dist;
    return result;
}

std::_List_iterator<path_point> label_for_paint::getNearestLine(int mouse_x, int mouse_y, double *distance)
{
    double min_dist = -1;
    std::_List_iterator<path_point> result;
    auto it_2 = points.begin();
    if (it_2 == points.end()) return it_2;
    auto it_1 = points.end();
    --it_1;
    if (it_1 == it_2) return points.end();
    while(it_2 != points.end())
    {
        path_point pp1 = *it_1;
        path_point pp2 = *it_2;
        path_line pl{&pp1, &pp2};
        double dist = pl.distance(mouse_x, mouse_y);
        if ((dist < min_dist) || (min_dist == -1))
        {
            min_dist = dist;
            result = it_2;
        }
        it_1 = it_2;
        ++it_2;
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
    for (auto p_it = points.begin(); p_it != points.end(); ++p_it)
    {
        path_point *p = &*p_it;
        QBrush brush;
        if (p == chosen) brush = {Qt::red};
        else if (p == near_cursor) brush = {Qt::green};
        else brush = {Qt::gray};
        painter.setBrush(brush);
        painter.drawEllipse(*p, 10, 10);
    }

}

//линии
void label_for_paint::paintLines(QPainter &painter)
{
    QPoint* first = nullptr;
    QPoint* last = nullptr;
    for (auto& p : points)
    {
        if (!first) first = &p;
        if (last)
        {
            if (&p == &*nearest_line) painter.setPen(QPen(Qt::green, 1));
            else painter.setPen(QPen(Qt::black, 1));
            painter.drawLine(p, *last);
        }
        last = &p;
    }
    if (closed && first && last)
    {
        if (first == &*nearest_line) painter.setPen(QPen(Qt::green, 1));
        else painter.setPen(QPen(Qt::black, 1));
        painter.drawLine(*first, *last);
    }
}

void label_for_paint::paintArc(QPainter &painter, const path_point &from, const path_point &to, const double angle, const int pnum)
{
    std::complex<double> vec_from_to = {(double) (to.x() - from.x()),(double) (to.y() - from.y())};
    std::complex<double> single_rotation = std::exp(std::complex<double>{0, 2*angle/pnum});
    std::complex<double> complete_rotation = std::exp(std::complex<double>{0, 2*angle});
    constexpr std::complex<double> I {1, 0};
    std::complex<double> step_vector = vec_from_to * (I - single_rotation) / (I - complete_rotation);
    std::complex<double> p1{(double) from.x(), (double) from.y()};
    for (int i = 0; i < pnum; ++i)
    {
        auto p2 = p1 + step_vector;
        step_vector = step_vector * single_rotation;
        painter.drawLine(p1.real(), p1.imag(), p2.real(), p2.imag());
        p1 = p2;
    }
}

void label_for_paint::paintArcs(QPainter &painter)
{
    path_point* last = nullptr;
    for (auto& p : points)
    {
        if (last)
        {
            paintArc(painter, p, *last, last->_angle);
        }
        last = &p;
    }
    if (closed && last) paintArc(painter, *last, points.front(), last->_angle);
}

void label_for_paint::calcAngles()
{
    path_point* prev_point = nullptr;
    if (closed)
    {
        auto last_p_it = points.rbegin();
        if (last_p_it != points.rend())
        {
            prev_point = &*last_p_it;
        }
    }
    path_point* preprev_point = nullptr;
    std::complex<double> next_vector;
    std::complex<double> prev_vector;
    double next_vector_length;
    std::complex<double> prev_unit_vector;
    std::complex<double> next_unit_vector;
    for (auto& p : points)
    {
        if (prev_point)
        {
            prev_vector = next_vector;
            next_vector = {(double) (p.x() - prev_point->x()), (double) (p.y() - prev_point->y())};
            next_vector_length = std::abs(next_vector);
            prev_unit_vector = next_unit_vector;
            next_unit_vector = next_vector / next_vector_length;
        }
        if (preprev_point)
        {
            std::complex<double> rotation = prev_unit_vector / next_unit_vector;
            //double rotation_magnutude = std::arg(rotation);
            std::complex<double> old_angle = std::exp(std::complex<double>{0,preprev_point->_angle});
            std::complex<double> next_angle = rotation / old_angle;
            prev_point->_angle = std::arg(next_angle);
        }
        preprev_point = prev_point;
        prev_point = &p;
    }
}

//рисовалка
void label_for_paint::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    paintDots(painter);
    if (circled) paintArcs(painter);
    else paintLines(painter);
}

void label_for_paint::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        //курсор в свободном месте - добавить новую
        if (!near_cursor)
        {
            auto new_point_iterator = points.insert(nearest_line, path_point(ev->x(), ev->y()));
            chosen = &*new_point_iterator;
            near_cursor = chosen;
        }
        //курсор наведен в район существующей точки - захватить
        path_point* selected_point = dynamic_cast<path_point*>(near_cursor);
        if (selected_point)
        {
            dragged = selected_point;
            chosen = selected_point;
        }
    }
    if (ev->button() == Qt::RightButton)
    {
        if (near_cursor && !dragged)
        {
            path_point* selected_point = dynamic_cast<path_point*>(near_cursor);
            if (selected_point) points.remove(*selected_point);
            near_cursor = nullptr;
        }
        chosen = nullptr;
    }
    calcAngles();
    repaint();
}

void label_for_paint::mouseMoveEvent(QMouseEvent *ev)
{
    auto mouse_x = ev->x();
    auto mouse_y = ev->y();
    double dist;
    bool need_update = false;
    path_point* nearest = getNearestPoint(mouse_x, mouse_y, &dist);
    double ldist;
    auto was_nearest_line = nearest_line;
    nearest_line = getNearestLine(mouse_x, mouse_y, &ldist);
    if (was_nearest_line != nearest_line) need_update = true;
    if (dist < 10)//есть точка в районе 10 клеток от мышки - подсветить
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
    if (need_update)
    {
        calcAngles();
        repaint();
    }
}

void label_for_paint::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        dragged = nullptr;
        repaint();
    }
}

double path_point::distance(int m_x, int m_y)
{
    double dx = m_x - x();
    double dy = m_y - y();
    return sqrt(dx*dx+dy*dy);
}

double dot_complex(std::complex<double> left, std::complex<double> right)
{
    return left.real() * right.real() + left.imag() * right.imag();
}

double label_for_paint::path_line::distance(int x, int y)
{
    if (p1 && p2)
    {
        std::complex<double> z1_0(x - p1->x(), y - p1->y());//вектор из первой точки
        std::complex<double> z2_0(x - p2->x(), y - p2->y());//вектор из второй точки
        std::complex<double> z1_2(p2->x() - p1->x(), p2->y() - p1->y());//вектор между точками
        auto zn1_2 = normalize(z1_2);
        double scalar_component = dot_complex(z1_0, zn1_2);
        auto z0_line = z1_0 - scalar_component * zn1_2;//вектор в ближайшую точку на линии
        double dist_to_line = std::abs(z0_line);
        double disp_to_p1 = std::abs(z1_0);
        double disp_to_p2 = std::abs(z2_0);
        if (scalar_component < 0) return disp_to_p1;
        if (scalar_component > std::abs(z1_2)) return disp_to_p2;
        return dist_to_line;
    }
    return INFINITY;
}
