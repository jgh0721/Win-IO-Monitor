#ifndef HDR_ISOLATION_IRPCONTEXT_DEFS
#define HDR_ISOLATION_IRPCONTEXT_DEFS

#include "fltBase.hpp"

#include "utilities/bufferMgr_Defs.hpp"
#include "utilities/contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

extern LONG volatile GlobalEvtID;

typedef struct _IRP_CONTEXT
{
    PFLT_CALLBACK_DATA                  Data;
    PCFLT_RELATED_OBJECTS               FltObjects;

    LONG                                EvtID;
    CTX_INSTANCE_CONTEXT*               InstanceContext;        // �ݵ�� CtxReleaseContext �� ȣ���ؾ��Ѵ�

    ULONG                               ProcessId;
    TyGenericBuffer< WCHAR >            ProcessFullPath;
    WCHAR*                              ProcessFileName;        // This just point to ProcessFullPath, dont free this buffer!

    TyGenericBuffer< WCHAR >            SrcFileFullPath;
    WCHAR*                              SrcFileFullPathWOVolume;    // SrcFileFullPath ���� ����̺� ���� �Ǵ� ����̽� �̸����� ������ ������ ��� �� �̸�( \ �� �����Ѵ� )
    TyGenericBuffer< WCHAR >            DstFileFullPath;

    bool                                IsAudit;

    PVOID                               Params;                 // �� IRP ���� ����ϴ� ������ �Է� / ��� ���� ����ü�� ���� ������
    PVOID                               Result;

} IRP_CONTEXT, *PIRP_CONTEXT;

#endif // HDR_ISOLATION_IRPCONTEXT_DEFS