#ifndef HDR_PRIVATE_FCB_MGR_DEFS
#define HDR_PRIVATE_FCB_MGR_DEFS

#include "fltBase.hpp"

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
    LONG                                        OpnCount;
    // IRP_MJ_CREATE 1 증가, IRP_MJ_CLEANUP 에서 1 감소
    LONG                                        ClnCount;
    // IRP_MJ_CREATE 1 증가, IRP_MJ_CLOSE 에서 1 감소
    LONG                                        RefCount;

    ///////////////////////////////////////////////////////////////////////////

    // 실제 파일시스템에 연결된 객체
    PFILE_OBJECT                                LowerFileObject;
    HANDLE                                      LowerFileHandle;

} FCB, * PFCB;

#endif // HDR_PRIVATE_FCB_MGR_DEFS