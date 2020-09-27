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
                SetFlag( Fcb->Flags, FCB_STATE_MAIN_EXCLUSIVE );
            }
            else
            {
                ExAcquireResourceSharedLite( Fcb->AdvFcbHeader.Resource, TRUE );
                SetFlag( Fcb->Flags, FCB_STATE_MAIN_SHARED );
            }
        }

        if( Fcb->AdvFcbHeader.PagingIoResource )
        {
            if( !BooleanFlagOn( Fcb->Flags, FCB_STATE_PGIO_SHARED ) &&
                !BooleanFlagOn( Fcb->Flags, FCB_STATE_PGIO_EXCLUSIVE ) )
            {
                ExAcquireResourceExclusiveLite( Fcb->AdvFcbHeader.PagingIoResource, TRUE );
                SetFlag( Fcb->Flags, FCB_STATE_PGIO_EXCLUSIVE );
            }
        }

        FsRtlExitFileSystem();

        KdPrint( ( "[WinIOSol] %s Thread=%p Open=%d Clean=%d Ref=%d Name=%ws\n"
                   , __FUNCTION__
                   , PsGetCurrentThread()
                   , Fcb->OpnCount, Fcb->ClnCount, Fcb->RefCount
                   , Fcb->FileFullPath.Buffer ) );

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
        {
            if( BooleanFlagOn( Fcb->Flags, FCB_STATE_MAIN_SHARED ) ||
                BooleanFlagOn( Fcb->Flags, FCB_STATE_MAIN_EXCLUSIVE ) )
            {
                FltReleaseResource( &Fcb->MainResource );
                ClearFlag( Fcb->Flags, FCB_STATE_MAIN_SHARED );
                ClearFlag( Fcb->Flags, FCB_STATE_MAIN_EXCLUSIVE );
            }
        }

        if( Fcb->AdvFcbHeader.PagingIoResource )
        {
            if( BooleanFlagOn( Fcb->Flags, FCB_STATE_PGIO_SHARED ) ||
                BooleanFlagOn( Fcb->Flags, FCB_STATE_PGIO_EXCLUSIVE ) )
            {
                FltReleaseResource( &Fcb->PagingIoResource );
                ClearFlag( Fcb->Flags, FCB_STATE_PGIO_SHARED );
                ClearFlag( Fcb->Flags, FCB_STATE_PGIO_EXCLUSIVE );
            }
        }

        FsRtlExitFileSystem();

        KdPrint( ( "[WinIOSol] %s Thread=%p Open=%d Clean=%d Ref=%d Name=%ws\n"
                   , __FUNCTION__
                   , PsGetCurrentThread()
                   , Fcb->OpnCount, Fcb->ClnCount, Fcb->RefCount
                   , Fcb->FileFullPath.Buffer ) );

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
