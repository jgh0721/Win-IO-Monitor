#include "fltSectionSynchronization.hpp"

#include "privateFCBMgr.hpp"
#include "irpContext.hpp"

#include "utilities/osInfoMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreAcquireSectionSynchronization( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                                         PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    const auto&                                 FileObject = FltObjects->FileObject;

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

        FltAcquireResourceExclusive( &Fcb->MainResource );

        if( nsUtils::VerifyVersionInfoEx( 6, 0, ">=" ) == true )
        {
            if( Data->Iopb->Parameters.AcquireForSectionSynchronization.SyncType == SyncTypeCreateSection )
            {
                Data->IoStatus.Status = STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;
            }
            else
            {
                if( Fcb->LowerShareAccess.Writers == 0 )
                    Data->IoStatus.Status = STATUS_FILE_LOCKED_WITH_ONLY_READERS;
                else
                    Data->IoStatus.Status = STATUS_FILE_LOCKED_WITH_WRITERS;
            }
        }
        else
        {
            Data->IoStatus.Status = STATUS_SUCCESS;
        }

        Data->IoStatus.Information = 0;
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

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostAcquireSectionSynchronization( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
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

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreReleaseSectionSynchronization( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                                         PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    const auto&                                 FileObject = FltObjects->FileObject;

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

        FltReleaseResource( &Fcb->MainResource );
        Data->IoStatus.Status = STATUS_SUCCESS;
        Data->IoStatus.Information = 0;
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

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostReleaseSectionSynchronization( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                                           PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}
