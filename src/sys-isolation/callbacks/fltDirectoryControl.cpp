#include "fltDirectoryControl.hpp"

#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreDirectoryControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                            PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    __try
    {

    }
    __finally
    {

    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostDirectoryControl( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                              PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    __try
    {
        if( FileObject == NULLPTR )
            __leave;

        if( Data->IoStatus.Status != STATUS_SUCCESS )
            __leave;

        // MinorFunction == IRP_MN_QUERY_DIRECTORY, IRP_MN_NOTIFY_CHANGE_DIRECTORY 
        if( Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY )
            __leave;

        //
        //  Post operation callbacks may be called at DPC level.  This routine may be
        //  used to transfer completion processing to a "safe" IRQL level.  This
        //  routine will determine if it is safe to call the "SafePostCallback" now
        //  or if it must post the request to a worker thread.  If posting to a worker
        //  thread is needed it determines it is safe to do so (some operations can
        //  not be posted like paging IO).
        //

        if( FltDoCompletionProcessingWhenSafe( Data, FltObjects, CompletionContext, Flags, 
                                               FilterPostDirectoryControlWhenSafe, &FltStatus ) != FALSE )
        {
            __leave;
        }
        else
        {
            Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
            Data->IoStatus.Information = 0;
        }
    }
    __finally
    {

    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FilterPostDirectoryControlWhenSafe( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;
    IRP_CONTEXT*                                IrpContext = NULLPTR;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    __try
    {
        if( FileObject == NULLPTR )
            __leave;

        if( Data->IoStatus.Status != STATUS_SUCCESS )
            __leave;

        // MinorFunction == IRP_MN_QUERY_DIRECTORY, IRP_MN_NOTIFY_CHANGE_DIRECTORY 
        if( Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY )
            __leave;

        IrpContext = CreateIrpContext( Data, FltObjects );
        PrintIrpContext( IrpContext );

        auto FileInformationClass = (nsW32API::FILE_INFORMATION_CLASS)Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;

        switch( FileInformationClass )
        {
            case FileDirectoryInformation: {} break;
            case FileFullDirectoryInformation: {} break;
            case FileBothDirectoryInformation: {} break;
            case FileNamesInformation: {} break;
            case FileObjectIdInformation: {} break;
            case FileQuotaInformation: {} break;
            case FileReparsePointInformation: {} break;
            case FileIdBothDirectoryInformation: {} break;
            case FileIdFullDirectoryInformation: {} break;
            case FileIdGlobalTxDirectoryInformation: {} break;
            case nsW32API::FileIdExtdDirectoryInformation: {} break;
            case nsW32API::FileIdExtdBothDirectoryInformation: {} break;
        }
    }
    __finally
    {
        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}
