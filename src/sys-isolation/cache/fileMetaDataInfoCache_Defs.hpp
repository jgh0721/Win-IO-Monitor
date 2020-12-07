#ifndef HDR_WINIOISOLATION_FILE_METADATA_CACHE_DEFS
#define HDR_WINIOISOLATION_FILE_METADATA_CACHE_DEFS

#include "fltBase.hpp"
#include "fltCmnLibs_llrb.hpp"

#include "utilities/bufferMgr_Defs.hpp"
#include "metadata/Metadata_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

static const int HASH_DIGEST_SIZE = 48;

typedef struct _FILE_INFO_NODE
{
    LLRB_ENTRY( _FILE_INFO_NODE )   Entry;

    TyGenericBuffer<WCHAR>              FileFullPath;
    METADATA_DRIVER*                    MetaDataInfo;

} FILE_METADATA_NODE;


#endif // HDR_WINIOISOLATION_FILE_METADATA_CACHE_DEFS

