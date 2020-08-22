#ifndef HDR_UTIL_CONTEXTMGR_DEFS
#define HDR_UTIL_CONTEXTMGR_DEFS

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _CTX_GLOBAL_DATA
{
    PDRIVER_OBJECT                  DriverObject;
    PDEVICE_OBJECT                  DeviceObject;       // Control Device Object
    PFLT_FILTER                     Filter;

    PFLT_PORT                       ServerPort;
    PFLT_PORT                       ClientPort;

    ULONG                           DebugLevel;

    NPAGED_LOOKASIDE_LIST           FileNameLookasideList;
    NPAGED_LOOKASIDE_LIST           ProcNameLookasideList;
    NPAGED_LOOKASIDE_LIST           SendPacketLookasideList;
    NPAGED_LOOKASIDE_LIST           ReplyPacketLookasideList;

} CTX_GLOBAL_DATA, * PCTX_GLOBAL_DATA;

extern CTX_GLOBAL_DATA GlobalContext;

typedef struct _CTX_INSTANCE_CONTEXT
{
    
} CTX_INSTANCE_CONTEXT, *PCTX_INSTANCE_CONTEXT;

#define CTX_INSTANCE_CONTEXT_SIZE sizeof( CTX_INSTANCE_CONTEXT )

typedef struct _CTX_VOLUME_CONTEXT
{

} CTX_VOLUME_CONTEXT, * PCTX_VOLUME_CONTEXT;

#define CTX_VOLUME_CONTEXT_SIZE sizeof( CTX_VOLUME_CONTEXT )

typedef struct _CTX_FILE_CONTEXT
{

} CTX_FILE_CONTEXT, * PCTX_FILE_CONTEXT;

#define CTX_FILE_CONTEXT_SIZE sizeof( CTX_FILE_CONTEXT )

typedef struct _CTX_STREAM_CONTEXT
{

} CTX_STREAM_CONTEXT, * PCTX_STREAM_CONTEXT;

#define CTX_STREAM_CONTEXT_SIZE sizeof( CTX_STREAM_CONTEXT )

typedef struct _CTX_STREAMHANDLE_CONTEXT
{
    UNICODE_STRING          FileName;

} CTX_STREAMHANDLE_CONTEXT, * PCTX_STREAMHANDLE_CONTEXT;

#define CTX_STREAMHANDLE_CONTEXT_SIZE sizeof( CTX_STREAMHANDLE_CONTEXT )

typedef struct _CTX_SECTION_CONTEXT
{

} CTX_SECTION_CONTEXT, * PCTX_SECTION_CONTEXT;

#define CTX_SECTION_CONTEXT_SIZE sizeof( CTX_SECTION_CONTEXT )

typedef struct _CTX_TRANSACTION_CONTEXT
{

} CTX_TRANSACTION_CONTEXT, * PCTX_TRANSACTION_CONTEXT;

#define CTX_TRANSACTION_CONTEXT_SIZE sizeof( CTX_TRANSACTION_CONTEXT )

#endif // HDR_UTIL_CONTEXTMGR_DEFS