#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_WinIOMonitorControl.h"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

class WinIOMonitorControl : public QMainWindow
{
    Q_OBJECT

public:
    WinIOMonitorControl(QWidget *parent = Q_NULLPTR);

private:
    Ui::WinIOMonitorControlClass ui;
};
