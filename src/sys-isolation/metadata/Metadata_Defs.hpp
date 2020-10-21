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
 *      FileName Rule

            Normal  : Not Managed By Isolation, Free FileName
            Real    : Managed By Isolation, Non-Approve Process See thats
                      Pretend Filename + Containor Suffix
            Pretend : Managed By Isolation, Virtual File which link with Real, Approve Process See thats
                      User want to create filename( ex) AAA.TXT )

            Real FileName : AAA.TXT.EXE, Non-Approve Process See The Filename
            Pretended FileName : AAA.TXT, Approve Process See The Filename

            Approved Process
                Create :
                        Normal = allow   
                        Pretend = allow, create Real 
                        Real = reject
                Delete :
                        Normal = allow
                        Pretend = allow, delete Real
                        Real = not visible, approve process cannot see this 
                Rename :
                        Normal = allow
                        Pretend = allow, but always append containor suffix
                        Real = not visible, approve process cannot see this
                Query  : 
                        Normal = visible
                        Pretend = visible, link with real
                        Real = not visible, approve process cannot see this
            Non-Approved Process
                Create :
                        Normal = allow
                        Pretend = allow( open ), reject( create )
                        Real = reject
                Rename :
                        Normal = allow
                        Pretend = not visible
                        Real = allow, but cannot change extension
                Delete :
                        Normal = allow
                        Pretend = allow
                        Real = allow
                Query  : 
                        Normal = allow
                        Pretend = not visible
                        Real = vibisle

 *
 * Must be Driver's or Solution's MetaData Size aligned by Sector Size
 */

static const char* METADATA_MAGIC_TEXT = "LOSOIW";
static const int METADATA_MAGIC_TEXT_SIZE = 6;
static const int METADATA_DRIVER_SIZE = 1024;
static const unsigned int METADATA_MAXIMUM_CONTAINOR_SIZE = 1048576;

typedef enum _METADATA_TYPE
{
    METADATA_UNK_TYPE,
    METADATA_NOR_TYPE,
    METADATA_RAR_TYPE,
    METADATA_STB_TYPE
} METADATA_TYPE;

typedef enum _METADATA_ENCRYPT_METHOD   // must be same with ENCRYPTION_METHOD
{
    METADATA_ENC_NONE,
    METADATA_ENC_XOR,
    METADATA_ENC_XTEA,
    METADATA_ENC_AES128,
    METADATA_ENC_AES256

} METADATA_ENCRYPT_METHOD;

#ifndef CONTAINOR_SUFFIX_MAX
#define CONTAINOR_SUFFIX_MAX 12
#endif

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
        // including first "." char ( ex) .EXE )
        WCHAR                       ContainorSuffix[ CONTAINOR_SUFFIX_MAX ];

        PVOID                       Fcb;                    // Reserved Use
        LIST_ENTRY                  ListEntry;              // Reserved Use

    } MetaData;

    CHAR DataPointer[ METADATA_DRIVER_SIZE ];

} METADATA_DRIVER, *PMETADATA_DRIVER;

#endif // HDR_ISOLATION_METADATA_DEFS