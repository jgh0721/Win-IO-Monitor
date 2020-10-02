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

NTSTATUS CreateFileExistFCB( __in IRP_CONTEXT* IrpContext );
NTSTATUS CreateFileNonExistFCB( __in IRP_CONTEXT* IrpContext );

/**
 * @brief Open Lower Fileobject
 * @param IrpContext 
 * @return

    IRQL = PASSIVE_LEVEL
*/
NTSTATUS OpenLowerFileObject( __in PIRP_CONTEXT IrpContext );
ACCESS_MASK CreateDesiredAccess( __in PIRP_CONTEXT IrpContext );
VOID InitializeVolumeProperties( __in PIRP_CONTEXT IrpContext );

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
    bool                    DeleteOnClose;

    FCB*                    Fcb;
    FILE_OBJECT*            LowerFileObject;
    HANDLE                  LowerFileHandle;
    
} CREATE_ARGS, * PCREATE_ARGS;

NTSTATUS ProcessPreCreate_SUPERSEDE( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_SUPERSEDE_NEW( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_SUPERSEDE_EXIST( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OPEN( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OPEN_NEW( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OPEN_EXIST( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_CREATE( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_CREATE_NEW( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_CREATE_EXIST( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OPEN_IF( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OPEN_IF_NEW( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OPEN_IF_EXIST( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OVERWRITE( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OVERWRITE_NEW( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OVERWRITE_EXIST( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OVERWRITE_IF( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OVERWRITE_IF_NEW( __in IRP_CONTEXT* Args );
NTSTATUS ProcessPreCreate_OVERWRITE_IF_EXIST( __in IRP_CONTEXT* Args );

#endif // HDR_ISOLATION_CREATE
