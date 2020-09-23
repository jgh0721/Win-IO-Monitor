#ifndef HDR_UTIL_CONTEXTMGR_DEFS
#define HDR_UTIL_CONTEXTMGR_DEFS

#include "fltBase.hpp"

#include "bufferMgr_Defs.hpp"
#include "W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _CTX_GLOBAL_DATA
{
    PDRIVER_OBJECT                  DriverObject;
    PDEVICE_OBJECT                  DeviceObject;       // Control Device Object
    PFLT_FILTER                     Filter;

    CACHE_MANAGER_CALLBACKS         CacheMgrCallbacks;

    PFLT_PORT                       ServerPort;
    PFLT_PORT                       ClientPort;

    ULONG                           DebugLevel;
    NPAGED_LOOKASIDE_LIST           DebugLookasideList;

    NPAGED_LOOKASIDE_LIST           IrpContextLookasideList;
    NPAGED_LOOKASIDE_LIST           FileNameLookasideList;
    NPAGED_LOOKASIDE_LIST           ProcNameLookasideList;
    NPAGED_LOOKASIDE_LIST           SendPacketLookasideList;
    NPAGED_LOOKASIDE_LIST           ReplyPacketLookasideList;
    NPAGED_LOOKASIDE_LIST           FcbLookasideList;
    NPAGED_LOOKASIDE_LIST           CcbLookasideList;

    PAGED_LOOKASIDE_LIST            SwapReadLookasideList_1024;
    PAGED_LOOKASIDE_LIST            SwapReadLookasideList_4096;
    PAGED_LOOKASIDE_LIST            SwapReadLookasideList_8192;
    PAGED_LOOKASIDE_LIST            SwapReadLookasideList_16384;
    PAGED_LOOKASIDE_LIST            SwapReadLookasideList_65536;

    PAGED_LOOKASIDE_LIST            SwapWriteLookasideList_1024;
    PAGED_LOOKASIDE_LIST            SwapWriteLookasideList_4096;
    PAGED_LOOKASIDE_LIST            SwapWriteLookasideList_8192;
    PAGED_LOOKASIDE_LIST            SwapWriteLookasideList_16384;
    PAGED_LOOKASIDE_LIST            SwapWriteLookasideList_65536;

    PVOID                           ProcessFilter;

    LARGE_INTEGER                   TimeOutMs;

} CTX_GLOBAL_DATA, * PCTX_GLOBAL_DATA;

extern CTX_GLOBAL_DATA GlobalContext;

typedef struct _CTX_INSTANCE_CONTEXT
{
    PFLT_FILTER                     Filter;
    PFLT_INSTANCE                   Instance;
    PFLT_VOLUME                     Volume;

    UNICODE_STRING                  DeviceName;
    WCHAR                           DeviceNameBuffer[ 128 ];

    UNICODE_STRING                  VolumeGUIDName;
    WCHAR                           VolumeGUIDNameBuffer[ 64 ];
    FLT_FILESYSTEM_TYPE             VolumeFileSystemType;
    union
    {
        FLT_VOLUME_PROPERTIES       VolumeProperties;
        UCHAR                       Data[ 256 ];
    };
    BOOLEAN                         IsVolumePropertySet;

    WCHAR                           DriveLetter;

    BOOLEAN                         IsWritable;

    ULONG                           BytesPerSector;
    ULONG                           SectorsPerAllocationUnit;
    ULONG                           ClusterSize;
    BOOLEAN                         IsAllocationPropertySet;

    ///
    /// Fcb Management
    ///

    ERESOURCE                       VcbLock;
    LIST_ENTRY                      FcbListHead;                // 해당 인스턴스에서 관리 중인 FCB 객체들의 목록

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
    TyGenericBuffer<WCHAR>              FileFullPath;

    PFLT_FILE_NAME_INFORMATION          NameInfo;

    ULONG                               CreateCount;
    ULONG                               CleanupCount;
    ULONG                               CloseCount;

    //  File ID, obtained from querying the file system for FileInternalInformation.
    //  If the File ID is 128 bits (as in ReFS) we get it via FileIdInformation.
    nsW32API::UNIFIED_FILE_REFERENCE    FileId;
    BOOLEAN                             FileIdSet;

    volatile LONG                       NumOps;
    volatile LONG                       IsNotified;

    BOOLEAN                             SetDisp;
    BOOLEAN                             DeleteOnClose;

    PERESOURCE                          Resource;

} CTX_STREAM_CONTEXT, * PCTX_STREAM_CONTEXT;

#define CTX_STREAM_CONTEXT_SIZE sizeof( CTX_STREAM_CONTEXT )

#define OPEN_BY_FILE_ID

typedef struct _CTX_STREAMHANDLE_CONTEXT
{
    UNICODE_STRING          FileName;

    ULONG                   ProcessId;
    TyGenericBuffer<WCHAR>  ProcessFileFullPath;
    WCHAR*                  ProcessName;

} CTX_STREAMHANDLE_CONTEXT, * PCTX_STREAMHANDLE_CONTEXT;

#define CTX_STREAMHANDLE_CONTEXT_SIZE sizeof( CTX_STREAMHANDLE_CONTEXT )

typedef struct _CTX_SECTION_CONTEXT
{

} CTX_SECTION_CONTEXT, * PCTX_SECTION_CONTEXT;

#define CTX_SECTION_CONTEXT_SIZE sizeof( CTX_SECTION_CONTEXT )

typedef struct _CTX_TRANSACTION_CONTEXT
{
    //  List of DF_DELETE_NOTIFY structures representing pending delete
    //  notifications.
    LIST_ENTRY                      DeleteNotifyList;

    PERESOURCE                      Resource;

} CTX_TRANSACTION_CONTEXT, * PCTX_TRANSACTION_CONTEXT;

#define CTX_TRANSACTION_CONTEXT_SIZE sizeof( CTX_TRANSACTION_CONTEXT )

//
//  This structure represents pending delete notifications for files that have
//  been deleted in an open transaction.
//

typedef struct _DF_DELETE_NOTIFY
{

    //
    //  Links to other DF_DELETE_NOTIFY structures in the list.
    //

    LIST_ENTRY Links;

    //
    //  Pointer to the stream context for the deleted stream/file.
    //

    PCTX_STREAM_CONTEXT StreamContext;

    //
    //  TRUE for a deleted file, FALSE for a stream.
    //

    BOOLEAN FileDelete;

} DF_DELETE_NOTIFY, * PDF_DELETE_NOTIFY;

#endif // HDR_UTIL_CONTEXTMGR_DEFS