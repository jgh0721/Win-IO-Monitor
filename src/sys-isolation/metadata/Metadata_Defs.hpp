#ifndef HDR_ISOLATION_METADATA_DEFS
#define HDR_ISOLATION_METADATA_DEFS

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/*!
 * File Metadata(Hidden Header)
 *
 * Type 1.
 *
 * ------------------------------------------------------------------
 * |            Metadata   |            Contents                    |
 * |-----------------------------------------------------------------
 * | Driver's | Solution's |            Contents                    |
 * |----------------------------------------------------------------|
 *
 * Type 2.
 * 
 * ------------------------------------------------------------------
 * |     Contents          |            Metadata                    |
 * |-----------------------------------------------------------------
 * |     Contents          |     Driver's   | Solution's            |
 * |----------------------------------------------------------------|
 *
 * Type 3.
 *
 * ------------------------------------------------------------------
 * | EXE Stub |         Metadata      |           Contents          |
 * |-----------------------------------------------------------------
 * | Stub Code| Driver's | Solution's |           Contents          |
 * |----------------------------------------------------------------|
 *
 *
 * Must be Driver's or Solution's MetaData Size aligned by Sector Size
 */

static const char* METADATA_MAGIC_TEXT = "LOSOIW";
static const int METADATA_MAGIC_TEXT_SIZE = 6;
static const int METADATA_DRIVER_SIZE = 1024;

typedef enum _METADATA_TYPE
{
    METADATA_UNK_TYPE,
    METADATA_NOR_TYPE,
    METADATA_RAR_TYPE,
    METADATA_STB_TYPE
} METADATA_TYPE;

typedef enum _METADATA_ENCRYPT_METHOD
{
    METADATA_ENC_NONE
    
} METADATA_ENCRYPT_METHOD;

typedef union _METADATA_DRIVER
{
    struct
    {
        CHAR                        Magic[ METADATA_MAGIC_TEXT_SIZE ];

        ULONG                       Version;
        METADATA_TYPE               Type;
        METADATA_ENCRYPT_METHOD     EncryptMethod;

        ULONG                       ContainorSize;
        ULONG                       SolutionMetaDataSize;
        LONGLONG                    ContentSize;

        PVOID                       Fcb;                    // Reserved Use
        LIST_ENTRY                  ListEntry;              // Reserved Use

    } MetaData;

    CHAR DataPointer[ METADATA_DRIVER_SIZE ];

} METADATA_DRIVER, *PMETADATA_DRIVER;

#endif // HDR_ISOLATION_METADATA_DEFS