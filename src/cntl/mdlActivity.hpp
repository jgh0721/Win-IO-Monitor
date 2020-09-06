#ifndef HDR_MODEL_IO_ACTIVITY
#define HDR_MODEL_IO_ACTIVITY

#include "cmnBase.hpp"
#include "WinIOMonitor_Event.hpp"
#include <QtCore>

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

struct TActivityModelItem
{
    LONG                            EvtID;
    QDateTime                       EventTime;
    LONG                            Category;
    ULONG                           Type;

    ULONG                           ProcessId;
    QString                         ProcessFullPath;
    QStringRef                      ProcessName;

    QString                         Src;
    QString                         Dst;

    MSG_PARAMETERS                  Parameters;
    QString                         ParametersText;
};

class CActivityModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    void                            Refresh();


    virtual QModelIndex index( int row, int column, const QModelIndex& parent ) const override;
    virtual QModelIndex parent( const QModelIndex& child ) const override;
    virtual int rowCount( const QModelIndex& parent ) const override;
    virtual int columnCount( const QModelIndex& parent ) const override;
    virtual QVariant data( const QModelIndex& index, int role ) const override;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;

private:

    QVector< TActivityModelItem >   vecRecords;
};

#endif // HDR_MODEL_IO_ACTIVITY