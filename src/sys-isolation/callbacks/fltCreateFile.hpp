#ifndef HDR_ISOLATION_CREATE
#define HDR_ISOLATION_CREATE

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

EXTERN_C_BEGIN

FLT_PREOP_CALLBACK_STATUS FLTAPI
FilterPreCreate( __inout PFLT_CALLBACK_DATA Data,
                 __in PCFLT_RELATED_OBJECTS FltObjects,
                 __deref_out_opt PVOID*     CompletionContext );

FLT_POSTOP_CALLBACK_STATUS FLTAPI
FilterPostCreate( __inout PFLT_CALLBACK_DATA    Data,
                  __in PCFLT_RELATED_OBJECTS    FltObjects,
                  __in_opt PVOID                CompletionContext,
                  __in FLT_POST_OPERATION_FLAGS Flags );

EXTERN_C_END

/*!
    Disposition         Action If File Exists               Action If File Does Not Exist
    -------------------------------------------------------------------------------------
    FILE_SUPERSEDE      Replace the file                    Create the file
    FILE_OPEN           Open the file                       Return an error
    FILE_CREATE         Return an error                     Create the file
    FILE_OPEN_IF        Open the file                       Create the file
    FILE_OVERWRITE      Open the file, and overwrite it     Return an error
    FILE_OVERWRITE_IF   Open the file, and overwrite it     Create the file
*/

#endif // HDR_ISOLATION_CREATE
