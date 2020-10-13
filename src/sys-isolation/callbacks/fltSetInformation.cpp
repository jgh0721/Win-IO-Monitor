#include "fltSetInformation.hpp"

#include "privateFCBMgr.hpp"
#include "irpContext.hpp"
#include "metadata/Metadata.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                          PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;
    PFLT_CALLBACK_DATA                          NewCallbackData = NULLPTR;

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( IsOwnFileObject( FltObjects->FileObject ) == false )
            __leave;

        if( FLT_IS_FASTIO_OPERATION( Data ) )
        {
            FltStatus = FLT_PREOP_DISALLOW_FASTIO;
            __leave;
        }

        FILE_OBJECT* FileObject = FltObjects->FileObject;
        FCB* Fcb = ( FCB* )FileObject->FsContext;
        IrpContext = CreateIrpContext( Data, FltObjects );
        PrintIrpContext( IrpContext );

        auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

        switch( FileInformationClass )
        {
            case FileAllocationInformation: {
                ProcessSetFileAllocationInformation( IrpContext );
            } break;
            case FileEndOfFileInformation: {
                ProcessSetFileEndOfFileInformation( IrpContext );
            } break;
            case FileValidDataLengthInformation: {
                ProcessSetFileValidDataLengthInformation( IrpContext );
            } break;
            case FilePositionInformation: {
                ProcessSetFilePositionInformation( IrpContext );
            } break;
            //case FileRenameInformation: {
            //    ProcessSetFileRenameInformation( IrpContext );
            //} break;
            //case nsW32API::FileRenameInformationEx: {
            //    ProcessSetFileRenameInformationEx( IrpContext );
            //} break;
            //case FileDispositionInformation: {
            //    ProcessSetFileDispositionInformation( IrpContext );
            //} break;
            //case nsW32API::FileDispositionInformationEx: {
            //    ProcessSetFileDispositionInformationEx( IrpContext );
            //} break;

            default:
            {
                NTSTATUS Status = STATUS_SUCCESS;

                Status = FltAllocateCallbackData( FltObjects->Instance, IrpContext->Ccb->LowerFileObject, &NewCallbackData );

                if( !NT_SUCCESS( Status ) )
                {
                    KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Status=0x%08x,%s\n",
                               IrpContext->EvtID, __FUNCTION__
                               , "FltAllocateCallbackData FAILED"
                               , Status, ntkernel_error_category::find_ntstatus( Status )->message
                               ) );

                    AssignCmnResult( IrpContext, Status );
                    break;
                }

                RtlCopyMemory( NewCallbackData->Iopb, Data->Iopb, sizeof( FLT_IO_PARAMETER_BLOCK ) );
                NewCallbackData->Iopb->TargetFileObject = IrpContext->Ccb->LowerFileObject != NULLPTR ? IrpContext->Ccb->LowerFileObject : Fcb->LowerFileObject;
                FltPerformSynchronousIo( NewCallbackData );

                AssignCmnResult( IrpContext, NewCallbackData->IoStatus.Status );
                AssignCmnResultInfo( IrpContext, NewCallbackData->IoStatus.Information );
            } break;
        }

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

            PrintIrpContext( IrpContext, true );
        }

        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                            PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS ProcessSetFileAllocationInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;

    auto FileObject = IrpContext->FltObjects->FileObject;
    auto Fcb = ( FCB* )FileObject->FsContext;

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    auto ParentOfTarget = IrpContext->Data->Iopb->Parameters.SetFileInformation.ParentOfTarget;

    auto InfoBuffer = IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

    __try
    {
        auto Ret = FltCheckOplock( &IrpContext->Fcb->FileOplock,
                                   IrpContext->Data, NULL, NULL, NULL );

        if( Ret != FLT_PREOP_SUCCESS_WITH_CALLBACK )
        {
            Status = IrpContext->Data->IoStatus.Status;
            AssignCmnResult( IrpContext, Status );
            __leave;
        }

        AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );
        AcquireCmnResource( IrpContext, FCB_PGIO_EXCLUSIVE );

        auto AllocationSize = ( PFILE_ALLOCATION_INFORMATION )InfoBuffer;

        if( AllocationSize->AllocationSize.QuadPart < Fcb->AdvFcbHeader.AllocationSize.QuadPart )
        {
            Fcb->AdvFcbHeader.AllocationSize.QuadPart = ROUND_TO_SIZE( AllocationSize->AllocationSize.QuadPart, IrpContext->InstanceContext->ClusterSize );

            if( Fcb->AdvFcbHeader.AllocationSize.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                Fcb->AdvFcbHeader.FileSize.QuadPart = Fcb->AdvFcbHeader.AllocationSize.QuadPart;
            }

            if( Fcb->AdvFcbHeader.ValidDataLength.QuadPart > Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
            }

            if( CcIsFileCached( FileObject ) )
            {
                CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
            }

            UpdateFileSizeOnMetaData( IrpContext, FileObject, Fcb->AdvFcbHeader.FileSize );

            Status = STATUS_SUCCESS;
            AssignCmnResult( IrpContext, Status );
            AssignCmnResultInfo( IrpContext, sizeof( FILE_ALLOCATION_INFORMATION ) );

            FileObject->Flags |= FO_FILE_MODIFIED;
            SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
        }
        else if( AllocationSize->AllocationSize.QuadPart == Fcb->AdvFcbHeader.AllocationSize.QuadPart )
        {
            ;
        }
        else
        {
            Fcb->AdvFcbHeader.AllocationSize.QuadPart =
                ROUND_TO_SIZE( Fcb->AdvFcbHeader.AllocationSize.QuadPart, IrpContext->InstanceContext->ClusterSize );

            
            if( CcIsFileCached( FileObject ) )
            {
                CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
            }

            Status = STATUS_SUCCESS;
            AssignCmnResult( IrpContext, Status );
            AssignCmnResultInfo( IrpContext, sizeof( FILE_ALLOCATION_INFORMATION ) );
        }
    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessSetFileEndOfFileInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;

    auto FileObject = IrpContext->FltObjects->FileObject;
    auto Fcb = ( FCB* )FileObject->FsContext;
    auto Ccb = ( CCB* )FileObject->FsContext2;

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    auto ParentOfTarget = IrpContext->Data->Iopb->Parameters.SetFileInformation.ParentOfTarget;

    auto InfoBuffer = IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
    BOOLEAN bAdvanceOnly = IrpContext->Data->Iopb->Parameters.SetFileInformation.AdvanceOnly;
    BOOLEAN bCacheMapInitialized = FALSE;
    auto EndOfFile = ( PFILE_END_OF_FILE_INFORMATION )InfoBuffer;

    __try
    {
        auto Ret = FltCheckOplock( &IrpContext->Fcb->FileOplock,
                                   IrpContext->Data, NULL, NULL, NULL );

        if( Ret != FLT_PREOP_SUCCESS_WITH_CALLBACK )
        {
            Status = IrpContext->Data->IoStatus.Status;
            AssignCmnResult( IrpContext, Status );
            __leave;
        }

        AcquireCmnResource( IrpContext, INST_EXCLUSIVE );

        if( FileObject->SectionObjectPointer->DataSectionObject != NULLPTR && 
            FileObject->SectionObjectPointer->SharedCacheMap == NULLPTR && 
            !BooleanFlagOn( IrpContext->Data->Iopb->IrpFlags, IRP_PAGING_IO ) )
        {
            if( FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) )
            {
                Status = STATUS_FILE_CLOSED;
                AssignCmnResult( IrpContext, Status );
                __leave;
            }

            CcInitializeCacheMap( FileObject, (PCC_FILE_SIZES)&Fcb->AdvFcbHeader.AllocationSize,
                                  FALSE, 
                                  &GlobalContext.CacheMgrCallbacks, 
                                  Fcb );

            bCacheMapInitialized = TRUE;
        }

        // LazyWriteCallback 에 의해 호출될 때 AdvanceOnly 가 1 로 설정되어 호출된다 
        if( bAdvanceOnly != FALSE )
        {
            if( EndOfFile->EndOfFile.QuadPart <= Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                Status = STATUS_SUCCESS;
                AssignCmnResult( IrpContext, Status );
                __leave;
            }

            // 지연된 쓰기 호출에서는 파일 크기를 변경할 수 없다
            EndOfFile->EndOfFile.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;

            if( BooleanFlagOn( Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
                EndOfFile->EndOfFile.QuadPart += GetHDRSizeFromMetaData( Fcb->MetaDataInfo );

            Status = FltSetInformationFile( IrpContext->FltObjects->Instance,
                                            Ccb->LowerFileObject != NULLPTR ? Ccb->LowerFileObject : Fcb->LowerFileObject,
                                            EndOfFile, sizeof( FILE_END_OF_FILE_INFORMATION ),
                                            FileEndOfFileInformation );
            if( NT_SUCCESS( Status ) )
            {
                UpdateFileSizeOnMetaData( IrpContext, FileObject, EndOfFile->EndOfFile.QuadPart - GetHDRSizeFromMetaData( Fcb->MetaDataInfo ) );
            }

            AssignCmnResult( IrpContext, Status );
        }
        else
        {
            LARGE_INTEGER HelperFileSize;

            if( EndOfFile->EndOfFile.QuadPart < Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                if( Fcb->SectionObjects.ImageSectionObject )
                {
                    if( !MmCanFileBeTruncated( &Fcb->SectionObjects, &EndOfFile->EndOfFile ) )
                    {
                        Status = STATUS_USER_MAPPED_FILE;
                        KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d, Status=0x%08x,%s\n",
                                   IrpContext->EvtID, __FUNCTION__, "MmCanFileBeTruncated FAILED", __LINE__
                                   , Status, ntkernel_error_category::find_ntstatus( Status )->message ) );

                        AssignCmnResult( IrpContext, Status );
                        __leave;
                    }

                    AcquireCmnResource( IrpContext, FCB_PGIO_EXCLUSIVE );
                }

                Fcb->AdvFcbHeader.FileSize.QuadPart = EndOfFile->EndOfFile.QuadPart;
                Fcb->AdvFcbHeader.AllocationSize.QuadPart = ROUND_TO_SIZE( EndOfFile->EndOfFile.QuadPart, Fcb->InstanceContext->ClusterSize );
                if( Fcb->AdvFcbHeader.FileSize.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    Fcb->AdvFcbHeader.ValidDataLength.QuadPart = Fcb->AdvFcbHeader.FileSize.QuadPart;
                }

                HelperFileSize.QuadPart = EndOfFile->EndOfFile.QuadPart;

                if( BooleanFlagOn( Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
                    HelperFileSize.QuadPart += GetHDRSizeFromMetaData( Fcb->MetaDataInfo );

                Status = FltSetInformationFile( IrpContext->FltObjects->Instance, 
                                                Ccb->LowerFileObject != NULLPTR ? Ccb->LowerFileObject : Fcb->LowerFileObject,
                                                &HelperFileSize,
                                                sizeof( FILE_END_OF_FILE_INFORMATION ),
                                                FileEndOfFileInformation );

                if( !NT_SUCCESS( Status ) )
                {
                    KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d, Status=0x%08x,%s\n",
                               IrpContext->EvtID, __FUNCTION__, "MmCanFileBeTruncated FAILED", __LINE__
                               , Status, ntkernel_error_category::find_ntstatus( Status )->message ) );

                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                UpdateFileSizeOnMetaData( IrpContext, FileObject, EndOfFile->EndOfFile );

                if( CcIsFileCached( FileObject ) )
                {
                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                }

                Status = STATUS_SUCCESS;
                AssignCmnResult( IrpContext, Status );
                AssignCmnResultInfo( IrpContext, sizeof( FILE_END_OF_FILE_INFORMATION ) );

                FileObject->Flags |= FO_FILE_MODIFIED;
            }
            else if( EndOfFile->EndOfFile.QuadPart == Fcb->AdvFcbHeader.FileSize.QuadPart )
            {
                ;
            }
            else
            {
                Fcb->AdvFcbHeader.FileSize.QuadPart = EndOfFile->EndOfFile.QuadPart;
                Fcb->AdvFcbHeader.ValidDataLength.QuadPart = EndOfFile->EndOfFile.QuadPart;
                Fcb->AdvFcbHeader.AllocationSize.QuadPart = ROUND_TO_SIZE( EndOfFile->EndOfFile.QuadPart, Fcb->InstanceContext->ClusterSize );

                HelperFileSize.QuadPart = EndOfFile->EndOfFile.QuadPart;

                Status = FltSetInformationFile( IrpContext->FltObjects->Instance, 
                                                Ccb->LowerFileObject != NULLPTR ? Ccb->LowerFileObject : Fcb->LowerFileObject,
                                                &HelperFileSize,
                                                sizeof( FILE_END_OF_FILE_INFORMATION ),
                                                FileEndOfFileInformation );

                if( !NT_SUCCESS( Status ) )
                {
                    KdPrint( ( "[WinIOSol] EvtID=%09d %s %s Line=%d, Status=0x%08x,%s\n",
                               IrpContext->EvtID, __FUNCTION__, "MmCanFileBeTruncated FAILED", __LINE__
                               , Status, ntkernel_error_category::find_ntstatus( Status )->message ) );

                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                if( CcIsFileCached( FileObject ) )
                {
                    CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )( &Fcb->AdvFcbHeader.AllocationSize ) );
                }

                UpdateFileSizeOnMetaData( IrpContext, FileObject, EndOfFile->EndOfFile );

                Status = STATUS_SUCCESS;
                AssignCmnResult( IrpContext, Status );
                AssignCmnResultInfo( IrpContext, 0 );

                FileObject->Flags |= FO_FILE_MODIFIED;
            }

            SetFlag( Fcb->Flags, FCB_STATE_FILE_MODIFIED );
        } // if bAdvanceOnly
    }
    __finally
    {
        if( bCacheMapInitialized != FALSE )
            CcUninitializeCacheMap( FileObject, NULL, NULL );
    }

    return Status;
}

NTSTATUS ProcessSetFileValidDataLengthInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;

    auto FileObject = IrpContext->FltObjects->FileObject;
    auto Fcb = ( FCB* )FileObject->FsContext;
    auto Ccb = ( CCB* )FileObject->FsContext2;

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    auto ParentOfTarget = IrpContext->Data->Iopb->Parameters.SetFileInformation.ParentOfTarget;

    auto InfoBuffer = IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
    BOOLEAN bAdvanceOnly = IrpContext->Data->Iopb->Parameters.SetFileInformation.AdvanceOnly;
    BOOLEAN bCacheMapInitialized = FALSE;
    auto VDLOfFile = ( PFILE_VALID_DATA_LENGTH_INFORMATION )InfoBuffer;

    __try
    {
        auto Ret = FltCheckOplock( &IrpContext->Fcb->FileOplock,
                                   IrpContext->Data, NULL, NULL, NULL );

        if( Ret != FLT_PREOP_SUCCESS_WITH_CALLBACK )
        {
            Status = IrpContext->Data->IoStatus.Status;
            AssignCmnResult( IrpContext, Status );
            __leave;
        }

        if( FileObject->SectionObjectPointer->DataSectionObject != NULLPTR &&
            FileObject->SectionObjectPointer->SharedCacheMap == NULLPTR &&
            !BooleanFlagOn( IrpContext->Data->Iopb->IrpFlags, IRP_PAGING_IO ) )
        {
            if( FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) )
            {
                Status = STATUS_FILE_CLOSED;
                AssignCmnResult( IrpContext, Status );
                __leave;
            }

            CcInitializeCacheMap( FileObject, ( PCC_FILE_SIZES )&Fcb->AdvFcbHeader.AllocationSize,
                                  FALSE,
                                  &GlobalContext.CacheMgrCallbacks,
                                  Fcb );

            bCacheMapInitialized = TRUE;
        }

        if( bAdvanceOnly != FALSE )
        {
            if( VDLOfFile->ValidDataLength.QuadPart <= Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                Status = STATUS_SUCCESS;
                AssignCmnResult( IrpContext, Status );
                __leave;
            }

            // 지연된 쓰기 호출에서는 파일 크기를 변경할 수 없다
            VDLOfFile->ValidDataLength.QuadPart = Fcb->AdvFcbHeader.ValidDataLength.QuadPart;

            if( BooleanFlagOn( Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
                VDLOfFile->ValidDataLength.QuadPart += GetHDRSizeFromMetaData( Fcb->MetaDataInfo );

            Status = FltSetInformationFile( IrpContext->FltObjects->Instance,
                                            Ccb->LowerFileObject != NULLPTR ? Ccb->LowerFileObject : Fcb->LowerFileObject,
                                            VDLOfFile, sizeof( FILE_VALID_DATA_LENGTH_INFORMATION ),
                                            FileValidDataLengthInformation );

            AssignCmnResult( IrpContext, Status );
        }
        else
        {
            if( VDLOfFile->ValidDataLength.QuadPart < Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
            {
                if( !MmCanFileBeTruncated( FileObject->SectionObjectPointer, &VDLOfFile->ValidDataLength ) )
                {
                    Status = STATUS_USER_MAPPED_FILE;
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                AcquireCmnResource( IrpContext, FCB_PGIO_EXCLUSIVE );
            }

            auto Current = Fcb->AdvFcbHeader.ValidDataLength;
            Fcb->AdvFcbHeader.ValidDataLength.QuadPart = VDLOfFile->ValidDataLength.QuadPart;

            if( BooleanFlagOn( Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
                VDLOfFile->ValidDataLength.QuadPart += GetHDRSizeFromMetaData( Fcb->MetaDataInfo );

            Status = FltSetInformationFile( IrpContext->FltObjects->Instance,
                                            Ccb->LowerFileObject != NULLPTR ? Ccb->LowerFileObject : Fcb->LowerFileObject,
                                            VDLOfFile, sizeof( FILE_VALID_DATA_LENGTH_INFORMATION ),
                                            FileValidDataLengthInformation );

            if( !NT_SUCCESS( Status ))
            {
                Fcb->AdvFcbHeader.ValidDataLength = Current;
                AssignCmnResult( IrpContext, Status );
                __leave;
            }

            if( CcIsFileCached( FileObject ) )
                CcSetFileSizes( FileObject, ( PCC_FILE_SIZES )&Fcb->AdvFcbHeader.AllocationSize );

            FileObject->Flags |= FO_FILE_MODIFIED;

            Status = STATUS_SUCCESS;
            AssignCmnResult( IrpContext, Status );

        } // if bAdvanceOnly
    }
    __finally
    {
        if( bCacheMapInitialized != FALSE )
            CcUninitializeCacheMap( FileObject, NULL, NULL );
    }

    return Status;
}

NTSTATUS ProcessSetFilePositionInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;

    auto FileObject = IrpContext->FltObjects->FileObject;
    FileObject = ( ( CCB* )FileObject->FsContext2 )->LowerFileObject;

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    auto ParentOfTarget = IrpContext->Data->Iopb->Parameters.SetFileInformation.ParentOfTarget;

    auto InfoBuffer = IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

    __try
    {
        Status = FltSetInformationFile( IrpContext->FltObjects->Instance,
                                        FileObject,
                                        InfoBuffer, Length, (FILE_INFORMATION_CLASS)FileInformationClass );

        AssignCmnResult( IrpContext, Status );

        if( NT_SUCCESS( Status ) )
        {
            FileObject->CurrentByteOffset = ( ( PFILE_POSITION_INFORMATION )InfoBuffer )->CurrentByteOffset;
        }
    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessSetFileRenameInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;

    auto FileObject = IrpContext->FltObjects->FileObject;
    auto Fcb = ( FCB* )FileObject->FsContext;

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    auto ParentOfTarget = IrpContext->Data->Iopb->Parameters.SetFileInformation.ParentOfTarget;

    auto InfoBuffer = IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

    __try
    {
        AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessSetFileRenameInformationEx( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;

    auto FileObject = IrpContext->FltObjects->FileObject;
    auto Fcb = ( FCB* )FileObject->FsContext;

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    auto ParentOfTarget = IrpContext->Data->Iopb->Parameters.SetFileInformation.ParentOfTarget;

    auto InfoBuffer = IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

    __try
    {
        AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessSetFileDispositionInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;

    auto FileObject = IrpContext->FltObjects->FileObject;
    auto Fcb = ( FCB* )FileObject->FsContext;

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    auto ParentOfTarget = IrpContext->Data->Iopb->Parameters.SetFileInformation.ParentOfTarget;

    auto InfoBuffer = IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

    __try
    {
        AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessSetFileDispositionInformationEx( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;

    auto FileObject = IrpContext->FltObjects->FileObject;
    auto Fcb = ( FCB* )FileObject->FsContext;

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    auto ParentOfTarget = IrpContext->Data->Iopb->Parameters.SetFileInformation.ParentOfTarget;

    auto InfoBuffer = IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

    __try
    {
        AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

    }
    __finally
    {

    }

    return Status;
}
