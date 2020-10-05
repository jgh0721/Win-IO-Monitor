#include "fltModWrite.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreAcquireModWrite( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                           PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;
    NTSTATUS                                    Status = STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;

    BOOLEAN                                     AcquireExclusive = FALSE;
    PERESOURCE                                  ResourceAcquired = NULLPTR;

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        auto Fcb = ( FCB* )FileObject->FsContext;

        auto EvtID = CreateEvtID();

        PLARGE_INTEGER EndingOffset = Data->Iopb->Parameters.AcquireForModifiedPageWriter.EndingOffset;
        PERESOURCE* ResourceToRelease = Data->Iopb->Parameters.AcquireForModifiedPageWriter.ResourceToRelease;

        KdPrint( ( "[WinIOSol] >> EvtID=%09d %s Thread=%p Open=%d Clean=%d Ref=%d EndingOffset=%I64d Src=%ws\n"
                   , EvtID, __FUNCTION__
                   , PsGetCurrentThread()
                   , Fcb->OpnCount, Fcb->ClnCount, Fcb->RefCount
                   , EndingOffset->QuadPart
                   , Fcb->FileFullPath.Buffer
                   ) );

        FsRtlEnterFileSystem();

        do
        {
            if( Fcb->AdvFcbHeader.Resource == NULL )
            {
                *ResourceToRelease = NULL;

                Status = STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;
                break;
            }

            if( FlagOn( Fcb->AdvFcbHeader.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX ) ||
                ( EndingOffset->QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart &&
                  Fcb->AdvFcbHeader.ValidDataLength.QuadPart != Fcb->AdvFcbHeader.FileSize.QuadPart ) 
                ) 
            {
                // 파일 크기 확장
                ResourceAcquired = Fcb->AdvFcbHeader.Resource;
                AcquireExclusive = TRUE;
            }
            else if( FlagOn( Fcb->AdvFcbHeader.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH ) ||
                     Fcb->AdvFcbHeader.PagingIoResource == NULL )
            {
                ResourceAcquired = Fcb->AdvFcbHeader.Resource;
                AcquireExclusive = FALSE;
            }
            else
            {
                ResourceAcquired = Fcb->AdvFcbHeader.PagingIoResource;
                AcquireExclusive = FALSE;
            }

            while( TRUE )
            {
                if( AcquireExclusive )
                {
                    if( !ExAcquireResourceExclusiveLite( ResourceAcquired, FALSE ) )
                    {
                        Status = STATUS_CANT_WAIT;
                        break;
                    }
                }
                else if( !ExAcquireSharedWaitForExclusive( ResourceAcquired, FALSE ) )
                {
                    Status = STATUS_CANT_WAIT;
                    break;
                }

                if( FlagOn( Fcb->AdvFcbHeader.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX ) ||
                    EndingOffset->QuadPart > Fcb->AdvFcbHeader.ValidDataLength.QuadPart )
                {
                    if( !AcquireExclusive )
                    {
                        ExReleaseResourceLite( ResourceAcquired );
                        AcquireExclusive = TRUE;
                        ResourceAcquired = Fcb->AdvFcbHeader.Resource;
                        continue;
                    }
                }
                else if( FlagOn( Fcb->AdvFcbHeader.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH ) )
                {
                    if( AcquireExclusive )
                    {
                        ExConvertExclusiveToSharedLite( ResourceAcquired );
                    }
                    else if( ResourceAcquired != Fcb->AdvFcbHeader.Resource )
                    {
                        ExReleaseResourceLite( ResourceAcquired );
                        ResourceAcquired = Fcb->AdvFcbHeader.Resource;
                        AcquireExclusive = TRUE;
                        continue;
                    }
                }
                else if( Fcb->AdvFcbHeader.PagingIoResource != NULL
                         && ResourceAcquired != Fcb->AdvFcbHeader.PagingIoResource )
                {
                    ResourceAcquired = NULL;

                    if( ExAcquireSharedWaitForExclusive( Fcb->AdvFcbHeader.PagingIoResource, FALSE ) )
                    {
                        ResourceAcquired = Fcb->AdvFcbHeader.PagingIoResource;
                    }

                    ExReleaseResourceLite( Fcb->AdvFcbHeader.Resource );

                    if( ResourceAcquired == NULL )
                    {
                        Status = STATUS_CANT_WAIT;
                        break;
                    }
                }
                else if( AcquireExclusive )
                {
                    ExConvertExclusiveToSharedLite( ResourceAcquired );
                }

                break;
            }

            if( Status == STATUS_CANT_WAIT )
                break;

            *ResourceToRelease = ResourceAcquired;

            Status = STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;

        } while( false );

        FsRtlExitFileSystem();

        Data->IoStatus.Status = Status;
        FltStatus = FLT_PREOP_COMPLETE;

        KdPrint( ( "[WinIOSol] << EvtID=%09d %s Thread=%p Open=%d Clean=%d Ref=%d AcquireExclusive=%d Src=%ws\n"
                   , EvtID, __FUNCTION__
                   , PsGetCurrentThread()
                   , Fcb->OpnCount, Fcb->ClnCount, Fcb->RefCount
                   , AcquireExclusive
                   , Fcb->FileFullPath.Buffer
                   ) );
    }
    __finally
    {

    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostAcquireModWrite( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                             PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreReleaseModWrite( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                           PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    FILE_OBJECT* FileObject = FltObjects->FileObject;

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        auto Fcb = ( FCB* )FileObject->FsContext;

        auto EvtID = CreateEvtID();
        KdPrint( ( "[WinIOSol] >> EvtID=%09d %s Thread=%p Open=%d Clean=%d Ref=%d Src=%ws\n"
                   , EvtID, __FUNCTION__
                   , PsGetCurrentThread()
                   , Fcb->OpnCount, Fcb->ClnCount, Fcb->RefCount
                   , Fcb->FileFullPath.Buffer
                   ) );

        FsRtlEnterFileSystem();

        if( Data->Iopb->Parameters.ReleaseForModifiedPageWriter.ResourceToRelease != NULLPTR )
        {
            ExReleaseResourceLite( Data->Iopb->Parameters.ReleaseForModifiedPageWriter.ResourceToRelease );
        }

        FsRtlExitFileSystem();

        Data->IoStatus.Status = STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;
        FltStatus = FLT_PREOP_COMPLETE;

        KdPrint( ( "[WinIOSol] << EvtID=%09d %s Thread=%p Open=%d Clean=%d Ref=%d Src=%ws\n"
                   , EvtID, __FUNCTION__
                   , PsGetCurrentThread()
                   , Fcb->OpnCount, Fcb->ClnCount, Fcb->RefCount
                   , Fcb->FileFullPath.Buffer
                   ) );
    }
    __finally
    {

    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostReleaseModWrite( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                             PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}
