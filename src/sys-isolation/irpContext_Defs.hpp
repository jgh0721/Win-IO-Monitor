#ifndef HDR_ISOLATION_IRPCONTEXT_DEFS
#define HDR_ISOLATION_IRPCONTEXT_DEFS

#include "fltBase.hpp"

#include "privateFCBMgr_Defs.hpp"
#include "policies/ProcessFilter.hpp"
#include "utilities/bufferMgr_Defs.hpp"
#include "utilities/contextMgr_Defs.hpp"

#include "WinIOIsolation_Event.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

extern LONG volatile GlobalEvtID;
static const int DEBUG_TEXT_SIZE = 1024;

enum TyEnCompleteState
{
    COMPLETE_FREE_MAIN_RSRC             = 0x1,      // Fcb �� MainResource �� ����
    COMPLETE_FREE_INST_RSRC             = 0x2,      // InstanceContext �� VcbLock ����
    COMPLETE_FREE_PGIO_RSRC             = 0x4,      // Fcb �� PagingIoResource �� ����
                                                    // ���� MainResource �� PagingIoResource �� ���ÿ� �ִٸ� PagingIoResource -> MainResource ������ �����Ѵ�
    COMPLETE_FREE_LOWER_FILEOBJECT      = 0x10,     // �ش� ��ü�� ���丮�̰ų�, �̹� FCB �� ��ü�� �����Ǿ��ų� ���� ������ Lower ��ü�� �ʿ����� ����
                                                    // FCB �� ���� �Ҵ��ؾ��� 
    COMPLETE_CRTE_INIT_FCB              = 0x100,    // FCB �� �ʱ�ȭ �ؾ���
    COMPLETE_CRTE_STREAM_CONTEXT        = 0x200,    // �ش� ���ϰ�ü�� ���� StreamContext �� �����ؾ��Ѵ�
                                                    // ���� �ش� ������ �ݸ���� �����̶�� Fcb �� LowerFileObject �� ���� StreamContext �� �����Ѵ�

    COMPLETE_DONT_CONT_PROCESS          = 0x1000,   // �ܺ� �Լ� ���� �Ŀ� ��� ����
    COMPLETE_IOSTATUS_STATUS            = 0x2000,   // Data->IoStatus.Status �� Status ���� �����Ѵ�       
    COMPLETE_IOSTATUS_INFORMATION       = 0x4000,   // Data->IoStatus.Information �� Information ���� �����Ѵ�
    COMPLETE_RETURN_FLTSTATUS           = 0x8000,   // PreFltStatus ���� IRP ó�� ����� ��ȯ�Ѵ�
};

typedef struct _IRP_CONTEXT
{
    LONG                                EvtID;
    CHAR*                               DebugText;

    PFLT_CALLBACK_DATA                  Data;
    PCFLT_RELATED_OBJECTS               FltObjects;
    bool                                IsOwnObject;            // IRP_MJ_CREATE �� �����ϰ�, �ش� ���� TRUE �� ���� Fcb �� Ccb �� ��ȿ�ϴ�

    CTX_STREAM_CONTEXT*                 StreamContext;
    CTX_INSTANCE_CONTEXT*               InstanceContext;        // �ݵ�� CtxReleaseContext �� ȣ���ؾ��Ѵ�

    FCB*                                Fcb;
    CCB*                                Ccb;
    PVOID                               UserBuffer;

    ULONG                               ProcessId;
    TyGenericBuffer< WCHAR >            ProcessFullPath;
    WCHAR*                              ProcessFileName;        // This just point to ProcessFullPath, dont free this buffer!
    HANDLE                              ProcessFilter;
    PROCESS_FILTER_ENTRY*               ProcessFilterEntry;
    PROCESS_FILTER_MASK_ENTRY*          ProcessFilterMaskItem;

    TyGenericBuffer< WCHAR >            SrcFileFullPath;
    WCHAR*                              SrcFileFullPathWOVolume;    // SrcFileFullPath ���� ����̺� ���� �Ǵ� ����̽� �̸����� ������ ������ ��� �� �̸�( \ �� �����Ѵ� )
    WCHAR*                              SrcFileName;                // SrcFileFullPath ���� ��� �κ��� ������ �����̸� + Ȯ����
    TyGenericBuffer< WCHAR >            DstFileFullPath;
    WCHAR*                              DstFileFullPathWOVolume;
    WCHAR*                              DstFileName;                // DstFileFullPath ���� ��� �κ��� ������ �����̸� + Ȯ���� 

    bool                                IsConcerned;

    PVOID                               Params;                 // �� IRP ���� ����ϴ� ������ �Է� / ��� ���� ����ü�� ���� ������
    TyGenericBuffer<MSG_REPLY_PACKET>   Result;


    ///////////////////////////////////////////////////////////////////////////
    /// IRP �� ó���� ���

    NTSTATUS                            Status;
    ULONG_PTR                           Information;
    FLT_PREOP_CALLBACK_STATUS           PreFltStatus;

    // TyEnCompleteState �� ����
    // CloseIrpContext ���� �ش� ���� ���յ��� �̿��Ͽ� ��ó���� �����Ѵ�
    ULONG                               CompleteStatus;

} IRP_CONTEXT, *PIRP_CONTEXT;

#endif // HDR_ISOLATION_IRPCONTEXT_DEFS