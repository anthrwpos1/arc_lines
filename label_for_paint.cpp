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

//определяет ближайший отрезок (возвращает итератор на вторую точку этого отрезка)
std::_List_iterator<path_point> label_for_paint::getNearestLine(int mouse_x, int mouse_y, double *distance)
{
    double min_dist = -1;
    std::_List_iterator<path_point> result;
    auto it_2 = points.begin();
    if (it_2 == points.end()) return it_2;//если только одна точка - возвращаем итератор в конец списка
    auto it_1 = points.end();
    --it_1;//предпоследняя точка
    if (it_1 == it_2) return points.end();//если точек две, возвращаем так-же итератор в конец
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

/* отрисовка ломанной дуги окружности из одной точки в другую
 * под заданным углом относительно соединяющей их прямой
 *
 * пусть А - первая точка, B - вторая, заданы комплексным числом.
 * t - вектор из точки А в первую точку дуги
 * c - единичный вектор поворота который изменяет вектор между точками дуги.
 *
 * Тогда B = A + t + ct + c^2t + ... c^n t, где n - число точек ломанной.
 * Отсюда B - A = t(1 + c + c^2 + ... + c^n)
 * скобка в правой части - геометрическая прогрессия, так что
 * B - A = t (1-c^n) / (1-c), откуда t = (Β - Α) * (1 - c) / (1-c^n)
 *
 * Из геометрии легко увидеть, что Arg(c) = 2F/n, где F - угол между прямой,
 * соединяющей точки и касательной к дуге, n - число точек ломанной.
 */
void label_for_paint::paintArc(QPainter &painter, const path_point &from, const path_point &to, const int pnum)
{
    double angle = to._arc_angle;
    std::complex<double> vec_from_to = {(double) (to.x() - from.x()),(double) (to.y() - from.y())};
    std::complex<double> single_rotation = std::exp(std::complex<double>{0, -2*angle/pnum});
    std::complex<double> complete_rotation = std::exp(std::complex<double>{0, -2*angle});
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
            paintArc(painter, p, *last);
        }
        last = &p;
    }
    if (closed && last) paintArc(painter, points.front(), *last);
}

//вычисляем углы между прямыми соединяющими точки
void label_for_paint::calc_lines_angles()
{
    using PPIterator = std::_List_iterator<path_point>;
    using complex = std::complex<double>;
    if (points.empty()) return;//элементов нет
    PPIterator point1 = points.begin();
    PPIterator point2 = point1;
    ++point2;
    if (point2 == points.end()) return;//только 1 элемент
    complex vector_12 {(double) (point2->x() - point1->x()), (double) (point2->y() - point1->y())};
    double vector_12_length = std::abs(vector_12);
    complex vector_12_normalized = vector_12 / vector_12_length;
    PPIterator point3 = point2;
    ++point3;
    while (point3 != points.end())
    {
        complex vector_23 {(double) (point3->x() - point2->x()), (double) (point3->y() - point2->y())};
        double vector_23_length = std::abs(vector_23);
        complex vector_23_normalized = vector_23 / vector_23_length;
        point2->_lines_rot = vector_23_normalized / vector_12_normalized;
        vector_12_normalized = vector_23_normalized;
        point1 = point2;
        point2 = point3;
        ++point3;
    }
    point3 = points.begin();
    for (int more_2_points = 0; more_2_points < 2; ++more_2_points)
    {
        complex vector_23 {(double) (point3->x() - point2->x()), (double) (point3->y() - point2->y())};
        double vector_23_length = std::abs(vector_23);
        complex vector_23_normalized = vector_23 / vector_23_length;
        point2->_lines_rot = vector_23_normalized / vector_12_normalized;
        vector_12_normalized = vector_23_normalized;
        point1 = point2;
        point2 = point3;
        ++point3;
    }
}

void label_for_paint::calcArcAngles()
{
    //вычисляем углы таким образом, чтобы замкнутая кривая
    //не содержала изломов. Из решения геометрии существуют
    //две таких кривых, если число точек нечетно
    //и для стороны A1A2 многоугольника A1A2A3...An
    //угол между прямой A1A2 и касательной к дуге соединяющей
    //A1 с A2 равен +-(F1+F2-F3+F4-F5+F6...-Fn) / 2, где Fn -
    //угол между прямыми An-1An и AnAn+1.
    //
    //если число точек четно, решения либо нет,
    //либо любая плавная цепочка дуг замыкается без излома
    calc_lines_angles();
    if (closed && (points.size() % 2 == 1))
    {
        int odd = 1;
        std::complex<double> angles_sum = 0;
        auto it = points.begin();
        if (it != points.end()) angles_sum = it->_lines_rot;
        ++it;
        if (it != points.end())
        {
            while(it != points.end())
            {
                if (odd) angles_sum *= it->_lines_rot;
                else angles_sum /= it->_lines_rot;
                ++it;
                odd = 1 - odd;
            }
            points.front()._arc_angle = std::arg(angles_sum) / 2.0;
        }
    }
    //вычисляем плавную цепочку дуг от первой точки.
    //Пусть A, B, C - три точки
    //V1 - угол между касательной к дуге AB и прямой AB
    //V2 - угол между касательной к дуге BC и прямой BC
    //тогда V2 = F - V1, где F - угол между прямыми AB и BC
    path_point* prev_point = nullptr;
    for (auto& p : points)
    {
        if (prev_point)
        {
            std::complex<double> old_angle = std::exp(std::complex<double>{0,prev_point->_arc_angle});//единичный вектор направления от предыдущей точки
            std::complex<double> next_angle = p._lines_rot / old_angle; //единичный вектор направления из вычисляемой точки
            p._arc_angle = std::arg(next_angle);//записываем угол
        }
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
    calcArcAngles();
    repaint();
}

void label_for_paint::mouseMoveEvent(QMouseEvent *ev)
{
    auto mouse_x = ev->x();
    auto mouse_y = ev->y();
    //ближайшая к курсору точка
    double dist;
    bool need_update = false;
    path_point* nearest = getNearestPoint(mouse_x, mouse_y, &dist);

    //ближайшая к курсору линия
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
        calcArcAngles();
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


//расстояние из точки до линии
//определяется как расстояние до ближайшей точки
//если перпендикуляр опущенный на прямую соединяющей точки
//попадает мимо отрезка
//либо длина этого перпендикуляра в ином случае.
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
