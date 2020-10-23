#include "deviceCntl.hpp"

#include "driverMgmt.hpp"
#include "fltCmnLibs_string.hpp"
#include "utilities/contextMgr_Defs.hpp"
#include "utilities/volumeMgr.hpp"
#include "utilities/contextMgr.hpp"

#include "policies/GlobalFilter.hpp"

#include "WinIOIsolation_IOCTL.hpp"
#include "metadata/Metadata.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

BOOLEAN FLTAPI FastIoDeviceControl( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength,
                                    PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus,
                                    PDEVICE_OBJECT DeviceObject )
{
    UNREFERENCED_PARAMETER( FileObject );
    UNREFERENCED_PARAMETER( Wait );

    KdPrintEx(( DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[WinIOSol] %s|PID=%d|Thread:%p|DeviceObject:%p|IRQL:%d|IOCTL=0x%08x\n", 
               __FUNCTION__, PsGetCurrentProcessId(), PsGetCurrentThread(), DeviceObject, KeGetCurrentIrql(), IoControlCode ) );

    switch( (int)IoControlCode )
    {
        case IOCTL_SET_DRIVER_STATUS: {
            DevIOCntlSetDriverStatus( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;
        case IOCTL_GET_DRIVER_STATUS: {
            DevIOCntlGetDriverStatus( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;

        case IOCTL_SET_STUB_CODE: {
            DevIOCntlSetStubCode( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;
        case IOCTL_GET_STUB_CODE: {
            DevIOCntlGetStubCode( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;

        case IOCTL_ADD_GLOBAL_POLICY: {
            DevIOCntlAddGlobalPolicy( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;
        case IOCTL_DEL_GLOBAL_POLICY_BY_MASK: {
            DevIOCntlDelGlobalPolicy( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;
        case IOCTL_GET_GLOBAL_POLICY_COUNT: {
            DevIOCntlGetGlobalPolicyCnt( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;
        case IOCTL_RST_GLOBAL_POLICY: {
            DevIOCntlRstGlobalPolicy( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;

        case IOCTL_FILE_GET_FILE_TYPE: {
            DevIOCntlFileGetType( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;
        case IOCTL_FILE_SET_SOLUTION_DATA: {
            DevIOCntlFileSetSolutionData( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;
        case IOCTL_FILE_GET_SOLUTION_DATA: {
            DevIOCntlFileGetSolutionData( FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject );
        } break;

    }

    // NOTE: 이곳에서 FALSE 를 반환하면 IO 관리자는 IRP 를 생성하여 전달한다 
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL Handler


BOOLEAN DevIOCntlSetDriverStatus( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != (ULONG)PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBuffer == NULLPTR || InputBufferLength < sizeof( BOOLEAN ) )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if( *(BOOLEAN*)InputBuffer != FALSE )
            InterlockedIncrement( &FeatureContext.IsRunning );
        else
            InterlockedDecrement( &FeatureContext.IsRunning );

        IoStatus->Status = STATUS_SUCCESS;

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlGetDriverStatus( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( OutputBuffer == NULLPTR || OutputBufferLength < sizeof(BOOLEAN) )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        *( BOOLEAN* )OutputBuffer = FeatureContext.IsRunning > 0;
        IoStatus->Status = STATUS_SUCCESS;

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlSetStubCode( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBuffer == NULLPTR || InputBufferLength <= 0 )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if( InputBufferLength > METADATA_MAXIMUM_CONTAINOR_SIZE )
        {
            IoStatus->Status = STATUS_BUFFER_OVERFLOW;
            break;
        }

        SetMetaDataStubCode( InputBuffer, InputBufferLength, NULL, 0 );

        IoStatus->Status = STATUS_SUCCESS;

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlGetStubCode( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( OutputBuffer == NULLPTR || OutputBufferLength < METADATA_MAXIMUM_CONTAINOR_SIZE )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        IoStatus->Status = STATUS_BUFFER_OVERFLOW;

        __try
        {
            auto StubCodeX86 = GetStubCodeX86();
            auto StubCodeX86Size = GetStubCodeX86Size();

            RtlZeroMemory( OutputBuffer, OutputBufferLength );
            RtlCopyMemory( OutputBuffer, StubCodeX86, StubCodeX86Size );

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = StubCodeX86Size;
        }
        __finally
        {
            
        }

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlAddGlobalPolicy( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBufferLength == 0 || InputBuffer == NULL )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        WCHAR* wszFilterMask = NULLPTR;
        WCHAR* wszBuffer = ( WCHAR* )InputBuffer;
        bool isInclude = nsUtils::stoul( wszBuffer, &wszFilterMask, 10 ) > 0;

        if( *wszFilterMask != L'|' )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        wszFilterMask++;
        if( wcslen( wszFilterMask ) == 0 )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        IoStatus->Status = GlobalFilter_Add( wszFilterMask, isInclude );
        IoStatus->Information = 0;

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlDelGlobalPolicy( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBufferLength == 0 || InputBuffer == NULL )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        WCHAR* wszFilterMask = NULLPTR;
        WCHAR* wszBuffer = ( WCHAR* )InputBuffer;
        bool isInclude = nsUtils::stoul( wszBuffer, &wszFilterMask, 10 ) > 0;

        if( *wszFilterMask != L'|' )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        wszFilterMask++;
        if( wcslen( wszFilterMask ) == 0 )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        IoStatus->Status = GlobalFilter_Remove( wszFilterMask );
        IoStatus->Information = 0;

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlGetGlobalPolicyCnt( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBufferLength == 0 || InputBuffer == NULL )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        // TODO:

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlRstGlobalPolicy( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        // TODO:

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlFileGetType( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( InputBuffer == NULLPTR || InputBufferLength == 0 )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if( OutputBuffer == NULLPTR || OutputBufferLength != sizeof(ULONG) )
        {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus->Information = sizeof( ULONG );
            break;
        }

        METADATA_DRIVER MetaDataInfo;
        *(ULONG*)OutputBuffer = GetFileMetaDataInfo( ( WCHAR* )InputBuffer, &MetaDataInfo, NULLPTR, 0 );

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof( ULONG );

    } while( false );
    
    return TRUE;
}

BOOLEAN DevIOCntlFileSetSolutionData( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {
        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        // TODO:

    } while( false );
    return TRUE;
}

BOOLEAN DevIOCntlFileGetSolutionData( PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT DeviceObject )
{
    do
    {

        // TODO:

    } while( false );

    return TRUE;
}
