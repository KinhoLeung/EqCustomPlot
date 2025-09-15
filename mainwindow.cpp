#include "mainwindow.h"
#include "eqcustomplot.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->centralWidget()->setLayout(new QVBoxLayout(this->centralWidget()));
    this->centralWidget()->layout()->addWidget(new EqCustomPlot(this));
}

MainWindow::~MainWindow()
{
    delete ui;
}
