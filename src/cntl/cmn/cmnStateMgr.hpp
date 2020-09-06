#ifndef HDR_INTERNAL_STATE_MGR
#define HDR_INTERNAL_STATE_MGR

#include "cmnBase.hpp"

#if defined( USE_QT_SUPPORT )
#include <QtCore>
#endif

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#if defined(USE_QT_SUPPORT)
class QStateMgr : public QObject
{
public:

    Q_SIGNAL void sigStateChange( int valueChanged );
};

#endif

template< typename T >
class CStateMgr
#if defined( USE_QT_SUPPORT )
    : public QStateMgr
#endif
{
public:
    T GetState()
    {
        return static_cast< T >( _value.loadAcquire() );
    }

    void SetState( T state )
    {
        _value.storeRelease( static_cast< int >( state ) );
        emit sigStateChange( _value );
    }

private:
#if defined(USE_QT_SUPPORT)
    QAtomicInt                          _value;
#endif
};

#endif // HDR_INTERNAL_STATE_MGR
