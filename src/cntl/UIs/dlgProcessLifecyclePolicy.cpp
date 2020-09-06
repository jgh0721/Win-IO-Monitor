#include "stdafx.h"
#include "dlgProcessLifecyclePolicy.hpp"

#include "WinIOMonitor_API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

CProcLifecyclePolicyUI::CProcLifecyclePolicyUI( QWidget* parent )
    : QDialog( parent )
{
    ui.setupUi( this );

    resetControls();
}

void CProcLifecyclePolicyUI::Refresh()
{

}

void CProcLifecyclePolicyUI::on_btnAddProcLCPolicy_clicked( bool checked )
{
    PROCESS_FILTER ProcessFilter;
    ZeroMemory( &ProcessFilter, sizeof( PROCESS_FILTER ) );

    if( ui.rdoProcessId->isChecked() == true )
        ProcessFilter.uProcessId = ui.edtProcessNameMask->text().toUInt();
    else if( ui.rdoProcessName->isChecked() == true )
        wcsncpy_s( ProcessFilter.wszProcessMask, ui.edtProcessNameMask->text().toStdWString().c_str(), _TRUNCATE );

    if( ui.chkNotifyProcessLC->isChecked() == true )
        ProcessFilter.uFlags |= PROCESS_NOTIFY_CREATION_TERMINATION;
    if( ui.chkDenyProcessCreation->isChecked() == true )
        ProcessFilter.uFlags |= PROCESS_DENY_CREATION;
    if( ui.chkDenyProcessTermination->isChecked() == true )
        ProcessFilter.uFlags |= PROCESS_DENY_TERMINATION;

    DWORD dwRet = AddProcessFileFilterMask( &ProcessFilter );

    if( dwRet == ERROR_SUCCESS )
    {
        QTableWidgetItem* item = NULLPTR;
        auto rowCount = ui.tblProcessLCPolicy->rowCount();
        ui.tblProcessLCPolicy->setRowCount( rowCount + 1 );

        item = new QTableWidgetItem;
        item->setText( QString("%1").arg( ui.rdoProcessId->isChecked() == true ? "PID" : "MASK" ) );
        ui.tblProcessLCPolicy->setItem( rowCount, 0, item );
        item = new QTableWidgetItem;
        item->setText( ui.edtProcessNameMask->text().trimmed() );
        ui.tblProcessLCPolicy->setItem( rowCount, 1, item );

        item = new QTableWidgetItem;
        item->setText( QString("%1").arg( ProcessFilter.uFlags ) );
        ui.tblProcessLCPolicy->setItem( rowCount, 2, item );

        resetControls();
    }
    else
    {
        QMessageBox::information( this, tr( "Win I/O Monitor" ), tr( "Failed to set process filter : %1" ).arg( dwRet ) );
    }
}

void CProcLifecyclePolicyUI::on_btnRemoveProcLCPolicy_clicked( bool checked )
{
    auto currentRowIdx = ui.tblProcessLCPolicy->currentRow();
    if( currentRowIdx < 0 )
        return;

    QTableWidgetItem* item = NULLPTR;

    bool isPID = false;
    QString sText;
    DWORD dwRet = 0;

    item = ui.tblProcessLCPolicy->item( currentRowIdx, 0 );
    if( item->text().compare( "PID" ) )
        isPID = true;

    item = ui.tblProcessLCPolicy->item( currentRowIdx, 1 );
    sText = item->text();

    if( isPID == true )
        dwRet = RemoveProcessFileFilterMask( sText.toUInt() );
    else
        dwRet = RemoveProcessFileFilterMask( sText.toStdWString().c_str() );

    if( dwRet == ERROR_SUCCESS )
    {
        ui.tblProcessLCPolicy->removeRow( currentRowIdx );
    }
    else
    {
        QMessageBox::information( this, tr( "Win I/O Monitor" ), tr( "Failed to remove process filter : %1" ).arg( dwRet ) );
    }
}

void CProcLifecyclePolicyUI::resetControls()
{
    ui.rdoProcessId->setChecked( true );
    ui.edtProcessNameMask->clear();
    ui.chkNotifyProcessLC->setChecked( true );
    ui.chkDenyProcessCreation->setChecked( false );
    ui.chkDenyProcessTermination->setChecked( false );
}
