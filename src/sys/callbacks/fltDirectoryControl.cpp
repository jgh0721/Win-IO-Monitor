#include "fltDirectoryControl.hpp"

#include "irpContext.hpp"
#include "utilities/bufferMgr.hpp"
#include "utilities/contextMgr.hpp"
#include "utilities/fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI WinIOPreDirectoryControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PIRP_CONTEXT                IrpContext = NULLPTR;
    // auto                        IrpContext = CreateIrpContext( Data, FltObjects );

    __try
    {
        if( Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY )
            __leave;

        FltStatus = FLT_PREOP_SYNCHRONIZE;
    }
    __finally
    {
        
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI WinIOPostDirectoryControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    IRP_CONTEXT*                IrpContext = NULLPTR;
    CTX_INSTANCE_CONTEXT*       InstanceContext = NULLPTR;

    __try
    {
        if( !NT_SUCCESS( Data->IoStatus.Status ) )
            __leave;

        if( FltObjects->FileObject == NULLPTR )
            __leave;

        if( Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY )
            __leave;

        if( FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING ) )
            __leave;

        if( Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length <= 0 )
            __leave;

        CtxGetContext( FltObjects, NULLPTR, FLT_INSTANCE_CONTEXT, ( PFLT_CONTEXT* )&InstanceContext );
        if( InstanceContext == NULLPTR )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );
        if( IrpContext == NULLPTR )
            __leave;

        PrintIrpContext( IrpContext );
        auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;

        switch( FileInformationClass )
        {
            case FileDirectoryInformation: {
                TuneFileDirectoryInformation( IrpContext, InstanceContext );
            } break;
            case FileFullDirectoryInformation: {
                TuneFileFullDirectoryInformation( IrpContext, InstanceContext );
            } break;
            case FileBothDirectoryInformation: {
                TuneFileBothDirectoryInformation( IrpContext, InstanceContext );
            } break;
            case FileNameInformation: {
                TuneFileNameInformation( IrpContext, InstanceContext );
            } break;
            case FileObjectIdInformation: {} break;
            case FileQuotaInformation: {} break;
            case FileReparsePointInformation: {} break;
            case FileIdBothDirectoryInformation: {
                TuneFileIdBothDirectoryInformation( IrpContext, InstanceContext );
            } break;
            case FileIdFullDirectoryInformation: {
                TuneFileIdFullDirectoryInformation( IrpContext, InstanceContext );
            } break;
            default: {
            } break;
        }
    }
    __finally
    {
        if( IrpContext != NULLPTR )
            CloseIrpContext( IrpContext );

        CtxReleaseContext( InstanceContext );
    }

    return FltStatus;
}

///////////////////////////////////////////////////////////////////////////////

void TuneFileDirectoryInformation( IRP_CONTEXT* IrpContext, CTX_INSTANCE_CONTEXT* InstanceContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFILE_DIRECTORY_INFORMATION Buffer = NULLPTR;

    __try
    {
        Status = GetQueryDirectoryBuffer( IrpContext->Data, ( PVOID* )&Buffer );
        if( !NT_SUCCESS( Status ) )
            __leave;

        ULONG Offset = 0;
        ULONG RequiredBufferBytes = 0;

        do
        {
            Offset = Buffer->NextEntryOffset;
            RequiredBufferBytes = IrpContext->SrcFileFullPath.BufferSize + Buffer->FileNameLength + sizeof( WCHAR );
            CheckBufferSize( &IrpContext->DstFileFullPath, RequiredBufferBytes );

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbCatW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, IrpContext->SrcFileFullPath.Buffer );
            RtlStringCbCatNW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, Buffer->FileName, Buffer->FileNameLength );

            WCHAR* wszMatch = nsUtils::EndsWithW( IrpContext->DstFileFullPath.Buffer, L".dwg.exe" );
            if( wszMatch != NULLPTR )
            {
                wszMatch[ 4 ] = L'\0';
                Buffer->FileNameLength -= sizeof( WCHAR ) * 4;
            }

            Buffer = ( decltype( Buffer ) )Add2Ptr( Buffer, Offset );
            
        } while( Offset != 0 );
    }
    __finally
    {
        
    }
}

void TuneFileFullDirectoryInformation( IRP_CONTEXT* IrpContext, CTX_INSTANCE_CONTEXT* InstanceContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFILE_FULL_DIR_INFORMATION Buffer = NULLPTR;

    __try
    {
        Status = GetQueryDirectoryBuffer( IrpContext->Data, ( PVOID* )&Buffer );
        if( !NT_SUCCESS( Status ) )
            __leave;

        ULONG Offset = 0;
        ULONG RequiredBufferBytes = 0;

        do
        {
            Offset = Buffer->NextEntryOffset;
            RequiredBufferBytes = IrpContext->SrcFileFullPath.BufferSize + Buffer->FileNameLength + sizeof( WCHAR );
            CheckBufferSize( &IrpContext->DstFileFullPath, RequiredBufferBytes );

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbCatW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, IrpContext->SrcFileFullPath.Buffer );
            RtlStringCbCatNW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, Buffer->FileName, Buffer->FileNameLength );

            WCHAR* wszMatch = nsUtils::EndsWithW( IrpContext->DstFileFullPath.Buffer, L".dwg.exe" );
            if( wszMatch != NULLPTR )
            {
                wszMatch[ 4 ] = L'\0';
                Buffer->FileNameLength -= sizeof( WCHAR ) * 4;
            }

            Buffer = ( decltype( Buffer ) )Add2Ptr( Buffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {

    }
}

void TuneFileBothDirectoryInformation( IRP_CONTEXT* IrpContext, CTX_INSTANCE_CONTEXT* InstanceContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFILE_BOTH_DIR_INFORMATION Buffer = NULLPTR;

    __try
    {
        Status = GetQueryDirectoryBuffer( IrpContext->Data, ( PVOID* )&Buffer );
        if( !NT_SUCCESS( Status ) )
            __leave;

        ULONG Offset = 0;
        ULONG RequiredBufferBytes = 0;

        do
        {
            Offset = Buffer->NextEntryOffset;
            RequiredBufferBytes = IrpContext->SrcFileFullPath.BufferSize + Buffer->FileNameLength + sizeof( WCHAR );
            CheckBufferSize( &IrpContext->DstFileFullPath, RequiredBufferBytes );

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbCatW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, IrpContext->SrcFileFullPath.Buffer );
            RtlStringCbCatNW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, Buffer->FileName, Buffer->FileNameLength );

            WCHAR* wszMatch = nsUtils::EndsWithW( IrpContext->DstFileFullPath.Buffer, L".dwg.exe" );
            if( wszMatch != NULLPTR )
            {
                wszMatch[ 4 ] = L'\0';
                Buffer->FileNameLength -= sizeof( WCHAR ) * 4;
            }

            Buffer = ( decltype( Buffer ) )Add2Ptr( Buffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {

    }
}

void TuneFileNameInformation( IRP_CONTEXT* IrpContext, CTX_INSTANCE_CONTEXT* InstanceContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFILE_NAMES_INFORMATION Buffer = NULLPTR;

    __try
    {
        Status = GetQueryDirectoryBuffer( IrpContext->Data, ( PVOID* )&Buffer );
        if( !NT_SUCCESS( Status ) )
            __leave;

        ULONG Offset = 0;
        ULONG RequiredBufferBytes = 0;

        do
        {
            Offset = Buffer->NextEntryOffset;
            RequiredBufferBytes = IrpContext->SrcFileFullPath.BufferSize + Buffer->FileNameLength + sizeof( WCHAR );
            CheckBufferSize( &IrpContext->DstFileFullPath, RequiredBufferBytes );

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbCatW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, IrpContext->SrcFileFullPath.Buffer );
            RtlStringCbCatNW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, Buffer->FileName, Buffer->FileNameLength );

            WCHAR* wszMatch = nsUtils::EndsWithW( IrpContext->DstFileFullPath.Buffer, L".dwg.exe" );
            if( wszMatch != NULLPTR )
            {
                wszMatch[ 4 ] = L'\0';
                Buffer->FileNameLength -= sizeof( WCHAR ) * 4;
            }

            Buffer = ( decltype( Buffer ) )Add2Ptr( Buffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {

    }
}

void TuneFileIdBothDirectoryInformation( IRP_CONTEXT* IrpContext, CTX_INSTANCE_CONTEXT* InstanceContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFILE_ID_BOTH_DIR_INFORMATION Buffer = NULLPTR;

    __try
    {
        Status = GetQueryDirectoryBuffer( IrpContext->Data, ( PVOID* )&Buffer );
        if( !NT_SUCCESS( Status ) )
            __leave;

        ULONG Offset = 0;
        ULONG RequiredBufferBytes = 0;

        do
        {
            Offset = Buffer->NextEntryOffset;
            RequiredBufferBytes = IrpContext->SrcFileFullPath.BufferSize + Buffer->FileNameLength + sizeof( WCHAR );
            CheckBufferSize( &IrpContext->DstFileFullPath, RequiredBufferBytes );

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbCatW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, IrpContext->SrcFileFullPath.Buffer );
            RtlStringCbCatNW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, Buffer->FileName, Buffer->FileNameLength );

            WCHAR* wszMatch = nsUtils::EndsWithW( IrpContext->DstFileFullPath.Buffer, L".dwg.exe" );
            if( wszMatch != NULLPTR )
            {
                wszMatch[ 4 ] = L'\0';
                Buffer->FileNameLength -= sizeof( WCHAR ) * 4;
            }

            Buffer = ( decltype( Buffer ) )Add2Ptr( Buffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {

    }
}

void TuneFileIdFullDirectoryInformation( IRP_CONTEXT* IrpContext, CTX_INSTANCE_CONTEXT* InstanceContext )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFILE_ID_FULL_DIR_INFORMATION Buffer = NULLPTR;

    __try
    {
        Status = GetQueryDirectoryBuffer( IrpContext->Data, ( PVOID* )&Buffer );
        if( !NT_SUCCESS( Status ) )
            __leave;

        ULONG Offset = 0;
        ULONG RequiredBufferBytes = 0;

        do
        {
            Offset = Buffer->NextEntryOffset;
            RequiredBufferBytes = IrpContext->SrcFileFullPath.BufferSize + Buffer->FileNameLength + sizeof( WCHAR );
            CheckBufferSize( &IrpContext->DstFileFullPath, RequiredBufferBytes );

            RtlZeroMemory( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize );
            RtlStringCbCatW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, IrpContext->SrcFileFullPath.Buffer );
            RtlStringCbCatNW( IrpContext->DstFileFullPath.Buffer, IrpContext->DstFileFullPath.BufferSize, Buffer->FileName, Buffer->FileNameLength );

            WCHAR* wszMatch = nsUtils::EndsWithW( IrpContext->DstFileFullPath.Buffer, L".dwg.exe" );
            if( wszMatch != NULLPTR )
            {
                wszMatch[ 4 ] = L'\0';
                Buffer->FileNameLength -= sizeof( WCHAR ) * 4;
            }

            Buffer = ( decltype( Buffer ) )Add2Ptr( Buffer, Offset );

        } while( Offset != 0 );
    }
    __finally
    {

    }
}

NTSTATUS GetQueryDirectoryBuffer( PFLT_CALLBACK_DATA Data, PVOID* Buffer )
{
    NTSTATUS Status = STATUS_SUCCESS;

    __try
    {
        if( Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        *Buffer = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;

        if( *Buffer != NULLPTR )
            __leave;

        /*!
            based on MSDN

            If both a DirectoryBuffer and MdlAddress buffer are provided, it is recommended that minifilters use the MDL.
            The memory that DirectoryBuffer points to is valid when it is a user mode address being accessed within the context of the calling process, or if it is a kernel mode address.
            If a minifilter changes the value of MdlAddress, then after its post operation callback, Filter Manager will free the MDL currently stored in MdlAddress and restore the previous value of MdlAddress.
        */
        if( Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress == NULL )
        {
            Status = FltLockUserBuffer( Data );
            if( !NT_SUCCESS( Status ) )
            {
                Data->IoStatus.Status = Status;
                Data->IoStatus.Information = 0;
                __leave;
            }
        }

        *Buffer = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress, NormalPagePriority );

        if( *Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }
    }
    __finally
    {
        
    }

    return Status;
}

NTSTATUS CheckBufferSize( TyGenericBuffer<WCHAR>* Buffer, ULONG RequiredSize )
{
    NTSTATUS Status = STATUS_SUCCESS;

    do
    {
        if( Buffer == NULLPTR )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        if( Buffer->Buffer == NULLPTR )
            *Buffer = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
        else if( Buffer->Buffer != NULLPTR )
        {
            if( Buffer->BufferSize < RequiredSize )
            {
                DeallocateBuffer( Buffer );
                *Buffer = AllocateBuffer<WCHAR>( BUFFER_FILENAME, RequiredSize );
            }
        }

        Status = STATUS_SUCCESS;

    } while( false );

    return Status;
}
