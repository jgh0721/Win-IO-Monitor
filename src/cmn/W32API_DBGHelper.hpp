#ifndef HDR_W32API_DEBUG_HELPER
#define HDR_W32API_DEBUG_HELPER

#include "W32API_Base.hpp"
#include "W32API_FltMgr.hpp"
#include "W32API_NtOsKrnl.hpp"
#include "W32API_NTSTATUS.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsW32API
{
    /*!
        In FLT_CALLBACK_DATA->FLT_IO_PARAMETER_BLOCK->IrpFlags
    */
    void FormatIrpFlags( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in ULONG IrpFlags );
    const char* ConvertIrpMajorFuncTo( __in UCHAR MajorFunction );
    void FormatIrpMajorFunc( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in UCHAR MajorFunction );
    const char* ConvertIrpMinorFuncTo( __in UCHAR MajorFunction, __in UCHAR MinorFunction );
    void FormatIrpMinorFunc( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in UCHAR MajorFunction, __in UCHAR MinorFunction );

    void FormatTopLevelIrp( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize );

    /*!
        In FLT_CALLBACK_DATA->FLT_IO_PARAMETER_BLOCK->OperationFlags
    */
    void FormatOperationFlags( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in ULONG OperationFlags );

    const char* ConvertFileInformationClassTo( __in const FILE_INFORMATION_CLASS FileInformationClass );
    void FormatFileInformationClass( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_INFORMATION_CLASS FileInformationClass );
    const char* ConvertFsInformationClassTo( __in const FS_INFORMATION_CLASS FsInformationClass );
    void FormatFsInformationClass( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FS_INFORMATION_CLASS FsInformationClass );
    const char* ConvertBuiltInFsControlCodeTo( __in ULONG FsControlCode );
    void FormatBuiltInFsControlCode( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in ULONG FsControlCode );

    const char* ConvertCreateDispositionTo( __in ULONG CreateDisposition );
    void FormatCreateDisposition( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in ULONG CreateDisposition );
    void FormatCreateDesiredAccess( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in ULONG DesiredAccess );
    const char* ConvertCreateShareAccessTo( __in USHORT ShareAccess );
    void FormatCreateShareAccess( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in USHORT ShareAccess );
    void FormatCreateOptions( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in ULONG CreateOptions );
    const char* ConvertCreateResultInformationTo( __in NTSTATUS Status, __in ULONG_PTR Information );

    void FormatFileBasicInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_BASIC_INFORMATION* Info );
    void FormatFileStandardInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_STANDARD_INFORMATION* Info );
    void FormatFileAccessInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_ACCESS_INFORMATION* Info );
    void FormatFileRenameInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in_z_opt WCHAR* Destination, __in FILE_RENAME_INFORMATION* Info );
    void FormatFileDispositionInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_DISPOSITION_INFORMATION* Info );
    void FormatFilePositionInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_POSITION_INFORMATION* Info );
    void FormatFileAlignmentInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_ALIGNMENT_INFORMATION* Info );
    void FormatFileAllInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_ALL_INFORMATION* Info );
    void FormatFileAllocationInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_ALLOCATION_INFORMATION* Info );
    void FormatFileEndOfFileInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_END_OF_FILE_INFORMATION* Info );
    void FormatFileValidDataLengthInformation( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in FILE_VALID_DATA_LENGTH_INFORMATION* Info );
    void FormatFileRenameInformationEx( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in_z_opt WCHAR* Destination, __in nsW32API::FILE_RENAME_INFORMATION_EX* Info );
    void FormatFileRenameInformationEx( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in ULONG Flags );
    void FormatFileDispositionInformationEx( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in nsW32API::FILE_DISPOSITION_INFORMATION_EX* Info );
    void FormatFileDispositionInformationEx( __out_bcount_z( BufferSize ) char* PrintBuffer, __in ULONG BufferSize, __in ULONG Flags );

} // nsW32API

#endif // HDR_W32API_DEBUG_HELPER