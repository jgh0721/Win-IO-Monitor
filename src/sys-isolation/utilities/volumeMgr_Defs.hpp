#ifndef HDR_WINIOISOLATION_VOLUMENAME_MGR_DEFS
#define HDR_WINIOISOLATION_VOLUMENAME_MGR_DEFS

#include "fltBase.hpp"

#include "contextMgr_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _VOLUME_INFO_ENTRY
{
    WCHAR                   VolumeNameBuffer[ 128 ];
    ULONG                   VolumeNameCch;              // real volumename char count, not including null 
    WCHAR                   Letter;

    CTX_INSTANCE_CONTEXT*   InstanceContext;

    LIST_ENTRY              ListEntry;

} VOLUME_INFO_ENTRY, *PVOLUME_INFO_ENTRY;

#endif // HDR_WINIOISOLATION_VOLUMENAME_MGR_DEFS