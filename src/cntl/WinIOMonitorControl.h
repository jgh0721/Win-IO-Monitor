#pragma once

#include "cmnBase.hpp"

#include <QtWidgets/QMainWindow>
#include "ui_WinIOMonitorControl.h"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

class CActivityModel;

class WinIOMonitorControl : public QMainWindow
{
    Q_OBJECT

public:
    WinIOMonitorControl( QWidget* parent = Q_NULLPTR );


    Q_INVOKABLE void Initialize();

public slots:

    void                                    on_acProcess_Lifecycle_Control_triggered( bool checked = false );
    void                                    on_acFilesystem_Per_Process_Control_triggered( bool checked = false );

private:

    void                                    InitializeUIs();
    bool                                    InitializeDBMS();


    Ui::WinIOMonitorControlClass            ui;
    CActivityModel*                         model = NULLPTR;
};
