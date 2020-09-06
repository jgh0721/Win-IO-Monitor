#include "stdafx.h"
#include "mdlActivity.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

void CActivityModel::Refresh()
{
}

QModelIndex CActivityModel::index( int row, int column, const QModelIndex& parent /* = QModelIndex() */ ) const
{
    if( row < 0 || column < 0 || vecRecords.size() <= 0 )
        return QModelIndex();

    if( row >= vecRecords.size() )
        return QModelIndex();

    return createIndex( row, column );
}

QModelIndex CActivityModel::parent( const QModelIndex& child ) const
{
    return QModelIndex();
}

int CActivityModel::rowCount( const QModelIndex& parent /*= QModelIndex() */ ) const
{
    return vecRecords.size();
}

int CActivityModel::columnCount( const QModelIndex& parent /*= QModelIndex() */ ) const
{
    return 8;
}

QVariant CActivityModel::data( const QModelIndex& index, int role ) const
{
    return QVariant();
}

QVariant CActivityModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Vertical )
        return QVariant();

    if( role != Qt::DisplayRole )
        return QVariant();

    switch( section )
    {
        case 0: return tr( "Category" );
        case 1: return tr( "Type" );
        case 2: return tr( "Time" );
        case 3: return tr( "ProcessId" );
        case 4: return tr( "ProcessName" );
        case 5: return tr( "Src" );
        case 6: return tr( "Dst" );
        case 7: return tr( "Contents" );
            break;
        default:
            Q_ASSERT( false );
    };

    return QVariant();
}

