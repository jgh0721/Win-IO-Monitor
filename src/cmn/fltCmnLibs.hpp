#ifndef HDR_FLT_CMNLIBS
#define HDR_FLT_CMNLIBS

#include "fltBase.hpp"
#include "fltCmnLibs_base.hpp"
#include "fltCmnLibs_string.hpp"
#include "fltCmnLibs_path.hpp"
#include "fltCmnLibs_llrb.hpp"
#include "fltCmnLibs_ring_buffer.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsUtils
{
    FORCEINLINE bool IsEqualUUID( const UUID& lhs, const UUID& rhs )
    {
        return (
            ( ( unsigned long* )&lhs )[ 0 ] == ( ( unsigned long* )&rhs )[ 0 ] &&
            ( ( unsigned long* )&lhs )[ 1 ] == ( ( unsigned long* )&rhs )[ 1 ] &&
            ( ( unsigned long* )&lhs )[ 2 ] == ( ( unsigned long* )&rhs )[ 2 ] &&
            ( ( unsigned long* )&lhs )[ 3 ] == ( ( unsigned long* )&rhs )[ 3 ] );
    }
}

#endif // HDR_FLT_CMNLIBS