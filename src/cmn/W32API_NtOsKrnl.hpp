#ifndef HDR_W32API_NTOSKRNL
#define HDR_W32API_NTOSKRNL

#include "fltBase.hpp"
#include "W32API_Base.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsW32API
{
    DYNLOAD_BEGIN_CLASS( CNtOsKrnlAPI, L"NtOsKrnl.exe" )

        // The PsSetLoadImageNotifyRoutine routine registers a driver-supplied callback that is subsequently notified whenever an image is loaded (or mapped into memory).
        DYNLOAD_FUNC_WITH_01( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, NTAPI, PsSetLoadImageNotifyRoutine, IN PLOAD_IMAGE_NOTIFY_ROUTINE, NotifyRoutine );

        // The PsSetCreateProcessNotifyRoutine routine adds a driver-supplied callback routine to, or removes it from, a list of routines to be called whenever a process is created or deleted.
        DYNLOAD_FUNC_WITH_02( STATUS_INVALID_PARAMETER, NTSTATUS, NTAPI, PsSetCreateProcessNotifyRoutine, IN PCREATE_PROCESS_NOTIFY_ROUTINE, NotifyRoutine, IN BOOLEAN, Remove );

        // The PsSetCreateProcessNotifyRoutineEx routine registers or removes a callback routine that notifies the caller when a process is created or exits.
        DYNLOAD_FUNC_WITH_02( STATUS_ACCESS_DENIED, NTSTATUS, NTAPI, PsSetCreateProcessNotifyRoutineEx, IN PCREATE_PROCESS_NOTIFY_ROUTINE_EX, NotifyRoutine, IN BOOLEAN, Remove );

        // The IoGetTransactionParameterBlock routine returns the transaction parameter block for a transacted file operation.
        DYNLOAD_FUNC_WITH_01( NULL, PTXN_PARAMETER_BLOCK, NTAPI, IoGetTransactionParameterBlock, __in PFILE_OBJECT, FileObject );

        /*!
         *   This routine returns the ImageFileName information from the process, if available.  This is a "lazy evaluation" wrapper
             around SeInitializeProcessAuditName.  If the image file name information has already been computed, then this call simply
             allocates and returns a UNICODE_STRING with this information.  Otherwise, the function determines the name, stores the name in the
             EPROCESS structure, and then allocates and returns a UNICODE_STRING.  Caller must free the memory returned in pImageFileName.
         */
        DYNLOAD_FUNC_WITH_02( STATUS_INSUFFICIENT_RESOURCES, NTSTATUS, NTAPI, SeLocateProcessImageName, __in PEPROCESS, Process, __deref_out PUNICODE_STRING*, pImageFileName );

        DYNLOAD_FUNC_WITH_05( STATUS_INVALID_PARAMETER, NTSTATUS, NTAPI, ZwQueryInformationProcess, __in HANDLE, ProcessHandle, __in PROCESSINFOCLASS, ProcessInformationClass, __out_bcount( ProcessInformationLength ) PVOID, ProcessInformation, __in ULONG, ProcessInformationLength, __out_opt PULONG, ReturnLength );

        /*!
            The IoReplaceFileObjectName routine replaces the name of a file object.

            Windows 7 ~
        */
        DYNLOAD_FUNC_WITH_03( STATUS_INVALID_PARAMETER, NTSTATUS, NTAPI, IoReplaceFileObjectName, __in PFILE_OBJECT, FileObject, __in PWSTR, NewFileName, __in USHORT, FileNameLength );

        /*!
            The MmDoesFileHaveUserWritableReferencesfunction returns the number of writable references for a file object.

            Windows Vista ~
        */
        DYNLOAD_FUNC_WITH_01( 0, ULONG, NTAPI, MmDoesFileHaveUserWritableReferences, __in PSECTION_OBJECT_POINTERS, SectionPointer );
        /*!
            The FsRtlChangeBackingFileObject routine replaces the current file object with a new file object

            Windows Vista ~
        */
        DYNLOAD_FUNC_WITH_04( STATUS_NOT_IMPLEMENTED, NTSTATUS, NTAPI, FsRtlChangeBackingFileObject, __in_opt PFILE_OBJECT, CurrentFileObject, __in PFILE_OBJECT, NewFileObject, __in FSRTL_CHANGE_BACKING_TYPE, ChangeBackingType, __in ULONG, Flags );

    DYNLOAD_END_CLASS();

} // nsW32API

extern nsW32API::CNtOsKrnlAPI GlobalNtOsKrnlMgr;

#endif // HDR_W32API_NTOSKRNL