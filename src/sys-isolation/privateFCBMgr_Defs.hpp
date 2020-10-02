#ifndef HDR_PRIVATE_FCB_MGR_DEFS
#define HDR_PRIVATE_FCB_MGR_DEFS

#include "fltBase.hpp"

#include "utilities/bufferMgr_Defs.hpp"
#include "utilities/contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define FCB_STATE_DELETE_ON_CLOSE   0x100
#define FCB_STATE_FILE_MODIFIED     0x200
#define FCB_STATE_FILE_SIZE_CHANGED 0x400

#define FCB_STATE_PGIO_SHARED       0x10000
#define FCB_STATE_PGIO_EXCLUSIVE    0x20000
#define FCB_STATE_MAIN_SHARED       0x40000
#define FCB_STATE_MAIN_EXCLUSIVE    0x80000

typedef struct _FCB_CUSTOM
{

} FCB_CUSTOM, *PFCB_CUSTOM;

typedef struct _FCB
{
    FSRTL_ADVANCED_FCB_HEADER                   AdvFcbHeader;
    FAST_MUTEX                                  FastMutex;
    SECTION_OBJECT_POINTERS                     SectionObjects;

    // 전달받은 FileObject 에서 추출한 FsContext 가 필터 드라이버에서 생성한 객체가 맞는지 확인용
    ULONG                                       NodeTag;
    ULONG                                       NodeSize;

    ULONG                                       Flags;

    /*!
     * 파일시스템 드라이버와 네트워크 리다이렉터가 캐시관리자와 상호작용할 때 사용하는 읽기/쓰기 락
     *
     * 캐시관리자는 파일시스템 드라이버 또는 네트워크 리다이렉터가 모든 쓰기 동작에 대해 MainResource 를 Exclusively 하게 획득할 것을 기대한다
     * 캐시관리자는 파일시스템 드라이버 또는 네트워크 리다이렉터가 모든 읽기 동작에 대해 MainResource 를 Shared 하게 획득할 것을 기대한다
     */
    ERESOURCE                                   MainResource;
    /*!
     * 캐시관리자의 Modified Page Writer(MPW) 스레드에서 획득한다
     */
    ERESOURCE                                   PagingIoResource;

    /*!
     * AllocationSize : 파일시스템 드라이버 또는 네트워크 리다이렉터에 의해 초기화되며
     *                  해당 값이 변경되면 반드시 캐시관리자에게 변경된 값을 알려야한다
     *
     * ValidDataLength : 실제 파일시스템의 지원여부와 관계없이 캐시관리자는 파일시스템 드라이버 또는 네트워크 리다이렉터가 이 값을 초기화하리랴 예상한다
     *
     * 파일 크기 변경을 캐시관리자에게 통지하려면, 먼저 MainResource 와 PagingIoResource 를 Exclusively 하게 획득한 후 CcSetFileSizes 를 호출한다
     */
    

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

    TyGenericBuffer<WCHAR>                      FileFullPath;
    // SrcFileFullPath 에서 드라이브 문자 또는 디바이스 이름등을 제외한 순수한 경로 및 이름( \ 로 시작한다 )
    WCHAR*                                      FileFullPathWOVolume;
    // FileFullPath 에서 마지막 이름 부분만 가르킴, 직접 해제하지 말 것
    WCHAR*                                      FileName;

    CTX_INSTANCE_CONTEXT*                       InstanceContext;

    ///////////////////////////////////////////////////////////////////////////

    LIST_ENTRY                                  ListEntry;

} FCB, * PFCB;

#define CCB_STATE_OPEN_BY_FILEID                0x1000

// Free on IRP_MJ_CLEANUP
typedef struct _CCB
{
    ULONG                                       Flags;

    ULONG                                       ProcessId;
    WCHAR*                                      ProcessName;            // ProcessFileFullPath 에 대한 포인터, 직접 해제하지 말 것!!!
    TyGenericBuffer<WCHAR>                      ProcessFileFullPath;

    TyGenericBuffer<WCHAR>                      SrcFileFullPath;

    FILE_OBJECT*                                LowerFileObject;
    HANDLE                                      LowerFileHandle;

} CCB, *PCCB;

#endif // HDR_PRIVATE_FCB_MGR_DEFS