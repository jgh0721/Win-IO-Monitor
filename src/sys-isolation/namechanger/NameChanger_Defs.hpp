#ifndef HDR_ISOLATION_NAME_CHANGER_DEFS
#define HDR_ISOLATION_NAME_CHANGER_DEFS

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _NAME_CHANGER_ENTRY
{
    WCHAR SrcPattern[ MAX_PATH ];
    WCHAR DstPattern[ MAX_PATH ];
    
} NAME_CHANGER_ENTRY;



#endif // HDR_ISOLATION_NAME_CHANGER_DEFS