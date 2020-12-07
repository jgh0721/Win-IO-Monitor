#include "fileMetaDataInfoCache.hpp"

#include "fltCmnLibs_string.hpp"


#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef LLRB_HEAD( ProcessTree, _FILE_INFO_NODE ) FILEMETADATA_TREE_HEAD;
static FILEMETADATA_TREE_HEAD gProcessTreeHead = LLRB_INITIALIZER( &gProcessTreeHead );

#pragma warning(push)
#pragma warning(disable:4706) // LLRB uses assignments in conditional expressions
LLRB_GENERATE( ProcessTree, _FILE_INFO_NODE, Entry, CompareProcessNodes );
#pragma warning(pop)

int CompareProcessNodes( FILE_METADATA_NODE* first, FILE_METADATA_NODE* second )
{
    return nsUtils::stricmp( first->FileFullPath.Buffer, second->FileFullPath.Buffer );
}

#ifdef __cplusplus
}; // extern "C"
#endif
