#ifndef HDR_PRIVATE_FCB_MGR_DEFS
#define HDR_PRIVATE_FCB_MGR_DEFS

#include "fltBase.hpp"

#include "utilities/bufferMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _FCB
{
    FSRTL_ADVANCED_FCB_HEADER                   AdvFcbHeader;
    FAST_MUTEX                                  FastMutex;
    SECTION_OBJECT_POINTERS                     SectionObjects;

    // 전달받은 FileObject 에서 추출한 FsContext 가 필터 드라이버에서 생성한 객체가 맞는지 확인용
    ULONG                                       NodeTag;
    ULONG                                       NodeSize;

    ULONG                                       Flags;

    ERESOURCE                                   MainResource;
    ERESOURCE                                   PagingIoResource;

    FILE_LOCK                                   FileLock;
    OPLOCK                                      FileOplock;

    // IRP_MJ_CREATE 1 증가, IRP_MJ_CLEANUP 에서 1 감소
    LONG volatile                               OpnCount;
    // IRP_MJ_CREATE 1 증가, IRP_MJ_CLEANUP 에서 1 감소
    LONG volatile                               ClnCount;
    // IRP_MJ_CREATE 1 증가, IRP_MJ_CLOSE 에서 1 감소
    LONG volatile                               RefCount;

    ///////////////////////////////////////////////////////////////////////////

    // 실제 파일시스템에 연결된 객체
    PFILE_OBJECT                                LowerFileObject;
    HANDLE                                      LowerFileHandle;
    SHARE_ACCESS                                LowerShareAccess;

    // 드라이브 문자, 디바이스 이름 등을 제외한 경로 및 이름( \ 시작 )
    TyGenericBuffer<WCHAR>                      FileFullPath;
    // FileFullPath 에서 마지막 이름 부분만 가르킴, 직접 해제하지 말 것
    WCHAR*                                      FileName;

    ///////////////////////////////////////////////////////////////////////////

    LIST_ENTRY                                  ListEntry;

} FCB, * PFCB;

// Free on IRP_MJ_CLEANUP
typedef struct _HANDLE_CONTEXT
{
    ULONG                                       ProcessId;

    WCHAR*                                      ProcessName;            // ProcessFileFullPath 에 대한 포인터, 직접 해제하지 말 것!!!
    TyGenericBuffer<WCHAR>                      ProcessFileFullPath;

    TyGenericBuffer<WCHAR>                      SrcFileFullPath;

} HANDLE_CONTEXT, *PHANDLE_CONTEXT;

#endif // HDR_PRIVATE_FCB_MGR_DEFS