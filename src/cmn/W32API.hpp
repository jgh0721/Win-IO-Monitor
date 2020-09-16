#ifndef HDR_W32API
#define HDR_W32API

#include "fltBase.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#include "W32API_Base.hpp"
#include "W32API_NtOsKrnl.hpp"
#include "W32API_FltMgr.hpp"

namespace nsW32API
{
    /**
     * @brief The FltIsVolumeWritable routine determines whether the disk device that corresponds to a volume or minifilter driver instance is writable. 
     * @param FltObject  Volume or Instance Object
     * @param IsWritable 
     * @return
     *
     * Support WinXP
    */
    NTSTATUS IsVolumeWritable( __in PVOID FltObject, __out PBOOLEAN IsWritable );

    NTSTATUS FltCreateFileEx( __in      PFLT_FILTER Filter,
                              __in_opt  PFLT_INSTANCE Instance,
                              __out     PHANDLE FileHandle,
                              __out     PFILE_OBJECT* FileObject,
                              __in      ACCESS_MASK DesiredAccess,
                              __in      POBJECT_ATTRIBUTES ObjectAttributes,
                              __out     PIO_STATUS_BLOCK IoStatusBlock,
                              __in_opt  PLARGE_INTEGER AllocationSize,
                              __in      ULONG FileAttributes,
                              __in      ULONG ShareAccess,
                              __in      ULONG CreateDisposition,
                              __in      ULONG CreateOptions,
                              __in_opt  PVOID EaBuffer,
                              __in      ULONG EaLength,
                              __in      ULONG Flags
    );

    /*!
        if os support FltCreateFileEx2, then call native FltCreateFileEx2
        , if not then call FltCreateFileEx or FltCreateFile and ObReferenceObjectByHandle

        전달받은 FileHandle 및 FileObject 는 더이상 필요가 없을 때 반드시 FltClose, ObDereferenceObject 를 호출하여 해제해야한다. 

        Support WinXP, code from MS Namechanger sample code
    */
    NTSTATUS FltCreateFileEx2( __in PFLT_FILTER                   Filter,
                               __in_opt PFLT_INSTANCE             Instance,
                               __out PHANDLE                      FileHandle,
                               __out_opt PFILE_OBJECT*            FileObject,
                               __in ACCESS_MASK                   DesiredAccess,
                               __in POBJECT_ATTRIBUTES            ObjectAttributes,
                               __out PIO_STATUS_BLOCK             IoStatusBlock,
                               __in_opt PLARGE_INTEGER            AllocationSize,
                               __in ULONG                         FileAttributes,
                               __in ULONG                         ShareAccess,
                               __in ULONG                         CreateDisposition,
                               __in ULONG                         CreateOptions,
                               __in_bcount_opt( EaLength ) PVOID  EaBuffer,
                               __in ULONG                         EaLength,
                               __in ULONG                         Flags,
                               __in_opt PIO_DRIVER_CREATE_CONTEXT DriverContext
    );

    ///////////////////////////////////////////////////////////////////////////
    /// Debug Helper

    const char* ConvertCreateShareAccess( __in ULONG ShareAccess );
    const char* ConvertCreateDisposition( __in ULONG CreateDisposition );

    const char* ConvertCreateResultInformation( __in NTSTATUS Status, __in ULONG_PTR Information );

    const char* ConvertFileInformationClassTo( __in const FILE_INFORMATION_CLASS FileInformationClass );

} // nsW32API

#endif // HDR_W32API