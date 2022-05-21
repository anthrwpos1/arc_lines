#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //connect(ui->PaintFrame, &label_for_paint::mouseMove, this, &MainWindow::refresh_mouse_coord);
    connect(ui->PaintFrame, &label_for_paint::show_dist_to_line, this, &MainWindow::show_dist_to_line);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_checkBox_stateChanged(int arg1)
{
    ui->PaintFrame->closed = arg1;
    ui->PaintFrame->repaint();
}


void MainWindow::on_circling_stateChanged(int arg1)
{
    ui->PaintFrame->circled = arg1;
    ui->PaintFrame->repaint();
}

void MainWindow::show_dist_to_line(double arg)
{
    ui->statusbar->showMessage(QString::number(arg));
}

