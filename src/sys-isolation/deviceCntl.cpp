#include "deviceCntl.hpp"

#include "driverMgmt.hpp"
#include "fltCmnLibs_string.hpp"
#include "utilities/contextMgr_Defs.hpp"
#include "utilities/volumeMgr.hpp"
#include "utilities/contextMgr.hpp"

#include "policies/GlobalFilter.hpp"
#include "cipher/Cipher_Krnl.hpp"
#include "metadata/Metadata.hpp"

#include "WinIOIsolation_IOCTL.hpp"

#pragma warning(disable: 4311)
#pragma warning(disable: 4302)

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
        case IOCTL_SET_DRIVER_CONFIG: {
            DevIOCntlSetDriverConfig( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_GET_DRIVER_CONFIG: {
            DevIOCntlGetDriverConfig( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;

        case IOCTL_SET_DRIVER_STATUS: {
            DevIOCntlSetDriverStatus( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_GET_DRIVER_STATUS: {
            DevIOCntlGetDriverStatus( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;

        case IOCTL_SET_STUB_CODE: {
            DevIOCntlSetStubCode( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_GET_STUB_CODE: {
            DevIOCntlGetStubCode( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;

        case IOCTL_ADD_GLOBAL_POLICY: {
            DevIOCntlAddGlobalPolicy( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_DEL_GLOBAL_POLICY_BY_MASK: {
            DevIOCntlDelGlobalPolicy( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_GET_GLOBAL_POLICY_COUNT: {
            DevIOCntlGetGlobalPolicyCnt( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_RST_GLOBAL_POLICY: {
            DevIOCntlRstGlobalPolicy( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_GET_GLOBAL_POLICY: {
            DevIOCntlGetGlobalPolicy( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;

        case IOCTL_ADD_PROCESS_POLICY: {
            DevIOCntlAddProcessPolicy( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_DEL_PROCESS_POLICY_BY_ID: {
            DevIOCntlDelProcessPolicyID( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_DEL_PROCESS_POLICY_BY_PID: {
            DevIOCntlDelProcessPolicyPID( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_DEL_PROCESS_POLICY_BY_MASK: {
            DevIOCntlDelProcessPolicyMask( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_ADD_PROCESS_POLICY_ITEM: {
            DevIOCntlAddProcessPolicyItem( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_DEL_PROCESS_POLICY_ITEM: {
            DevIOCntlDelProcessPolicyItem( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_RST_PROCESS_POLICY: {
            DevIOCntlRstProcessPolicy( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;

        case IOCTL_FILE_GET_FILE_TYPE: {
            DevIOCntlFileGetType( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_FILE_SET_SOLUTION_DATA: {
            DevIOCntlFileSetSolutionData( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_FILE_GET_SOLUTION_DATA: {
            DevIOCntlFileGetSolutionData( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_FILE_ENCRYPT_FILE: {
            DevIOCntlFileEncrypt( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;
        case IOCTL_FILE_DECRYPT_FILE: {
            DevIOCntlFileDecrypt( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoStatus );
        } break;

    }

    // NOTE: 이곳에서 FALSE 를 반환하면 IO 관리자는 IRP 를 생성하여 전달한다 
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL Handler

BOOLEAN DevIOCntlSetDriverConfig( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBuffer == NULLPTR || InputBufferLength < sizeof( DRIVER_CONFIG ) )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        auto DriverConfig = ( DRIVER_CONFIG* )InputBuffer;
        if( InputBufferLength != DriverConfig->SizeOfStruct )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        SetTimeOutMs( DriverConfig->TimeOutMs );

        for( int idx = 0; DriverConfig->SolutionMetaDataSize; ++idx )
        {
            RtlCopyMemory( &FeatureContext.EncryptContext[ idx ], &DriverConfig->EncryptConfig[idx],
                           sizeof( ENCRYPT_CONTEXT ) );
        }

        PVOID StubCodeX86 = NULLPTR; ULONG StubCodeX86Size = 0;
        PVOID StubCodeX64 = NULLPTR; ULONG StubCodeX64Size = 0;

        if( DriverConfig->LengthOfStubCodeX86 > 0 && DriverConfig->OffsetOfStubCodeX86 > 0 )
        {
            StubCodeX86 = Add2Ptr( DriverConfig, DriverConfig->OffsetOfStubCodeX86 );
            StubCodeX86Size = DriverConfig->LengthOfStubCodeX86;
        }

        if( DriverConfig->LengthOfStubCodeX64 > 0 && DriverConfig->OffsetOfStubCodeX64 > 0 )
        {
            StubCodeX64 = Add2Ptr( DriverConfig, DriverConfig->OffsetOfStubCodeX64 );
            StubCodeX64Size = DriverConfig->LengthOfStubCodeX64;
        }

        SetMetaDataStubCode( StubCodeX86, StubCodeX86Size, StubCodeX64, StubCodeX64Size );

        IoStatus->Status = STATUS_SUCCESS;

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlGetDriverConfig( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBuffer == NULLPTR || InputBufferLength < sizeof( BOOLEAN ) )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        IoStatus->Status = STATUS_SUCCESS;

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlSetDriverStatus( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

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

BOOLEAN DevIOCntlGetDriverStatus( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

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

BOOLEAN DevIOCntlSetStubCode( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

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

BOOLEAN DevIOCntlGetStubCode( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

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

BOOLEAN DevIOCntlAddGlobalPolicy( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

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

BOOLEAN DevIOCntlDelGlobalPolicy( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

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
        UNREFERENCED_PARAMETER( isInclude );

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

BOOLEAN DevIOCntlGetGlobalPolicyCnt( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBufferLength != sizeof(BOOLEAN) || InputBuffer == NULL )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        if( OutputBufferLength != sizeof(ULONG) || OutputBuffer == NULLPTR )
        {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus->Information = sizeof( ULONG );
            break;
        }

        bool IsInclude = *( BOOLEAN* )InputBuffer != 0;
        RtlZeroMemory( OutputBuffer, OutputBufferLength );
        *( ULONG* )OutputBuffer = GlobalFilter_Count( IsInclude );

        IoStatus->Status = STATUS_SUCCESS;

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlRstGlobalPolicy( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        IoStatus->Status = GlobalFilter_Reset();

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlGetGlobalPolicy( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBufferLength != sizeof( BOOLEAN ) || InputBuffer == NULL )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        bool IsInclude = *( BOOLEAN* )InputBuffer != 0;
        ULONG Count = GlobalFilter_Count( IsInclude );

        if( ( OutputBufferLength != ( sizeof( USER_GLOBAL_FILTER ) * Count ) ) || 
            ( OutputBuffer == NULLPTR ) )
        {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus->Information = sizeof( USER_GLOBAL_FILTER ) * Count;
            break;
        }

        RtlZeroMemory( OutputBuffer, OutputBufferLength );
        IoStatus->Status = GlobalFilter_Copy( OutputBuffer, OutputBufferLength, IsInclude );

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlAddProcessPolicy( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBufferLength != sizeof( USER_PROCESS_FILTER ) || InputBuffer == NULLPTR  )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        const auto UPFilterItem = ( USER_PROCESS_FILTER* )InputBuffer;
        auto PFilterItem = ( PROCESS_FILTER_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER_ENTRY ), POOL_MAIN_TAG );
        RtlZeroMemory( PFilterItem, sizeof( PROCESS_FILTER_ENTRY ) );
        RtlCopyMemory( &PFilterItem->Id, &UPFilterItem->Id, sizeof( UUID ) );
        PFilterItem->ProcessId = UPFilterItem->ProcessId;
        RtlCopyMemory( PFilterItem->ProcessFilterMask, UPFilterItem->ProcessFilterMask, MAX_PATH * sizeof( WCHAR ) );
        PFilterItem->ProcessFilter = UPFilterItem->ProcessFilter;
        InitializeListHead( &PFilterItem->IncludeListHead );
        InitializeListHead( &PFilterItem->ExcludeListHead );

        IoStatus->Status = ProcessFilter_Add( PFilterItem );

        if( !NT_SUCCESS( IoStatus->Status ) )
            ExFreePoolWithTag( PFilterItem, POOL_MAIN_TAG );

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlDelProcessPolicyID( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBufferLength != sizeof(UUID) || InputBuffer == NULL )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        IoStatus->Status = ProcessFilter_Remove( ( UUID* )InputBuffer );

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlDelProcessPolicyPID( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBufferLength != sizeof(ULONG) || InputBuffer == NULL )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        IoStatus->Status = ProcessFilter_Remove( *(ULONG*)InputBuffer, NULLPTR );

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlDelProcessPolicyMask( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

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

        IoStatus->Status = ProcessFilter_Remove( 0, (WCHAR*)InputBuffer );

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlAddProcessPolicyItem( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( ( InputBufferLength != ( sizeof( UUID ) + sizeof( USER_PROCESS_FILTER_ENTRY ) ) ) || InputBuffer == NULL )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        const auto PFilterItemId = ( UUID* )InputBuffer;
        const auto UserPFilterItemEntry = ( USER_PROCESS_FILTER_ENTRY* )( Add2Ptr( InputBuffer, sizeof(UUID) ) );
        auto PFilterItemEntry = ( PROCESS_FILTER_MASK_ENTRY* )ExAllocatePoolWithTag( NonPagedPool, sizeof( PROCESS_FILTER_MASK_ENTRY ), POOL_MAIN_TAG );
        RtlZeroMemory( PFilterItemEntry, sizeof( PROCESS_FILTER_MASK_ENTRY ) );

        RtlCopyMemory( &PFilterItemEntry->Id, &UserPFilterItemEntry->Id, sizeof( UUID ) );
        PFilterItemEntry->FilterCategory = UserPFilterItemEntry->FilterCategory;
        PFilterItemEntry->FilterType = UserPFilterItemEntry->FilterType;
        RtlCopyMemory( PFilterItemEntry->FilterMask, UserPFilterItemEntry->FilterMask, MAX_PATH * sizeof( WCHAR ) );
        PFilterItemEntry->IsManagedFile = UserPFilterItemEntry->IsManagedFile;
        
        IoStatus->Status = ProcessFilter_AddEntry( PFilterItemId, PFilterItemEntry, UserPFilterItemEntry->IsInclude );

        if( !NT_SUCCESS( IoStatus->Status ) )
            ExFreePoolWithTag( PFilterItemEntry, POOL_MAIN_TAG );

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlDelProcessPolicyItem( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( ( InputBufferLength != ( sizeof( UUID ) + sizeof( USER_PROCESS_FILTER_ENTRY ) ) ) || InputBuffer == NULL )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        if( ( InputBufferLength != ( sizeof( UUID ) + sizeof( USER_PROCESS_FILTER_ENTRY ) ) ) || InputBuffer == NULL )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        const auto PFilterItemId = ( UUID* )InputBuffer;
        const auto UserPFilterItemEntry = ( USER_PROCESS_FILTER_ENTRY* )( Add2Ptr( InputBuffer, sizeof( UUID ) ) );

        IoStatus->Status = ProcessFilter_RemoveEntry( PFilterItemId, &UserPFilterItemEntry->Id, UserPFilterItemEntry->IsInclude );

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlRstProcessPolicy( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

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

        IoStatus->Status = ProcessFilter_Reset();

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlFileGetType( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

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

BOOLEAN DevIOCntlFileSetSolutionData( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBuffer == NULLPTR || InputBufferLength <= sizeof( USER_FILE_SOLUTION_DATA ) )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        auto Buffer = ( USER_FILE_SOLUTION_DATA* )InputBuffer;
        if( Buffer->SizeOfStruct != InputBufferLength )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        auto SrcFileFullPath = ( WCHAR* )Add2Ptr( Buffer, Buffer->OffsetOfFileName );
        auto SolutionMetaData = Add2Ptr( Buffer, Buffer->OffsetOfSolutionData );

        IoStatus->Status = WriteSolutionMetaData( SrcFileFullPath, SolutionMetaData, Buffer->LengthOfSolutionData );

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlFileGetSolutionData( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBuffer == NULLPTR || InputBufferLength == 0 )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        if( OutputBuffer == NULLPTR || OutputBufferLength == 0 )
        {
            IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            IoStatus->Information = 0;  // TODO: 전역 솔루션 정보 크기로 대체
            break;
        }

        METADATA_DRIVER MetaDataInfo;
        GetFileMetaDataInfo( ( WCHAR* )InputBuffer, &MetaDataInfo,
                             &OutputBuffer, &OutputBufferLength );

        IoStatus->Status = STATUS_SUCCESS;

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlFileEncrypt( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBuffer == NULLPTR || InputBufferLength <= sizeof( USER_FILE_ENCRYPT ) )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        IoStatus->Status = CipherFile( ( USER_FILE_ENCRYPT* )InputBuffer );

    } while( false );

    return TRUE;
}

BOOLEAN DevIOCntlFileDecrypt( PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PIO_STATUS_BLOCK IoStatus )
{
    do
    {
        UNREFERENCED_PARAMETER( InputBuffer );
        UNREFERENCED_PARAMETER( InputBufferLength );
        UNREFERENCED_PARAMETER( OutputBuffer );
        UNREFERENCED_PARAMETER( OutputBufferLength );

        if( FeatureContext.CntlProcessId != ( ULONG )PsGetCurrentProcessId() )
        {
            IoStatus->Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        if( InputBuffer == NULLPTR || InputBufferLength <= sizeof( USER_FILE_ENCRYPT ) )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        auto opt = ( USER_FILE_ENCRYPT* )InputBuffer;
        if( opt->SizeOfStruct != InputBufferLength )
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            break;
        }

        PVOID SolutionMetaData = NULLPTR;
        ULONG SolutionMetaDataSize = 0;

        if( opt->LengthOfSolutionData > 0 && opt->OffsetOfSolutionData > 0 )
        {
            SolutionMetaData = Add2Ptr( opt, opt->OffsetOfSolutionData );
            SolutionMetaDataSize = opt->LengthOfSolutionData;
        }

        IoStatus->Status = DecipherFile( opt, SolutionMetaData, &SolutionMetaDataSize );

    } while( false );

    return TRUE;
}
