#ifndef HDR_ISOLATION_CREATE
#define HDR_ISOLATION_CREATE

#include "fltBase.hpp"
#include "irpContext_Defs.hpp"
#include "privateFCBMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreCreate( __inout PFLT_CALLBACK_DATA Data,
                 __in PCFLT_RELATED_OBJECTS FltObjects,
                 __deref_out_opt PVOID*     CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostCreate( __inout PFLT_CALLBACK_DATA    Data,
                  __in PCFLT_RELATED_OBJECTS    FltObjects,
                  __in_opt PVOID                CompletionContext,
                  __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END

/*!
    Disposition         Action If File Exists               Action If File Does Not Exist
    -------------------------------------------------------------------------------------
    FILE_SUPERSEDE      Replace the file                    Create the file
    FILE_OPEN           Open the file                       Return an error
    FILE_CREATE         Return an error                     Create the file
    FILE_OPEN_IF        Open the file                       Create the file
    FILE_OVERWRITE      Open the file, and overwrite it     Return an error
    FILE_OVERWRITE_IF   Open the file, and overwrite it     Create the file
*/

/**
 * @brief Open Lower Fileobject
 * @param IrpContext 
 * @return

    IRQL = PASSIVE_LEVEL
*/
NTSTATUS OpenLowerFileObject( __in PIRP_CONTEXT IrpContext );

ACCESS_MASK CreateDesiredAccess( __in PIRP_CONTEXT IrpContext );

///////////////////////////////////////////////////////////////////////////////

enum TyEnFileStatus
{
    FILE_ALREADY_EXISTS = 0x1,
    FILE_ALREADY_READONLY = 0x2
};

typedef struct _CREATE_ARGS
{
    LARGE_INTEGER           FileSize;
    LARGE_INTEGER           FileAllocationSize;
    ULONG                   FileStatus;         // TyEnFileStatus
    FILE_OBJECT*            FileObject;         // IRP 를 통해 전달받은 상위 FileObject

    ULONG                   CreateOptions;
    ULONG                   CreateDisposition;
    ACCESS_MASK             CreateDesiredAccess;
    TyGenericBuffer<WCHAR>  CreateFileName;     // FltCreateFileEx 를 호출하기 위한 디바이스이름\경로\이름
    UNICODE_STRING          CreateFileNameUS;
    OBJECT_ATTRIBUTES       CreateObjectAttributes;

    FCB*                    Fcb;
    FILE_OBJECT*            LowerFileObject;
    HANDLE                  LowerFileHandle;
    
} CREATE_ARGS, * PCREATE_ARGS;

enum TyEnCompleteStatus
{
    COMPLETE_FREE_MAIN_RSRC             = 0x1,      // Fcb 내의 MainResource
    COMPLETE_FREE_INST_RSRC             = 0x2,      // InstanceContext 내의 Lock
    COMPLETE_ALLOCATE_FCB               = 0x4,      // FCB 를 새로 할당해야함 
    COMPLETE_INIT_FCB                   = 0x8,      // FCB 를 초기화 해야함

    COMPLETE_DONT_CONTINUE_PROCESS      = 0x100,    // 외부 함수 수행 후에 즉시 종료
    COMPLETE_CLOSE_LOWER_FILE           = 0x200     // 해당 객체가 디렉토리이거나, 이미 FCB 에 객체가 설정되었거나 등의 이유로 Lower 객체가 필요하지 않음
};

typedef struct _RESULT
{
    IO_STATUS_BLOCK             IoStatus;       // IRP 를 처리하면서 설정, 해당 값을 Data->Iopb->IoStatus 에 설정한다
    FLT_PREOP_CALLBACK_STATUS   FltStatus;

    // IRP_MJ_CREATE( Pre-Create ) 에 대한 최종 후처리 지시사항들
    ULONG                       CompleteStatus;
} CREATE_RESULT, * PCREATE_RESULT;

NTSTATUS ProcessPreCreate_SUPERSEDE( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_SUPERSEDE_NEW( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_SUPERSEDE_EXIST( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OPEN( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OPEN_NEW( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OPEN_EXIST( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_CREATE( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_CREATE_NEW( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_CREATE_EXIST( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OPEN_IF( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OPEN_IF_NEW( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OPEN_IF_EXIST( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OVERWRITE( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OVERWRITE_NEW( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OVERWRITE_EXIST( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OVERWRITE_IF( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OVERWRITE_IF_NEW( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );
NTSTATUS ProcessPreCreate_OVERWRITE_IF_EXIST( __in IRP_CONTEXT* Args, __inout CREATE_RESULT* Result );

#endif // HDR_ISOLATION_CREATE
