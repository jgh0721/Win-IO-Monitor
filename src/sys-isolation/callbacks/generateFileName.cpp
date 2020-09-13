#include "generateFileName.hpp"

#include "utilities/osInfoMgr.hpp"
#include "utilities/contextMgr_Defs.hpp"

#include "W32API.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

NTSTATUS FLTAPI FilterGenerateFileName( PFLT_INSTANCE Instance, PFILE_OBJECT FileObject, PFLT_CALLBACK_DATA CallbackData,
                                        FLT_FILE_NAME_OPTIONS NameOptions, PBOOLEAN CacheFileNameInformation, PFLT_NAME_CONTROL FileName )
{
    /*!
     *
     * FileObject FO_OPENED_CASE_SENSITIVE 또는 FLT_CALLBACK_DATA->Iopb->OperationFlags SL_CASE_SENSITIVE
     */

    NTSTATUS status = STATUS_SUCCESS;
    PFLT_FILE_NAME_INFORMATION belowFileName = NULL;

    __try
    {
        //
        //  We expect to only get requests for opened and short names.
        //  If we get something else, fail. Please note that it is in
        //  fact possible that if we get a normalized name request the
        //  code would work because it's not really doing anything other 
        //  than calling FltGetFileNameInformation which would handle the
        //  normalized name request just fine. However, in a real name 
        //  provider this might require a different implementation. 
        //

        if( !FlagOn( NameOptions, FLT_FILE_NAME_OPENED ) &&
            !FlagOn( NameOptions, FLT_FILE_NAME_SHORT ) &&
            !FlagOn( NameOptions, FLT_FILE_NAME_NORMALIZED ))       // In Win8~ generate directly normalize callback
        {

            ASSERT( !"we have a received a request for an unknown format. investigate!" );

            return STATUS_NOT_SUPPORTED;
        }

        //  
        // First we need to get the file name. We're going to call   
        // FltGetFileNameInformation below us to get the file name from FltMgr.   
        // However, it is possible that we're called by our own minifilter for   
        // the name so in order to avoid an infinite loop we must make sure to   
        // remove the flag that tells FltMgr to query this same minifilter.   
        //  
        ClearFlag( NameOptions, FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER );
        SetFlag( NameOptions, FLT_FILE_NAME_DO_NOT_CACHE );

        //  
        // this will be called for FltGetFileNameInformationUnsafe as well and  
        // in that case we don't have a CallbackData, which changes how we call   
        // into FltMgr.  
        //  
        if( CallbackData == NULL )
        {
            //  
            // This must be a call from FltGetFileNameInformationUnsafe.  
            // However, in order to call FltGetFileNameInformationUnsafe the   
            // caller MUST have an open file (assert).  
            //  
            ASSERT( FileObject->FsContext != NULL );
            status = FltGetFileNameInformationUnsafe( FileObject,
                                                      Instance,
                                                      NameOptions,
                                                      &belowFileName );
            if( !NT_SUCCESS( status ) )
            {
                __leave;
            }
        }
        else
        {
            //  
            // We have a callback data, we can just call FltMgr.  
            //  
            status = FltGetFileNameInformation( CallbackData,
                                                NameOptions,
                                                &belowFileName );
            if( !NT_SUCCESS( status ) )
            {
                __leave;
            }
        }
        //  
        // At this point we have a name for the file (the opened name) that   
        // we'd like to return to the caller. We must make sure we have enough   
        // buffer to return the name or we must grow the buffer. This is easy   
        // when using the right FltMgr API.  
        //  
        status = FltCheckAndGrowNameControl( FileName, belowFileName->Name.Length );
        if( !NT_SUCCESS( status ) )
        {
            __leave;
        }
        //  
        // There is enough buffer, copy the name from our local variable into  
        // the caller provided buffer.  
        //  
        RtlCopyUnicodeString( &FileName->Name, &belowFileName->Name );
        //  
        // And finally tell the user they can cache this name.  
        //  
        *CacheFileNameInformation = TRUE;
    }
    __finally
    {
        if( belowFileName != NULL )
        {
            FltReleaseFileNameInformation( belowFileName );
        }
    }

    return status;
}

NTSTATUS FLTAPI FilterNormalizeNameComponent( PFLT_INSTANCE Instance, PCUNICODE_STRING ParentDirectory,
                                              USHORT VolumeNameLength, PCUNICODE_STRING Component, PFILE_NAMES_INFORMATION ExpandComponentName,
                                              ULONG ExpandComponentNameLength, FLT_NORMALIZE_NAME_FLAGS Flags, PVOID* NormalizationContext )
{
    //  
      // This is just a thin wrapper over FilterNormalizeNameComponentEx.   
      // Please note that we don't pass in a FILE_OBJECT because we don't   
      // have one.   
      //  
    return FilterNormalizeNameComponentEx( Instance,
                                           NULL,
                                           ParentDirectory,
                                           VolumeNameLength,
                                           Component,
                                           ExpandComponentName,
                                           ExpandComponentNameLength,
                                           Flags,
                                           NormalizationContext );
}

NTSTATUS FLTAPI FilterNormalizeNameComponentEx( PFLT_INSTANCE Instance, PFILE_OBJECT FileObject,
                                                PCUNICODE_STRING ParentDirectory, USHORT VolumeNameLength, PCUNICODE_STRING Component,
                                                PFILE_NAMES_INFORMATION ExpandComponentName, ULONG ExpandComponentNameLength, FLT_NORMALIZE_NAME_FLAGS Flags,
                                                PVOID* NormalizationContext )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE parentDirHandle = NULL;
    OBJECT_ATTRIBUTES parentDirAttributes;
    BOOLEAN isDestinationFile;
    BOOLEAN isCaseSensitive;
    IO_STATUS_BLOCK ioStatus;

    // 아래 2개의 변수는 비스타 이상에서만 사용됨
    IO_DRIVER_CREATE_CONTEXT driverContext;
    PTXN_PARAMETER_BLOCK txnParameter = NULL;

    __try
    {
        //  
        // Initialize the boolean variables. we only use the case sensitivity  
        // one but we initialize both just to point out that you can tell   
        // whether Component is a "destination" (target of a rename or hardlink  
        // creation operation).  
        //  
        isCaseSensitive = BooleanFlagOn( Flags,
                                         FLTFL_NORMALIZE_NAME_CASE_SENSITIVE );
        isDestinationFile = BooleanFlagOn( Flags,
                                           FLTFL_NORMALIZE_NAME_DESTINATION_FILE_NAME );
        //  
        // Open the parent directory for the component we're trying to   
        // normalize. It might need to be a case sensitive operation so we   
        // set that flag if necessary.  
        //  
        InitializeObjectAttributes( &parentDirAttributes,
                                    ( PUNICODE_STRING )ParentDirectory,
                                    OBJ_KERNEL_HANDLE | ( isCaseSensitive ? OBJ_CASE_INSENSITIVE : 0 ),
                                    NULL,
                                    NULL );

        if( nsUtils::VerifyVersionInfoEx( 6, ">=" ) == true )
        {
            //  
            // In Vista and newer this must be done in the context of the same  
            // transaction the FileObject belongs to.      
            //  
            IoInitializeDriverCreateContext( &driverContext );
            txnParameter = GlobalNtOsKrnlMgr.IoGetTransactionParameterBlock( FileObject );
            driverContext.TxnParameters = txnParameter;
            status = GlobalFltMgr.FltCreateFileEx2( GlobalContext.Filter,
                                                    Instance,
                                                    &parentDirHandle,
                                                    NULL,
                                                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                                    &parentDirAttributes,
                                                    &ioStatus,
                                                    0,
                                                    FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
                                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                    FILE_OPEN,
                                                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                                                    NULL,
                                                    0,
                                                    IO_IGNORE_SHARE_ACCESS_CHECK,
                                                    &driverContext );
        }
        else
        {
            //  
            // preVista we don't care about transactions  
            //  
            status = FltCreateFile( GlobalContext.Filter,
                                    Instance,
                                    &parentDirHandle,
                                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                    &parentDirAttributes,
                                    &ioStatus,
                                    0,
                                    FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    FILE_OPEN,
                                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                                    NULL,
                                    0,
                                    IO_IGNORE_SHARE_ACCESS_CHECK );
        }

        if( !NT_SUCCESS( status ) )
        {
            __leave;
        }

        //  
        // Now that we have a handle to the parent directory of Component, we  
        // need to query its long name from the file system. We're going to use  
        // ZwQueryDirectoryFile because the handle we have for the directory   
        // was opened with FltCreateFile and so targeting should work just fine.  
        //  
        status = ZwQueryDirectoryFile( parentDirHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &ioStatus,
                                       ExpandComponentName,
                                       ExpandComponentNameLength,
                                       FileNamesInformation,
                                       TRUE,
                                       ( PUNICODE_STRING )Component,
                                       TRUE );
    }
    __finally
    {
        if( parentDirHandle != NULL )
        {
            FltClose( parentDirHandle );
        }
    }

    return status;
}

void FLTAPI FilterNormalizeContextCleanup( PVOID* NormalizationContext )
{
    UNREFERENCED_PARAMETER( NormalizationContext );
}
