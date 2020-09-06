#ifndef HDR_UI_PROCESS_LIFECYCLE_POLICY
#define HDR_UI_PROCESS_LIFECYCLE_POLICY

#include "cmnBase.hpp"

#include <QtWidgets>

#include "ui_dlgProcessLifecyclePolicy.h"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

class CProcLifecyclePolicyUI : public QDialog
{
    Q_OBJECT
public:
    CProcLifecyclePolicyUI( QWidget* parent = 0 );

public slots:

    void                                    Refresh();

    void                                    on_btnAddProcLCPolicy_clicked( bool checked = false );
    void                                    on_btnRemoveProcLCPolicy_clicked( bool checked = false );

private:

    void                                    resetControls();

    Ui::dlgPolicyProcLifecycle              ui;
};

#endif // HDR_UI_PROCESS_LIFECYCLE_POLICY