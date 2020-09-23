#include "fltCcFlush.hpp"

#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreAcquireCcFlush( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                          PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        auto Fcb = ( FCB* )FileObject->FsContext;

        FsRtlEnterFileSystem();

        if( Fcb->AdvFcbHeader.Resource )
        {
            if( !ExIsResourceAcquiredSharedLite( Fcb->AdvFcbHeader.Resource ) )
            {
                ExAcquireResourceExclusiveLite( Fcb->AdvFcbHeader.Resource, TRUE );
            }
            else
            {
                ExAcquireResourceSharedLite( Fcb->AdvFcbHeader.Resource, TRUE );
            }
        }

        if( Fcb->AdvFcbHeader.PagingIoResource )
        {
            ExAcquireResourceSharedLite( Fcb->AdvFcbHeader.PagingIoResource, TRUE );
        }

        FsRtlExitFileSystem();

        Data->IoStatus.Status = STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;
        FltStatus = FLT_PREOP_COMPLETE;
    }
    __finally
    {

    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostAcquireCcFlush( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                            PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreReleaseCcFlush( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                          PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;

    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        auto Fcb = ( FCB* )FileObject->FsContext;

        FsRtlEnterFileSystem();

        if( Fcb->AdvFcbHeader.Resource )
            ExReleaseResourceLite( Fcb->AdvFcbHeader.Resource );

        if( Fcb->AdvFcbHeader.PagingIoResource )
            ExReleaseResourceLite( Fcb->AdvFcbHeader.PagingIoResource );

        FsRtlExitFileSystem();

        Data->IoStatus.Status = STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;
        FltStatus = FLT_PREOP_COMPLETE;
    }
    __finally
    {

    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostReleaseCcFlush( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                            PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}
