#include "fltSetInformation.hpp"

#include "utilities/bufferMgr.hpp"
#include "privateFCBMgr.hpp"
#include "irpContext.hpp"
#include "metadata/Metadata.hpp"
#include "communication/Communication.hpp"

#include "fltCmnLibs.hpp"
#include "utilities/fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                          PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    IRP_CONTEXT*                                IrpContext = NULLPTR;
    nsW32API::FILE_INFORMATION_CLASS            FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( FLT_IS_FASTIO_OPERATION( Data ) )
            return FLT_PREOP_DISALLOW_FASTIO;

        auto State = CommonPreSetInformation( Data, FltObjects, CompletionContext, &IrpContext );
        if( FlagOn( State, COMPLETE_INTERNAL_ERROR ) || FlagOn( State, COMPLETE_BYPASS_REQUEST ) || FlagOn( State, COMPLETE_FORWARD_POST_PROCESS ) )
            __leave;
        
        PrintIrpContext( IrpContext );
        
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
            case FileRenameInformation: {
                ProcessSetFileUnifiedRenameInformation( IrpContext );
            } break;
            case nsW32API::FileRenameInformationEx: {
                ProcessSetFileUnifiedRenameInformation( IrpContext );
            } break;
            case FileDispositionInformation: {
                ProcessSetFileDispositionInformation( IrpContext );
            } break;
            case nsW32API::FileDispositionInformationEx: {
                ProcessSetFileDispositionInformationEx( IrpContext );
            } break;

            default: {
                ASSERT( IrpContext->IsOwnObject == true );
                ProcessSetFileInformation( IrpContext );
                AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            } break;
        }
    }
    __finally
    {
        if( IrpContext != NULLPTR )
        {
            if( IrpContext->IsOwnObject == true )
                PrintIrpContext( IrpContext, true );
        }

        FltStatus = CommonPostProcess( Data, FltObjects, CompletionContext, IrpContext, FltStatus );
    }

    return FltStatus;
}

ULONG CommonPreSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext,
                               PIRP_CONTEXT* IrpContext )
{
    nsW32API::FILE_INFORMATION_CLASS            FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

    if( IsOwnFileObject( FltObjects->FileObject ) == false )
    {
        if( ( FileInformationClass != FileRenameInformation ) && ( FileInformationClass != nsW32API::FileRenameInformationEx ) &&
            ( FileInformationClass != FileDispositionInformation ) && ( FileInformationClass != nsW32API::FileDispositionInformationEx ) )
        {
            return COMPLETE_BYPASS_REQUEST;
        }
    }

    *IrpContext = CreateIrpContext( Data, FltObjects );
    auto Context = *IrpContext;
    
    if( Context == NULLPTR )
    {
        return COMPLETE_INTERNAL_ERROR;
    }

    if( Context->IsOwnObject == false )
    {
        if( Context->StreamContext == NULLPTR )
        {
            SetFlag( Context->CompleteStatus, COMPLETE_BYPASS_REQUEST );
            AssignCmnFltResult( Context, FLT_PREOP_SUCCESS_NO_CALLBACK );
            return Context->CompleteStatus;
        }

        if( ( FileInformationClass != FileRenameInformation ) && ( FileInformationClass != nsW32API::FileRenameInformationEx ) && 
            ( FileInformationClass != FileDispositionInformation ) && ( FileInformationClass != nsW32API::FileDispositionInformationEx ) )
        {
            SetFlag( Context->CompleteStatus, COMPLETE_BYPASS_REQUEST );
            AssignCmnFltResult( Context, FLT_PREOP_SUCCESS_NO_CALLBACK );
            return Context->CompleteStatus;
        }
    }

    return 0;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostSetInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                            PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    IRP_CONTEXT*                                IrpContext = (IRP_CONTEXT*)CompletionContext;
    PCTX_STREAM_CONTEXT                         StreamContext = NULLPTR;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    do
    {
        if( IrpContext == NULLPTR )
            break;

        IrpContext->Data = Data;
        IrpContext->FltObjects = FltObjects;
        ClearFlag( IrpContext->CompleteStatus, COMPLETE_FORWARD_POST_PROCESS );
        ClearFlag( IrpContext->CompleteStatus, COMPLETE_CRTE_STREAM_CONTEXT );
        StreamContext = IrpContext->StreamContext;

        if( !NT_SUCCESS( Data->IoStatus.Status ) )
            break;

        if( ( Data->Iopb->Parameters.SetFileInformation.FileInformationClass != FileDispositionInformation ) &&
            ( Data->Iopb->Parameters.SetFileInformation.FileInformationClass != nsW32API::FileDispositionInformationEx ) )
            break;

        //
        //  No synchronization is needed to set the SetDisp field,
        //  because in case of races, the NumOps field will be perpetually
        //  positive, and it being positive is already an indication this
        //  file is a delete candidate, so it will be checked at post-
        //  -cleanup regardless of the value of SetDisp.
        //

        //
        //  Using FileDispositinInformationEx -
        //    FILE_DISPOSITION_ON_CLOSE controls delete on close
        //    or set disposition behavior. It uses FILE_DISPOSITION_INFORMATION_EX structure.
        //    FILE_DISPOSITION_ON_CLOSE is set - Set or clear DeleteOnClose
        //    depending on FILE_DISPOSITION_DELETE flag.
        //    FILE_DISPOSITION_ON_CLOSE is NOT set - Set or clear disposition information
        //    depending on the flag FILE_DISPOSITION_DELETE.
        //
        //
        //   Using FileDispositionInformation -
        //    Controls only set disposition information behavior. It uses FILE_DISPOSITION_INFORMATION structure.
        //

        if( Data->Iopb->Parameters.SetFileInformation.FileInformationClass == nsW32API::FileDispositionInformationEx )
        {
            ULONG flags = ( ( nsW32API::PFILE_DISPOSITION_INFORMATION_EX )Data->Iopb->Parameters.SetFileInformation.InfoBuffer )->Flags;

            if( FlagOn( flags, FILE_DISPOSITION_ON_CLOSE ) )
            {
                StreamContext->DeleteOnClose = BooleanFlagOn( flags, FILE_DISPOSITION_DELETE );
            }
            else
            {
                StreamContext->SetDisp = BooleanFlagOn( flags, FILE_DISPOSITION_DELETE );
            }
        }
        else
        {
            StreamContext->SetDisp = ( ( PFILE_DISPOSITION_INFORMATION )Data->Iopb->Parameters.SetFileInformation.InfoBuffer )->DeleteFile;
        }

    } while( false );

    if( ( Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation ) ||
        ( Data->Iopb->Parameters.SetFileInformation.FileInformationClass == nsW32API::FileDispositionInformationEx ) )
    {
        //
        //  Now that the operation is over, decrement NumOps.
        //
        if( StreamContext != NULLPTR )
            InterlockedDecrement( &StreamContext->NumOps );
    }

    CommonPostProcess( Data, FltObjects, &CompletionContext, IrpContext, ( FLT_PREOP_CALLBACK_STATUS )0 );
    return FltStatus;
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS ProcessSetFileAllocationInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ASSERT( IrpContext->IsOwnObject == true );

    auto FileObject = IrpContext->FltObjects->FileObject;
    auto Fcb = ( FCB* )FileObject->FsContext;

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
        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }

    return Status;
}

NTSTATUS ProcessSetFileEndOfFileInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ASSERT( IrpContext->IsOwnObject == true );

    auto FileObject = IrpContext->FltObjects->FileObject;
    auto Fcb = ( FCB* )FileObject->FsContext;
    auto Ccb = ( CCB* )FileObject->FsContext2;

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

        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }

    return Status;
}

NTSTATUS ProcessSetFileValidDataLengthInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ASSERT( IrpContext->IsOwnObject == true );

    auto FileObject = IrpContext->FltObjects->FileObject;
    auto Fcb = ( FCB* )FileObject->FsContext;
    auto Ccb = ( CCB* )FileObject->FsContext2;

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

        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }

    return Status;
}

NTSTATUS ProcessSetFilePositionInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ASSERT( IrpContext->IsOwnObject == true );

    auto FileObject = IrpContext->FltObjects->FileObject;
    FileObject = ( ( CCB* )FileObject->FsContext2 )->LowerFileObject;

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

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
        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }

    return Status;
}

NTSTATUS ProcessSetFileUnifiedRenameInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS                        Status = STATUS_SUCCESS;
    FILE_OBJECT*                    FileObject = IrpContext->FltObjects->FileObject;

    METADATA_DRIVER*                MetaDataInfo = NULLPTR;
    TyGenericBuffer<WCHAR>          FileFullPathTest;
    PVOID                           InfoBuffer = NULLPTR;

    __try
    {
        if( IrpContext->ProcessFilter != NULLPTR )
        {
            // Approved Process

            if( IrpContext->IsOwnObject == true )
            {
                // Own File

                AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
                AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

                FileObject = IrpContext->Ccb->LowerFileObject != NULLPTR ? IrpContext->Ccb->LowerFileObject : IrpContext->Fcb->LowerFileObject;

                // Non-Pretended File
                if( IrpContext->Fcb->MetaDataInfo == NULLPTR ||
                    IrpContext->Fcb->MetaDataInfo->MetaData.Type != METADATA_STB_TYPE )
                {
                    Status = ProcessSetFileInformation( IrpContext );

                    if( IrpContext->IsConcerned == true && NT_SUCCESS( Status ) )
                    {
                        NotifyEventFileRenameTo( IrpContext );
                        PostProcessFileRename( IrpContext, &IrpContext->DstFileFullPath );
                    }

                    __leave;
                }

                auto Fcb = IrpContext->Fcb;
                auto CtxRenLinkContext = RetrieveRenameLinkContext( IrpContext->Data, IrpContext->FltObjects );

                FileFullPathTest = AllocateBuffer<WCHAR>( BUFFER_FILENAME, IrpContext->SrcFileFullPath.BufferSize + CONTAINOR_SUFFIX_MAX * sizeof( WCHAR ) );

                // if dst filename equals pretended filename + suffix then reject request
                RtlStringCbPrintfW( FileFullPathTest.Buffer, FileFullPathTest.BufferSize, L"%s%s", IrpContext->SrcFileName, Fcb->MetaDataInfo->MetaData.ContainorSuffix );
                
                if( nsUtils::stricmp( FileFullPathTest.Buffer, IrpContext->DstFileName ) == 0 )
                {
                    Status = STATUS_ACCESS_DENIED;
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                auto SuffixCnt = nsUtils::strlength( Fcb->MetaDataInfo->MetaData.ContainorSuffix );
                auto FileNameLength = ( IrpContext->InstanceContext->DeviceNameCch + nsUtils::strlength( IrpContext->DstFileFullPathWOVolume ) + SuffixCnt + 1 ) * sizeof( WCHAR );
                auto RequiredSize = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length + FileNameLength;

                InfoBuffer = ExAllocatePool( NonPagedPool, RequiredSize );
                if( InfoBuffer == NULLPTR )
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                RtlZeroMemory( InfoBuffer, RequiredSize );

                auto BaseOffset = 0;
                wchar_t* FileName = NULLPTR;

                switch( CtxRenLinkContext.FileInformationClass )
                {
                    case nsW32API::FileRenameInformation: {
                        ( ( FILE_RENAME_INFORMATION* )InfoBuffer )->ReplaceIfExists = FlagOn( CtxRenLinkContext.Flags, FILE_RENAME_REPLACE_IF_EXISTS );
                        ( ( FILE_RENAME_INFORMATION* )InfoBuffer )->RootDirectory = CtxRenLinkContext.RootDirectory;
                        ( ( FILE_RENAME_INFORMATION* )InfoBuffer )->FileNameLength = FileNameLength;
                        ( ( FILE_RENAME_INFORMATION* )InfoBuffer )->FileNameLength -= sizeof( WCHAR );

                        BaseOffset = FIELD_OFFSET( FILE_RENAME_INFORMATION, FileName );
                        FileName = ( WCHAR* )Add2Ptr( InfoBuffer, BaseOffset );

                    } break;
                    case nsW32API::FileRenameInformationEx: {
                        ( ( nsW32API::FILE_RENAME_INFORMATION_EX* )InfoBuffer )->Flags = CtxRenLinkContext.Flags;
                        ( ( nsW32API::FILE_RENAME_INFORMATION_EX* )InfoBuffer )->RootDirectory = CtxRenLinkContext.RootDirectory;
                        ( ( nsW32API::FILE_RENAME_INFORMATION_EX* )InfoBuffer )->FileNameLength = FileNameLength;
                        ( ( FILE_RENAME_INFORMATION* )InfoBuffer )->FileNameLength -= sizeof( WCHAR );

                        BaseOffset = FIELD_OFFSET( nsW32API::FILE_RENAME_INFORMATION_EX, FileName );
                        FileName = ( WCHAR* )Add2Ptr( InfoBuffer, BaseOffset );

                    } break;
                }

                RtlStringCbCatW( FileName, FileNameLength, IrpContext->InstanceContext->DeviceNameBuffer );
                RtlStringCbCatW( FileName, FileNameLength, IrpContext->DstFileFullPathWOVolume );
                RtlStringCbCatW( FileName, FileNameLength, Fcb->MetaDataInfo->MetaData.ContainorSuffix );

                Status = FltSetInformationFile( IrpContext->FltObjects->Instance, FileObject, InfoBuffer, RequiredSize, 
                                                ( ::FILE_INFORMATION_CLASS )CtxRenLinkContext.FileInformationClass );

                AssignCmnResult( IrpContext, Status );

                if( !NT_SUCCESS( Status ) )
                {
                    KdPrint( ( "[WinIOSol] >> EvtID=%09d %s Status=0x%08x,%s\n"
                               , IrpContext->EvtID, __FUNCTION__
                               , ntkernel_error_category::find_ntstatus( Status )->message
                               ) );
                }

                if( IrpContext->IsConcerned == true && NT_SUCCESS( Status ) )
                {
                    // NOTE: 변경된 이름으로 통지를 할 수 있도록 변수를 조절한다
                    auto OriDstFileFullPath = IrpContext->DstFileFullPath;
                    IrpContext->DstFileFullPath.Buffer = FileName;
                    IrpContext->DstFileFullPath.BufferSize = FileNameLength + sizeof( WCHAR );
                    NotifyEventFileRenameTo( IrpContext );
                    IrpContext->DstFileFullPath = OriDstFileFullPath;
                    PostProcessFileRename( IrpContext, &IrpContext->DstFileFullPath );
                }
            }
            else
            {
                // Non-Own File

                Status = ProcessSetFileInformation( IrpContext );

                if( IrpContext->IsConcerned == true && NT_SUCCESS( Status ) )
                {
                    NotifyEventFileRenameTo( IrpContext );
                    PostProcessFileRename( IrpContext, &IrpContext->DstFileFullPath );
                }
            }
        }
        else
        {
            // Non-Approved Process

            if( IrpContext->IsOwnObject == true )
            {
                // Own File

                // NOTE: 승인받은 프로세스만 IsOwnObject 가 true 가 될 수 있다. 
                ASSERT( false );
            }
            else
            {
                // Non-Own File
                // NOTE: 승인받지 않은 프로세스( 탐색기 등 ) 에서 암호화된 파일에 대해 이름변경을 시도할 수 있다

                MetaDataInfo = AllocateMetaDataInfo();

                if( IsOwnFile( IrpContext, IrpContext->SrcFileFullPathWOVolume, MetaDataInfo ) == false )
                {
                    Status = ProcessSetFileInformation( IrpContext );
                    if( NT_SUCCESS( Status ) )
                        PostProcessFileRename( IrpContext, &IrpContext->DstFileFullPath );

                    __leave;
                }

                if( nsUtils::EndsWithW( IrpContext->DstFileName, L".exe" ) == NULLPTR )
                {
                    Status = STATUS_ACCESS_DENIED;
                    AssignCmnResult( IrpContext, Status );
                    __leave;
                }

                Status = ProcessSetFileInformation( IrpContext );

                if( IrpContext->IsConcerned == true && NT_SUCCESS( Status ) )
                {
                    NotifyEventFileRenameTo( IrpContext );
                    PostProcessFileRename( IrpContext, &IrpContext->DstFileFullPath );
                }
            }
        }
    }
    __finally
    {
        if( InfoBuffer != NULLPTR )
            ExFreePool( InfoBuffer );
        DeallocateBuffer( &FileFullPathTest );
        UninitializeMetaDataInfo( MetaDataInfo );

        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }

    return Status;
}

NTSTATUS ProcessSetFileDispositionInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS Status = STATUS_SUCCESS;

    auto FileObject = IrpContext->FltObjects->FileObject;

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    auto ParentOfTarget = IrpContext->Data->Iopb->Parameters.SetFileInformation.ParentOfTarget;

    auto InfoBuffer = IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

    __try
    {
        if( IrpContext->IsOwnObject == true )
        {
            auto Fcb = ( FCB* )FileObject->FsContext;

            AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
            AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

            Status = ProcessSetFileInformation( IrpContext );

            if( NT_SUCCESS( Status ) )
            {
                SetFlag( Fcb->Flags, FCB_STATE_CHECK_DELETE_PENDING );
            }

            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
        }
        else
        {
            //
            //  Race detection logic. The NumOps field in the StreamContext
            //  counts the number of in-flight changes to delete disposition
            //  on the stream.
            //
            //  If there's already some operations in flight, don't bother
            //  doing postop. Since there will be no postop, this value won't
            //  be decremented, staying forever 2 or more, which is one of
            //  the conditions for checking deletion at post-cleanup.
            //
            BOOLEAN Race = ( InterlockedIncrement( &IrpContext->StreamContext->NumOps ) > 1 );

            if( !Race )
            {
                AssignCmnResult( IrpContext, COMPLETE_FORWARD_POST_PROCESS );
                AssignCmnFltResult( IrpContext, FLT_PREOP_SYNCHRONIZE );
            }

            Status = STATUS_SUCCESS;
        }
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

    auto Length = IrpContext->Data->Iopb->Parameters.SetFileInformation.Length;
    auto FileInformationClass = ( nsW32API::FILE_INFORMATION_CLASS )IrpContext->Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    auto ParentOfTarget = IrpContext->Data->Iopb->Parameters.SetFileInformation.ParentOfTarget;

    __try
    {
        if( IrpContext->IsOwnObject == true )
        {
            auto Fcb = ( FCB* )FileObject->FsContext;

            AcquireCmnResource( IrpContext, INST_EXCLUSIVE );
            AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );

            Status = ProcessSetFileInformation( IrpContext );

            if( NT_SUCCESS( Status ) )
            {
                auto InfoBuffer = (nsW32API::FILE_DISPOSITION_INFORMATION_EX*)IrpContext->Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

                if( FlagOn( InfoBuffer->Flags, FILE_DISPOSITION_POSIX_SEMANTICS ) )
                    SetFlag( Fcb->Flags, FCB_STATE_POSIX_SEMANTICS );

                SetFlag( Fcb->Flags, FCB_STATE_CHECK_DELETE_PENDING );
            }

            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
        }
        else
        {
            //
            //  Race detection logic. The NumOps field in the StreamContext
            //  counts the number of in-flight changes to delete disposition
            //  on the stream.
            //
            //  If there's already some operations in flight, don't bother
            //  doing postop. Since there will be no postop, this value won't
            //  be decremented, staying forever 2 or more, which is one of
            //  the conditions for checking deletion at post-cleanup.
            //
            BOOLEAN Race = ( InterlockedIncrement( &IrpContext->StreamContext->NumOps ) > 1 );

            if( !Race )
            {
                AssignCmnResult( IrpContext, COMPLETE_FORWARD_POST_PROCESS );
                AssignCmnFltResult( IrpContext, FLT_PREOP_SYNCHRONIZE );
            }

            Status = STATUS_SUCCESS;
        }
    }
    __finally
    {

    }

    return Status;
}

NTSTATUS ProcessSetFileInformation( IRP_CONTEXT* IrpContext )
{
    NTSTATUS                                    Status = STATUS_SUCCESS;
    PFLT_CALLBACK_DATA                          NewCallbackData = NULLPTR;
    const auto&                                 Data = IrpContext->Data;
    PFILE_OBJECT                                FileObject = NULLPTR;

    do
    {
        if( IrpContext->IsOwnObject == true )
            FileObject = IrpContext->Ccb->LowerFileObject != NULLPTR ? IrpContext->Ccb->LowerFileObject : IrpContext->Fcb->LowerFileObject;
        else
            FileObject = IrpContext->FltObjects->FileObject;

        Status = FltAllocateCallbackData( IrpContext->FltObjects->Instance, FileObject, &NewCallbackData );

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
        ClearFlag( NewCallbackData->Iopb->IrpFlags, IRP_PAGING_IO );
        NewCallbackData->Iopb->TargetFileObject = FileObject;

        if( IsOwnFileObject( NewCallbackData->Iopb->Parameters.SetFileInformation.ParentOfTarget ) == true )
        {
            CCB* Ccb = (CCB*)NewCallbackData->Iopb->Parameters.SetFileInformation.ParentOfTarget->FsContext2;
            if( Ccb != NULLPTR )
                NewCallbackData->Iopb->Parameters.SetFileInformation.ParentOfTarget = Ccb->LowerFileObject;
            else
            {
                Status = STATUS_ACCESS_DENIED;
                AssignCmnResult( IrpContext, Status );
                break;
            }
        }

        FltPerformSynchronousIo( NewCallbackData );

        Status = NewCallbackData->IoStatus.Status;
        AssignCmnResult( IrpContext, NewCallbackData->IoStatus.Status );

    } while( false );

    if( NewCallbackData != NULLPTR )
        FltFreeCallbackData( NewCallbackData );

    return Status;
}

void PostProcessFileRename( IRP_CONTEXT* IrpContext, TyGenericBuffer<WCHAR>* DstFileFullPath )
{
    if( IrpContext == NULLPTR || DstFileFullPath == NULLPTR )
        return;

    if( IrpContext->IsOwnObject == true )
    {
        if( IrpContext->Fcb != NULLPTR )
        {
            DeallocateBuffer( &IrpContext->Fcb->FileFullPath );
            IrpContext->Fcb->FileFullPathWOVolume = NULLPTR;
            IrpContext->Fcb->FileName = NULLPTR;

            if( IrpContext->Fcb->MetaDataInfo != NULLPTR && IrpContext->Fcb->MetaDataInfo->MetaData.Type == METADATA_STB_TYPE )
            {
                DeallocateBuffer( &IrpContext->Fcb->PretendFileFullPath );
                IrpContext->Fcb->PretendFileFullPathWOVolume = NULLPTR;
                IrpContext->Fcb->PretendFileName = NULLPTR;

                IrpContext->Fcb->PretendFileFullPath = CloneBuffer( DstFileFullPath );
                IrpContext->Fcb->PretendFileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &IrpContext->Fcb->PretendFileFullPath );
                IrpContext->Fcb->PretendFileName = nsUtils::ReverseFindW( IrpContext->Fcb->PretendFileFullPath.Buffer, L'\\' );

                IrpContext->Fcb->FileFullPath = AllocateBuffer<WCHAR>( BUFFER_FILENAME, DstFileFullPath->BufferSize + CONTAINOR_SUFFIX_MAX * sizeof( WCHAR ) + sizeof( WCHAR ) );
                RtlStringCbCopyNW( IrpContext->Fcb->FileFullPath.Buffer, IrpContext->Fcb->FileFullPath.BufferSize,
                                   DstFileFullPath->Buffer, DstFileFullPath->BufferSize );
                RtlStringCbCatW( IrpContext->Fcb->FileFullPath.Buffer, IrpContext->Fcb->FileFullPath.BufferSize,
                                 IrpContext->Fcb->MetaDataInfo->MetaData.ContainorSuffix );
                IrpContext->Fcb->FileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &IrpContext->Fcb->FileFullPath );
                IrpContext->Fcb->FileName = nsUtils::ReverseFindW( IrpContext->Fcb->FileFullPath.Buffer, L'\\' );
            }
            else
            {
                ASSERT( IrpContext->Fcb->PretendFileFullPath.Buffer == NULLPTR );

                IrpContext->Fcb->FileFullPath = CloneBuffer( DstFileFullPath );
                IrpContext->Fcb->FileFullPathWOVolume = ExtractFileFullPathWOVolume( IrpContext->InstanceContext, &IrpContext->Fcb->FileFullPath );
                IrpContext->Fcb->FileName = nsUtils::ReverseFindW( IrpContext->Fcb->FileFullPath.Buffer, L'\\' );
            }
        }

        if( IrpContext->Ccb != NULLPTR )
        {
            DeallocateBuffer( &IrpContext->Ccb->SrcFileFullPath );
            IrpContext->Ccb->SrcFileFullPath = CloneBuffer( DstFileFullPath );
        }
    }
    else
    {
        if( IrpContext->StreamContext != NULLPTR )
        {
            DeallocateBuffer( &IrpContext->StreamContext->FileFullPath );
            IrpContext->StreamContext->FileFullPath = CloneBuffer( DstFileFullPath );
        }
    }
}
