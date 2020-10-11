#include "Metadata.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

typedef struct _METADATA_CONTEXT
{
    NPAGED_LOOKASIDE_LIST                   MetaDataLookASideList;

    ERESOURCE                               MetaDataLock;
    LIST_ENTRY                              ListHead;

} METADATA_CONTEXT, *PMETADATA_CONTEXT;