#ifndef HDR_WINIOISOLATION_FILE_METADATA_CACHE
#define HDR_WINIOISOLATION_FILE_METADATA_CACHE

#include "fltBase.hpp"
#include "fileMetaDataInfoCache_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------
/// @brief Compare two process nodes for sorting the LLRB tree
///
/// @param first   First process node to compare
/// @param second  Second process node to compare
///
/// @returns <0 if first node's process ID is less than second;
///           0 if nodes' process IDs are equal
///          >0 if first node's process ID is greater than second
int CompareProcessNodes( FILE_METADATA_NODE* first, FILE_METADATA_NODE* second );

#endif // HDR_WINIOISOLATION_FILE_METADATA_CACHE


#ifdef __cplusplus
}; // extern "C"
#endif
