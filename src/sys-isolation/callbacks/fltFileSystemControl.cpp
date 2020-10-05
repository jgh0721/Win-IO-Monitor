#include "fltFileSystemControl.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#include "utilities/osInfoMgr.hpp"
#include "utilities/fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreFileSystemControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                             PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;
    IRP_CONTEXT*                                IrpContext = NULLPTR;
    PFLT_CALLBACK_DATA                          NewCallbackData = NULLPTR;
    NTSTATUS                                    Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( CompletionContext );

    FsRtlEnterFileSystem();

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );
        PrintIrpContext( IrpContext );

        auto FsControlCode = Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode;

        switch( Data->Iopb->MinorFunction )
        {
            case IRP_MN_USER_FS_REQUEST: {
                switch( FsControlCode )
                {
                    case FSCTL_MOVE_FILE: {} break;
                    case FSCTL_CREATE_OR_GET_OBJECT_ID: {} break;
                    case FSCTL_READ_FILE_USN_DATA: {} break;
                    case FSCTL_GET_RETRIEVAL_POINTERS: {} break;

                    case FSCTL_REQUEST_OPLOCK:          // Windows 7 ~ 
                    case FSCTL_REQUEST_OPLOCK_LEVEL_1: 
                    case FSCTL_REQUEST_OPLOCK_LEVEL_2: 
                    case FSCTL_REQUEST_BATCH_OPLOCK:
                    case FSCTL_REQUEST_FILTER_OPLOCK:
                    case FSCTL_OPLOCK_BREAK_NOTIFY:
                    case FSCTL_OPLOCK_BREAK_ACK_NO_2: 
                    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
                    case FSCTL_OPBATCH_ACK_CLOSE_PENDING: {

                        ULONG OplockCount = 0;
                        ULONG InputBufferSize = Data->Iopb->Parameters.FileSystemControl.Common.InputBufferLength;
                        ULONG OutputBufferSize = Data->Iopb->Parameters.FileSystemControl.Common.OutputBufferLength;

                        auto InputBuffer = ( nsW32API::REQUEST_OPLOCK_INPUT_BUFFER* )Data->Iopb->Parameters.FileSystemControl.Buffered.SystemBuffer;

                        if( FsControlCode == FSCTL_REQUEST_OPLOCK )
                        {
                            if( (InputBufferSize < sizeof( nsW32API::REQUEST_OPLOCK_INPUT_BUFFER )) ||
                                (OutputBufferSize < sizeof( nsW32API::REQUEST_OPLOCK_OUTPUT_BUFFER )) )
                            {
                                AssignCmnResult( IrpContext, STATUS_BUFFER_TOO_SMALL );
                                AssignCmnResultInfo( IrpContext, 0 );
                                AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
                                __leave;
                            }
                        }

                        if( ( FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1 ) ||
                            ( FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2 ) ||
                            ( FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK ) ||
                            ( FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK ) ||
                            ( ( FsControlCode == FSCTL_REQUEST_OPLOCK ) && 
                              ( FlagOn( InputBuffer->Flags, REQUEST_OPLOCK_INPUT_FLAG_REQUEST ) ) )
                            )
                        {
                            AcquireCmnResource( IrpContext, INST_SHARED );
                            AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

                            if( FsControlCode != FSCTL_REQUEST_OPLOCK_LEVEL_2 )
                                OplockCount = IrpContext->Fcb->OpnCount;
                            else // From MSDN. Specifies the locking state of the file. Set this parameter to a nonzero ULONG value if there are byte-range locks on the file, or zero otherwise.
                                OplockCount = ( ULONG )FsRtlAreThereCurrentFileLocks( &IrpContext->Fcb->FileLock );
                        }
                        else if( ( FsControlCode == FSCTL_OPLOCK_BREAK_ACKNOWLEDGE ) ||
                                 ( FsControlCode == FSCTL_OPLOCK_BREAK_NOTIFY ) ||
                                 ( FsControlCode == FSCTL_OPLOCK_BREAK_ACK_NO_2 ) ||
                                 ( FsControlCode == FSCTL_OPBATCH_ACK_CLOSE_PENDING ) ||
                                 ( ( FsControlCode == FSCTL_REQUEST_OPLOCK ) &&
                                   ( FlagOn( InputBuffer->Flags, REQUEST_OPLOCK_INPUT_FLAG_ACK ) ) )
                                 )
                        {
                            AcquireCmnResource( IrpContext, FCB_MAIN_SHARED );
                        }
                        
                        if( FsControlCode != FSCTL_REQUEST_OPLOCK )
                        {
                            FltStatus = FltOplockFsctrl( &IrpContext->Fcb->FileOplock,
                                                         Data, OplockCount );
                        }
                        else
                        {
                            if( nsUtils::VerifyVersionInfoEx( 6, 2, ">=" ) == true )
                            {
                                // if FILESYSTEM is NTFS, OPLOCK_FSCTRL_FLAG_ALL_KEYS_MATCH specify
                                FltStatus = GlobalFltMgr.FltOplockFsctrlEx( &IrpContext->Fcb->FileOplock,
                                                                            Data, OplockCount, 0 );
                            }
                            else
                            {
                                FltStatus = FltOplockFsctrl( &IrpContext->Fcb->FileOplock,
                                                             Data, OplockCount );
                            }
                        }

                        IrpContext->Fcb->AdvFcbHeader.IsFastIoPossible = CheckIsFastIOPossible( IrpContext->Fcb );
                        AssignCmnResult( IrpContext, STATUS_SUCCESS );

                    } break;
                }
            } break;
        }

        auto Ccb = ( CCB* )FileObject->FsContext2;

        if( Ccb->LowerFileObject == NULLPTR )
        {
            AssignCmnResult( IrpContext, STATUS_FILE_DELETED );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            __leave;
        }

        Status = FltAllocateCallbackData( FltObjects->Instance, IrpContext->Ccb->LowerFileObject, &NewCallbackData );

        if( !NT_SUCCESS( Status ) )
        {
            KdPrint( ( "[WinIOSol] >> EvtID=%09d %s %s Status=0x%08x,%s\n",
                       IrpContext->EvtID, __FUNCTION__
                       , "FltAllocateCallbackData FAILED"
                       , Status, ntkernel_error_category::find_ntstatus( Status )->message
                       ) );

            AssignCmnResult( IrpContext, Status );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            __leave;
        }

        RtlCopyMemory( NewCallbackData->Iopb, Data->Iopb, sizeof( FLT_IO_PARAMETER_BLOCK ) );
        NewCallbackData->Iopb->TargetFileObject = IrpContext->Ccb->LowerFileObject;
        FltPerformSynchronousIo( NewCallbackData );

        AssignCmnResult( IrpContext, NewCallbackData->IoStatus.Status );
        AssignCmnResultInfo( IrpContext, NewCallbackData->IoStatus.Information );
        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }
    __finally
    {
        if( NewCallbackData != NULLPTR )
            FltFreeCallbackData( NewCallbackData );

        if( IrpContext != NULLPTR )
        {
            if( BooleanFlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
                FltStatus = IrpContext->PreFltStatus;
        }

        CloseIrpContext( IrpContext );
    }

    FsRtlExitFileSystem();

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostFileSystemControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                               PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}
