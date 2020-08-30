#include "WinIOMonitorControl.h"
#include "stdafx.h"
#include <QtWidgets/QApplication>

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WinIOMonitorControl w;
    w.show();
    return a.exec();
}
