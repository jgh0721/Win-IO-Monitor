#include "stdafx.h"
#include "WinIOMonitorControl.h"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

WinIOMonitorControl::WinIOMonitorControl( QWidget* parent )
    : QMainWindow( parent )
{
    ui.setupUi( this );
}
