#include "fltSectionSynchronization.hpp"

#include "privateFCBMgr.hpp"
#include "irpContext.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

///////////////////////////////////////////////////////////////////////////////

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreAcquireSectionSynchronization( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                                         PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    const auto&                                 FileObject = FltObjects->FileObject;

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        auto Fcb = ( FCB* )FileObject->FsContext;

        KdPrint( ( "[WinIOSol] EvtID=%09d IRP=%s Name=%ws \n"
                   , CreateEvtID(), FltGetIrpName( Data->Iopb->MajorFunction )
                   , Fcb->FileFullPath.Buffer ) );

        FltAcquireResourceExclusive( &Fcb->MainResource );
        Data->IoStatus.Status = STATUS_SUCCESS;
        Data->IoStatus.Information = 0;
        FltStatus = FLT_PREOP_COMPLETE;
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

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        auto Fcb = ( FCB* )FileObject->FsContext;

        KdPrint( ( "[WinIOSol] EvtID=%09d IRP=%s Name=%ws \n"
                   , CreateEvtID(), FltGetIrpName( Data->Iopb->MajorFunction )
                   , Fcb->FileFullPath.Buffer ) );

        FltReleaseResource( &Fcb->MainResource );
        Data->IoStatus.Status = STATUS_SUCCESS;
        Data->IoStatus.Information = 0;
        FltStatus = FLT_PREOP_COMPLETE;
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
