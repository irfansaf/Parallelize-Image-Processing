#include "stdafx.h"
#include "midterm.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    midterm w;
    w.show();
    return a.exec();
}