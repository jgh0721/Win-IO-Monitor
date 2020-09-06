#include "stdafx.h"
#include "WinIOMonitorControl.h"

#include "mdlActivity.hpp"

#include "WinIOMonitor_API.hpp"

#include "UIs/dlgProcessLifecyclePolicy.hpp"

#include <QtSql>

#include <QtitanBase.h>
#include <QtitanGrid.h>
#include <QtitanDBGrid.h>

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

const QString SQL_TABLE = "";

/*!

    CREATE TABLE IF NOT EXISTS TBL_DATA (
    
    )
*/

WinIOMonitorControl::WinIOMonitorControl( QWidget* parent )
    : QMainWindow( parent )
{
    ui.setupUi( this );

    QMetaObject::invokeMethod( this, "Initialize", Qt::QueuedConnection );
}

void WinIOMonitorControl::Initialize()
{
    bool isSuccess = false;

    do
    {
        InitializeUIs();
        IF_FALSE_BREAK( isSuccess, InitializeDBMS() );

        DWORD dwRet = ConnectTo();

        if( dwRet != ERROR_SUCCESS )
        {
            QMessageBox::information( this,
                                      tr( "Win I/O Monitor" ),
                                      tr( "cannot connect to WinIOMonitor Driver" ) );

            // isSuccess = false;
            break;
        }

    } while( false );

    if( isSuccess == false )
        qApp->quit();
}

void WinIOMonitorControl::on_acProcess_Lifecycle_Control_triggered( bool checked )
{
    auto ui = new CProcLifecyclePolicyUI;
    ui->show();
}

void WinIOMonitorControl::on_acFilesystem_Per_Process_Control_triggered( bool checked )
{

}

void WinIOMonitorControl::InitializeUIs()
{
    auto view = ui.gridActivity->view<GridBandedTableView>();
    auto& viewOpts = view->options();

    view->beginUpdate();
    viewOpts.setAutoCreateColumns( false );
    viewOpts.setAlternatingRowColors( true );
    viewOpts.setColumnAutoWidth( true );
    viewOpts.setColumnHorSizingEnabled( true );
    viewOpts.setBandsHeader( false );

    model = new CActivityModel;

    view->setModel( model );
    view->addColumns();

    Qtitan::GridBandedTableColumn* column = NULLPTR;

    //auto contextBand = view->getBand( 0 );
    //contextBand->setCaption( tr( "추적 문맥" ) );
    //auto fileBand = view->addBand( tr( "원본 파일" ) );
    //view->createBandRow( fileBand, contextBand->index(), Qtitan::ColumnMovePosition::NextRowPosition );

    //column = ( GridBandedTableColumn* )view->getColumn( TBL_LOG_INFO_IDX_EVTTYPE.first );
    //column->setBandIndex( contextBand->index() );
    //column->setVisible( true );
    //column->setMaxWidth( 70 );
    //column->setTextAlignment( Qt::AlignCenter );

    //column = ( GridBandedTableColumn* )view->getColumn( TBL_LOG_INFO_IDX_EVTTIME.first );
    //column->setBandIndex( contextBand->index() );
    //column->setVisible( true );
    //column->setMaxWidth( 150 );
    //column->setTextAlignment( Qt::AlignCenter );

    //column = ( GridBandedTableColumn* )view->getColumn( TBL_LOG_INFO_IDX_PROCESSID.first );
    //column->setBandIndex( contextBand->index() );
    //column->setVisible( true );
    //column->setMaxWidth( 90 );
    //column->setTextAlignment( Qt::AlignCenter );

    //column = ( GridBandedTableColumn* )view->getColumn( TBL_LOG_INFO_IDX_PROCESSNAME.first );
    //column->setBandIndex( contextBand->index() );
    //column->setVisible( true );
    //column->setTextAlignment( Qt::AlignCenter );

    //column = ( GridBandedTableColumn* )view->getColumn( TBL_LOG_INFO_IDX_SRCPATH.first );
    //column->setBandIndex( fileBand->index() );
    //column->setRowIndex( 1 );
    //column->setVisible( true );
    //column->setTextAlignment( Qt::AlignCenter );

    //column = ( GridBandedTableColumn* )view->getColumn( TBL_LOG_INFO_IDX_SRCNAME.first );
    //column->setBandIndex( fileBand->index() );
    //column->setRowIndex( 1 );
    //column->setVisible( true );
    //column->setTextAlignment( Qt::AlignCenter );

    //column = ( GridBandedTableColumn* )view->getColumn( TBL_LOG_INFO_IDX_DSTPATH.first );
    //column->setBandIndex( fileBand->index() );
    //column->setRowIndex( 1 );
    //column->setVisible( true );
    //column->setTextAlignment( Qt::AlignCenter );

    //column = ( GridBandedTableColumn* )view->getColumn( TBL_LOG_INFO_IDX_DSTNAME.first );
    //column->setBandIndex( fileBand->index() );
    //column->setRowIndex( 1 );
    //column->setVisible( true );
    //column->setTextAlignment( Qt::AlignCenter );


    view->endUpdate();
}

bool WinIOMonitorControl::InitializeDBMS()
{
    bool isSuccess = false;

    do
    {
        // TODO: Create DBMS Manager and then move code to DBMS Manager 
        QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", QString( "create" ) );

#if CMN_BUILD_MODE == CMN_BUILD_DEBUG
        db.setDatabaseName( QApplication::applicationDirPath() + "/storage.db" );
#else
        db.setDatabaseName( ":memory:" );
#endif
        IF_FALSE_BREAK( isSuccess, db.open( "", "" ) );

        const QString TBL_STORAGE = "CREATE TABLE IF NOT EXISTS TBL_STORAGE ( "
                                    "EvtID                  INTEGER DEFAULT 0, "
                                    "EventTime              DATETIME, "
                                    "Category               INTEGER DEFAULT 0, "
                                    "Type                   INTEGER DEFAULT 0, "
                                    "ProcessId              INTEGER DEFAULT 0 "

                                    " )";
        QSqlQuery dbmsQuery = db.exec( TBL_STORAGE );
        if( dbmsQuery.lastError().type() != QSqlError::NoError )
        {
            isSuccess = false;
            break;
        }
        ;

        
        
        isSuccess = true;

    } while( false );

    return isSuccess;
}
