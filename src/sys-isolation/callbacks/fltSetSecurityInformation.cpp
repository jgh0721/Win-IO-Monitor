#include "fltSetSecurityInformation.hpp"

#include "callbacks.hpp"
#include "irpContext.hpp"
#include "privateFCBMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FLT_PREOP_CALLBACK_STATUS FLTAPI FilterPreSetSecurityInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                            PVOID* CompletionContext )
{
    FLT_PREOP_CALLBACK_STATUS                   FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    FILE_OBJECT*                                FileObject = FltObjects->FileObject;
    IRP_CONTEXT*                                IrpContext = NULLPTR;

    UNREFERENCED_PARAMETER( CompletionContext );

    /*!
        NOTE: 윈도 Vista 이전 OS 에서는 FltSetSecurityObject 를 호출하면 항상 STATUS_NOT_IMPLEMENTED 를 반환하므로,
        직접 IRP 를 빌드하여 호출한다
    */

    PDEVICE_OBJECT  FltDeviceObject = NULLPTR;
    PDEVICE_OBJECT  DeviceObject = NULLPTR;
    PIRP            NewIrp = NULLPTR;

    __try
    {
        if( IsOwnFileObject( FileObject ) == false )
            __leave;

        if( FLT_IS_FASTIO_OPERATION( Data ) )
            return FLT_PREOP_DISALLOW_FASTIO;

        IrpContext = CreateIrpContext( Data, FltObjects );
        PrintIrpContext( IrpContext );

        AcquireCmnResource( IrpContext, FCB_MAIN_EXCLUSIVE );
        FileObject = ( ( CCB* )FileObject->FsContext2 )->LowerFileObject;

        if( FileObject == NULLPTR )
        {
            AssignCmnResult( IrpContext, STATUS_FILE_DELETED );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            __leave;
        }

        NTSTATUS Status = FltGetDeviceObject( FltObjects->Volume, &FltDeviceObject );

        if( !NT_SUCCESS( Status ) )
        {
            AssignCmnResult( IrpContext, Status );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            __leave;
        }

        DeviceObject = IoGetDeviceAttachmentBaseRef( FltDeviceObject );

        NewIrp = IoAllocateIrp( DeviceObject->StackSize + 1, FALSE );

        if( NewIrp == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            AssignCmnResult( IrpContext, Status );
            AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
            __leave;
        }

        NewIrp->Flags = IRP_SYNCHRONOUS_API;
        NewIrp->RequestorMode = KernelMode;
        NewIrp->UserIosb = NULL;
        NewIrp->UserEvent = NULL;
        NewIrp->Tail.Overlay.Thread = ( PETHREAD )KeGetCurrentThread();

        KEVENT event;
        KeInitializeEvent( &event, NotificationEvent, FALSE );

        PIO_STACK_LOCATION nextIrpSp = IoGetNextIrpStackLocation( NewIrp );

        nextIrpSp->MajorFunction = Data->Iopb->MajorFunction;
        nextIrpSp->MinorFunction = Data->Iopb->MinorFunction;
        nextIrpSp->FileObject = FileObject;

        nextIrpSp->Parameters.SetSecurity.SecurityDescriptor = Data->Iopb->Parameters.SetSecurity.SecurityDescriptor;
        nextIrpSp->Parameters.SetSecurity.SecurityInformation = Data->Iopb->Parameters.SetSecurity.SecurityInformation;

        IoSetCompletionRoutine( NewIrp, EventSyncMoreProcessingComplete, &event,
                                TRUE, TRUE, TRUE );

        if( IoCallDriver( DeviceObject, NewIrp ) == STATUS_PENDING )
            KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );

        AssignCmnResult( IrpContext, NewIrp->IoStatus.Status );
        AssignCmnResultInfo( IrpContext, NewIrp->IoStatus.Information );
        AssignCmnFltResult( IrpContext, FLT_PREOP_COMPLETE );
    }
    __finally
    {
        if( NewIrp != NULLPTR )
            IoFreeIrp( NewIrp );
        if( FltDeviceObject != NULLPTR )
            ObDereferenceObject( FltDeviceObject );
        if( DeviceObject != NULLPTR )
            ObDereferenceObject( DeviceObject );

        if( IrpContext != NULLPTR )
        {
            if( FlagOn( IrpContext->CompleteStatus, COMPLETE_RETURN_FLTSTATUS ) )
                FltStatus = IrpContext->PreFltStatus;

            PrintIrpContext( IrpContext, true );
        }

        CloseIrpContext( IrpContext );
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS FLTAPI FilterPostSetSecurityInformation( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,
                                                              PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags )
{
    FLT_POSTOP_CALLBACK_STATUS                  FltStatus = FLT_POSTOP_FINISHED_PROCESSING;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FltStatus;
}
