#include "fltUtilities.hpp"

#include "bufferMgr.hpp"
#include "contextMgr.hpp"
#include "osInfoMgr.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsUtils
{
	size_t strlength( const wchar_t* str )
	{
		size_t ullLength = 0;
		NTSTATUS Status = RtlStringCchLengthW( str, NTSTRSAFE_MAX_CCH, &ullLength );
		if( !NT_SUCCESS( Status ) )
			return 0;

		return ullLength;
	}

	size_t strlength( const char* str )
	{
		size_t ullLength = 0;
		NTSTATUS Status = RtlStringCchLengthA( str, NTSTRSAFE_MAX_CCH, &ullLength );
		if( !NT_SUCCESS( Status ) )
			return 0;

		return ullLength;
	}

	WCHAR* ReverseFindW( __in_z WCHAR* wszString, WCHAR ch )
	{
		size_t length = strlength( wszString );

		for( size_t i = length; i >= 0; i-- )
		{
			if( wszString[ i ] == ch )
			{
				return &wszString[ i ];
			}
		}

		return NULL;
	}

	WCHAR* ForwardFindW( __in_z WCHAR* wszString, WCHAR ch )
	{
		size_t length = strlength( wszString );

		for( size_t i = 0; i < length; i++ )
		{
			if( wszString[ i ] == ch )
			{
				return &wszString[ i ];
			}
		}

		return NULL;
	}

    WCHAR* UpperWString( WCHAR* wszString )
    {
		ASSERT( wszString != nullptr );
		if( wszString == nullptr )
			return nullptr;

		size_t length = strlength( wszString );

		for( size_t i = 0; i < length; i++ )
			wszString[ i ] = RtlUpcaseUnicodeChar( wszString[ i ] );

		return wszString;
    }

    WCHAR* EndsWithW( WCHAR* wszString, const WCHAR* wszPattern )
    {
		auto lhs = strlength( wszString );
		auto rhs = strlength( wszPattern );

		if( lhs < rhs )
			return NULLPTR;

		auto base = lhs - rhs;
		for( auto idx = base; idx < lhs; ++idx )
		{
			if( RtlUpcaseUnicodeChar( wszString[ idx ] ) != RtlUpcaseUnicodeChar( wszPattern[ idx - base ] ) )
			{
				return NULLPTR;
			}
		}

		return &wszString[ base ];
    }

    bool WildcardMatch_straight( const char* pszString, const char* pszMatch, bool isCaseSensitive /* = false */ )
	{
		const char* mp = NULL;
		const char* cp = NULL;

		ASSERT( pszString != nullptr && pszMatch != nullptr );
		if( pszString == nullptr || pszMatch == nullptr )
			return false;

		while( *pszString )
		{
			if( *pszMatch == char( '*' ) )
			{
				if( !*++pszMatch )
					return true;
				mp = pszMatch;
				cp = pszString + 1;
			}
			else if( ( isCaseSensitive == true && *pszMatch == char( '?' ) ) ||
					 ( isCaseSensitive == false && ( ( *pszMatch == char( '?' ) ) || ( RtlUpcaseUnicodeChar( *pszMatch ) == RtlUpcaseUnicodeChar( *pszString ) ) ) )
					 )
			{
				pszMatch++;
				pszString++;
			}
			else if( !cp )
				return false;
			else
			{
				pszMatch = mp;
				pszString = cp++;
			}
		}

		while( *pszMatch == char( '*' ) )
			pszMatch++;

		return !*pszMatch;
	}

	bool WildcardMatch_straight( const wchar_t* pszString, const wchar_t* pszMatch, bool isCaseSensitive /* = false */ )
	{
		const wchar_t* mp = NULL;
		const wchar_t* cp = NULL;

		ASSERT( pszString != nullptr && pszMatch != nullptr );
		if( pszString == nullptr || pszMatch == nullptr )
			return false;

		while( *pszString )
		{
			if( *pszMatch == wchar_t( '*' ) )
			{
				if( !*++pszMatch )
					return true;
				mp = pszMatch;
				cp = pszString + 1;
			}
			else if( ( isCaseSensitive == true && *pszMatch == wchar_t( '?' ) ) ||
					 ( isCaseSensitive == false && ( ( *pszMatch == wchar_t( '?' ) ) || ( RtlUpcaseUnicodeChar( *pszMatch ) == RtlUpcaseUnicodeChar( *pszString ) ) ) )
					 )
			{
				pszMatch++;
				pszString++;
			}
			else if( !cp )
				return false;
			else
			{
				pszMatch = mp;
				pszString = cp++;
			}
		}

		while( *pszMatch == wchar_t( '*' ) )
			pszMatch++;

		return !*pszMatch;
	}

    NTSTATUS DfAllocateUnicodeString( PUNICODE_STRING String )
    {
		PAGED_CODE();

		ASSERT( NULL != String );
		ASSERT( 0 != String->MaximumLength );

		String->Length = 0;

		String->Buffer = (PWCH)ExAllocatePoolWithTag( NonPagedPool,
												String->MaximumLength,
												'abcd' );

		if( NULL == String->Buffer )
		{

			return STATUS_INSUFFICIENT_RESOURCES;
		}

		return STATUS_SUCCESS;
    }

    void DfFreeUnicodeString( PUNICODE_STRING String )
    {
		PAGED_CODE();

		ASSERT( NULL != String );
		ASSERT( 0 != String->MaximumLength );

		String->Length = 0;

		if( NULL != String->Buffer )
		{

			String->MaximumLength = 0;
			ExFreePool( String->Buffer );
			String->Buffer = NULL;
		}
	}

    ///////////////////////////////////////////////////////////////////////////
	/// FileName

	BOOLEAN FindDriveLetterByDeviceName( UNICODE_STRING* uniDeviceName, WCHAR* wchDriveLetter )
	{
		BOOLEAN IsFindSuccess = FALSE;
		WCHAR  SymbolicNameBuffer[] = L"\\??\\A:";

		do
		{
			if( wchDriveLetter == NULL )
				break;

			*wchDriveLetter = 0;
			WCHAR DriveLetter = '\0';
			UNICODE_STRING  SymbolicName;
			OBJECT_ATTRIBUTES  ObjectAttributes;
			NTSTATUS status;
			HANDLE hObject = NULL;

			for( DriveLetter = L'A'; DriveLetter <= L'Z'; DriveLetter++ )
			{
				SymbolicNameBuffer[ 4 ] = DriveLetter;
				RtlInitUnicodeString( &SymbolicName, SymbolicNameBuffer );
				InitializeObjectAttributes( &ObjectAttributes, &SymbolicName, OBJ_KERNEL_HANDLE, NULL, NULL );

				status = ZwOpenSymbolicLinkObject( &hObject, GENERIC_READ, &ObjectAttributes );

				if( NT_SUCCESS( status ) )
				{
					UNICODE_STRING ObjectName;
					WCHAR ObjectNameBuffer[ 128 ] = { 0, };
					ULONG ReturnedLength;

					RtlInitEmptyUnicodeString( &ObjectName, ObjectNameBuffer, sizeof( WCHAR ) * 128 );
					ZwQuerySymbolicLinkObject( hObject, &ObjectName, &ReturnedLength );

					if( !RtlCompareUnicodeString( uniDeviceName, &ObjectName, TRUE ) )
					{
						IsFindSuccess = TRUE;
						*wchDriveLetter = DriveLetter;
						break;
					}

					ZwClose( hObject );
				}
			}

			if( hObject != NULL )
				ZwClose( hObject );

		} while( false );

		return IsFindSuccess;
	}


	TyGenericBuffer<WCHAR> ExtractFileFullPath( __in PFILE_OBJECT FileObject, __in_opt CTX_INSTANCE_CONTEXT* InstanceContext, __in bool IsInCreate )
	{
		TyGenericBuffer<WCHAR> tyGenericBuffer;
		RtlZeroMemory( &tyGenericBuffer, sizeof( TyGenericBuffer<WCHAR> ) );

		//ASSERT( FileObject != nullptr );

	    ULONG uRequiredBytes = 0;
		PFILE_OBJECT ParentFileObject = IsInCreate == true ? FileObject->RelatedFileObject : NULLPTR;

	    do
        {
			// Get RequiredBytes
			if( ParentFileObject && ParentFileObject->FileName.Length )
				uRequiredBytes = ParentFileObject->FileName.Length;

			uRequiredBytes += FileObject->FileName.Length;
			uRequiredBytes += sizeof( WCHAR );				// Including Null Terminating Character

			// Including Driver Letter or device name
			if( InstanceContext != NULL && InstanceContext->DriveLetter != L'\0' )
				uRequiredBytes += 2 * sizeof( WCHAR );		// Including colon character
			else if( InstanceContext != NULL && InstanceContext->DriveLetter == L'\0' && InstanceContext->DeviceNameBuffer )
				uRequiredBytes += _countof( InstanceContext->DeviceNameBuffer ) * sizeof( WCHAR );

			tyGenericBuffer = AllocateBuffer<WCHAR>( BUFFER_FILENAME, uRequiredBytes );
			if( tyGenericBuffer.Buffer == NULLPTR )
				break;

			RtlZeroMemory( tyGenericBuffer.Buffer, uRequiredBytes );

			ULONG offset = 0;		// byte offset

			if( InstanceContext != NULL )
			{
				if( InstanceContext->DriveLetter != L'\0' )
				{
					tyGenericBuffer.Buffer[ 0 ] = InstanceContext->DriveLetter;
					tyGenericBuffer.Buffer[ 1 ] = L':';
					offset += 2 * sizeof( WCHAR );
				}
				else if( InstanceContext->DriveLetter == L'\0' && InstanceContext->DeviceNameBuffer )
				{
					RtlStringCbCatW( tyGenericBuffer.Buffer, tyGenericBuffer.BufferSize, InstanceContext->DeviceNameBuffer );
					offset += nsUtils::strlength( InstanceContext->DeviceNameBuffer ) * sizeof( WCHAR );
				}
			}

			// ParentFileObject is valid only in Pre-Create (IRP_MJ_CREATE)
			if( IsInCreate == true && 
				( ParentFileObject && ParentFileObject->FileName.Length ) )
			{
				ULONG cbLength = ParentFileObject->FileName.Length % 2 == 0 ? ParentFileObject->FileName.Length : ParentFileObject->FileName.Length - 1;

				if( cbLength > 0 )
				{
					ULONG uJump = 0;
					if( ParentFileObject->FileName.Buffer[ 0 ] == L'\\' &&
						ParentFileObject->FileName.Buffer[ 1 ] == L'?' &&
						ParentFileObject->FileName.Buffer[ 2 ] == L'?' &&
						ParentFileObject->FileName.Buffer[ 3 ] == L'\\' )
					{
						uJump = 6;
						cbLength -= 6 * sizeof( WCHAR );        // jump 6 characters( \??\X: )
					}

					RtlCopyMemory( ( ( PBYTE )tyGenericBuffer.Buffer ) + offset, ParentFileObject->FileName.Buffer + uJump, cbLength );
					ULONG index = ( offset + cbLength ) / sizeof( WCHAR );
					index -= 1;
					offset += cbLength;

					// 경로의 마지막에 \ 로 끝나지 않으면 \ 를 추가한다, ParentFileObject 는 항상 경로부분이고, 파일이름을 밑에서 붙인다
					if( tyGenericBuffer.Buffer[ index ] != L'\\' )
					{
						index += 1;
						tyGenericBuffer.Buffer[ index ] = L'\\';
						offset += sizeof( WCHAR );
					}
				}
			}

			RtlCopyMemory( Add2Ptr( tyGenericBuffer.Buffer, offset ),
						   FileObject->FileName.Buffer,
						   FileObject->FileName.Length );

        } while( false );

        do
        {
			/////////////////////////////////////////////////////////////////////////////
			///// Post-Processing

			if( tyGenericBuffer.Buffer == NULLPTR )
				break;

		//if( InstanceContext != NULL && InstanceContext->VolumeProperties.DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM )
		//{
		//	size_t base = 0;
		//	if( InstanceContext->DriveLetter != L'\0' )
		//	{
		//		base = 2;
		//	}
		//	else if( InstanceContext->DriveLetter == L'\0' && InstanceContext->DeviceNameBuffer )
		//	{
		//		base = wcslen( InstanceContext->DeviceNameBuffer );
		//	}

		//	//\;Y:000000000000ecb4\192.168.1.108\cifs_test_share_rw\dd\1.txt
		//	//\;hgfs\;Z:0000000000016d1a\vmware-host\Shared Folders\test_share_rw\test.txt
		//	if( ( tyGenericBuffer.uBufferSize >= 2 * sizeof( WCHAR ) ) &&
		//		( tyGenericBuffer.buffer[ base + 1 ] == L';' ) )
		//	{
		//		WCHAR* p = ForwardFindW( &tyGenericBuffer.buffer[ base + 1 ], L'\\' );
		//		if( p != NULL )
		//		{
		//			if( p[ 1 ] == L';' )
		//			{
		//				p = ForwardFindW( p + 1, L'\\' );

		//				if( p )
		//				{
		//					// NULL 문자를 포함하여 이동
		//					RtlMoveMemory( tyGenericBuffer.buffer, p, ( wcslen( p ) + 1 ) * sizeof( WCHAR ) );
		//				}
		//				else
		//				{
		//					// TODO: 생각하지 못 한 조합의 경로 문자열이다.
		//					KdPrint( ( "[iMonFSD] %s|Unexpected Path=%ws\n", __FUNCTION__, tyGenericBuffer.buffer ) );
		//				}
		//			}
		//			else
		//			{
		//				// NULL 문자를 포함하여 이동
		//				RtlMoveMemory( tyGenericBuffer.buffer, p, ( wcslen( p ) + 1 ) * sizeof( WCHAR ) );
		//			}
		//		}
		//	}
		//}

        } while( false );

		return tyGenericBuffer;
	}

    NTSTATUS DfGetVolumeGuidName( PCFLT_RELATED_OBJECTS FltObjects, PUNICODE_STRING VolumeGuidName )
    {
		NTSTATUS status;
		PUNICODE_STRING sourceGuidName;
		PCTX_INSTANCE_CONTEXT instanceContext = NULL;

		PAGED_CODE();

		//
		//  Obtain an instance context. Target is NULL for instance context, as
		//  the FLT_INSTANCE can be obtained from the FltObjects.
		//

		status = CtxGetOrSetContext( FltObjects,
									NULL,
									(PFLT_CONTEXT*)&instanceContext,
									FLT_INSTANCE_CONTEXT );

		if( NT_SUCCESS( status ) )
		{

			//
			//  sourceGuidName is the source from where we'll copy the volume
			//  GUID name. Hopefully the name is present in the instance context
			//  already (buffer is not NULL) so we'll try to use that.
			//

			sourceGuidName = &instanceContext->VolumeGUIDName;

			if( NULL == sourceGuidName->Buffer )
			{

				//
				//  The volume GUID name is not cached in the instance context
				//  yet, so we will have to query the volume for it and put it
				//  in the instance context, so future queries can get it directly
				//  from the context.
				//

				UNICODE_STRING tempString;

				//
				//  Add sizeof(WCHAR) so it's possible to add a trailing backslash here.
				//

                tempString.MaximumLength = VOLUME_GUID_NAME_SIZE * sizeof( WCHAR ) + sizeof( WCHAR );

				status = DfAllocateUnicodeString( &tempString );

				if( !NT_SUCCESS( status ) )
				{

					//DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
					//			  "delete!%s: DfAllocateUnicodeString returned 0x%08x!\n",
					//			  __FUNCTION__,
					//			  status );

					return status;
				}

				//  while there is no guid name, don't do the open by id deletion logic.
				//  (it's actually better to defer obtaining the volume GUID name up to
				//   the point when we actually need it, in the open by ID scenario.)
				status = FltGetVolumeGuidName( FltObjects->Volume,
											   &tempString,
											   NULL );

				if( !NT_SUCCESS( status ) )
				{

					//DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
					//			  "delete!%s: FltGetVolumeGuidName returned 0x%08x!\n",
					//			  __FUNCTION__,
					//			  status );

					DfFreeUnicodeString( &tempString );

					return status;
				}

				//
				//  Append trailing backslash.
				//

				RtlAppendUnicodeToString( &tempString, L"\\" );

				//
				//  Now set the sourceGuidName to the tempString. It is okay to
				//  set Length and MaximumLength with no synchronization because
				//  those will always be the same value (size of a volume GUID
				//  name with an extra trailing backslash).
				//

				sourceGuidName->Length = tempString.Length;
				sourceGuidName->MaximumLength = tempString.MaximumLength;

				//
				//  Setting the buffer, however, requires some synchronization,
				//  because another thread might be attempting to do the same,
				//  and even though they're exactly the same string, they're
				//  different allocations (buffers) so if the other thread we're
				//  racing with manages to set the buffer before us, we need to
				//  free our temporary string buffer.
				//

				InterlockedCompareExchangePointer( &sourceGuidName->Buffer,
												   tempString.Buffer,
												   NULL );

				if( sourceGuidName->Buffer != tempString.Buffer )
				{

					//
					//  We didn't manage to set the buffer, so let's free the
					//  tempString buffer.
					//

					DfFreeUnicodeString( &tempString );
				}
			}

			//
			//  sourceGuidName now contains the correct GUID name, so copy that
			//  to the caller string.
			//

			RtlCopyUnicodeString( VolumeGuidName, sourceGuidName );

			//
			//  We're done with the instance context.
			//

			FltReleaseContext( instanceContext );
		}

		return status;
    }

    NTSTATUS DfBuildFileIdString( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects,        PCTX_STREAM_CONTEXT StreamContext, PUNICODE_STRING String )
    {
		NTSTATUS status;

		PAGED_CODE();

		ASSERT( NULL != String );

		//
		//  We'll compose the string with:
		//  1. The volume GUID name.
		//  2. A backslash
		//  3. The File ID.
		//

		//
		//  Make sure the file ID is loaded in the StreamContext.  Note that if the
		//  file has been deleted DfGetFileId will return STATUS_FILE_DELETED.
		//  Since we're interested in detecting whether the file has been deleted
		//  that's fine; the open-by-ID will not actually take place.  We have to
		//  ensure it is loaded before building the string length below since we
		//  may get either a 64-bit or 128-bit file ID back.
		//

		status = DfGetFileId( Data, StreamContext );

		if( !NT_SUCCESS( status ) )
		{

			return status;
		}

		//
		//  First add the lengths of 1, 2, 3 and allocate accordingly.
		//  Note that ReFS understands both 64- and 128-bit file IDs when opening
		//  by ID, so whichever size we get back from DfSizeofFileId will work.
		//

		String->MaximumLength = VOLUME_GUID_NAME_SIZE * sizeof( WCHAR ) +
			sizeof( WCHAR ) +
			DfSizeofFileId( StreamContext->FileId );

		status = DfAllocateUnicodeString( String );

		if( !NT_SUCCESS( status ) )
		{

			return status;
		}

		//
		//  Now obtain the volume GUID name with a trailing backslash (1 + 2).
		//

		// obtain volume GUID name here and cache it in the InstanceContext.
		status = DfGetVolumeGuidName( FltObjects,
									  String );

		if( !NT_SUCCESS( status ) )
		{

			DfFreeUnicodeString( String );

			return status;
		}

		//
		//  Now append the file ID to the end of the string.
		//

		RtlCopyMemory( Add2Ptr( String->Buffer, String->Length ),
					   &StreamContext->FileId,
					   DfSizeofFileId( StreamContext->FileId ) );

		String->Length += DfSizeofFileId( StreamContext->FileId );

		ASSERT( String->Length == String->MaximumLength );

		return status;
    }

    NTSTATUS DfDetectDeleteByFileId( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PCTX_STREAM_CONTEXT StreamContext )
    {
		NTSTATUS status;
		UNICODE_STRING fileIdString;
		HANDLE handle;
		OBJECT_ATTRIBUTES objectAttributes;
		IO_STATUS_BLOCK ioStatus;
		IO_DRIVER_CREATE_CONTEXT driverCreateContext;

		PAGED_CODE();

		//
		//  First build the file ID string.  Note that this may fail with STATUS_FILE_DELETED
		//  and short-circuit our open-by-ID.  Since we're really trying to see if
		//  the file is deleted, that's perfectly okay.
		//

		status = DfBuildFileIdString( Data,
									  FltObjects,
									  StreamContext,
									  &fileIdString );

		if( !NT_SUCCESS( status ) )
		{

			return status;
		}

		InitializeObjectAttributes( &objectAttributes,
									&fileIdString,
									OBJ_KERNEL_HANDLE,
									NULL,
									NULL );

		//
		//  It is important to initialize the IO_DRIVER_CREATE_CONTEXT structure's
		//  TxnParameters. We'll always want to do this open on behalf of a
		//  transaction because opening the file by ID is the method we use to
		//  detect if the whole file still exists when we're in a transaction.
		//

	    // TODO: Depends on OS Version( Support WinXP )

		if( nsUtils::VerifyVersionInfoEx( 6, ">=" ) == true )
		{
			IoInitializeDriverCreateContext( &driverCreateContext );
			driverCreateContext.TxnParameters = nsW32API::NtOsKrnlAPIMgr.pfnIoGetTransactionParameterBlock( Data->Iopb->TargetFileObject );

            status = nsW32API::FltMgrAPIMgr.pfnFltCreateFileEx2( GlobalContext.Filter,
                                                                 Data->Iopb->TargetInstance,
                                                                 &handle,
                                                                 NULL,
                                                                 FILE_READ_ATTRIBUTES,
                                                                 &objectAttributes,
                                                                 &ioStatus,
                                                                 ( PLARGE_INTEGER )NULL,
                                                                 0L,
                                                                 FILE_SHARE_VALID_FLAGS,
                                                                 FILE_OPEN,
                                                                 FILE_OPEN_REPARSE_POINT | FILE_OPEN_BY_FILE_ID,
                                                                 ( PVOID )NULL,
                                                                 0L,
                                                                 IO_IGNORE_SHARE_ACCESS_CHECK,
                                                                 &driverCreateContext );
		}
		else
		{
			if( nsW32API::FltMgrAPIMgr.pfnFltCreateFileEx != NULLPTR )
			{
                status = nsW32API::FltMgrAPIMgr.pfnFltCreateFileEx( GlobalContext.Filter,
                                                                    Data->Iopb->TargetInstance,
                                                                    &handle,
                                                                    NULL,
                                                                    FILE_READ_ATTRIBUTES,
                                                                    &objectAttributes,
                                                                    &ioStatus,
                                                                    ( PLARGE_INTEGER )NULL,
                                                                    0L,
                                                                    FILE_SHARE_VALID_FLAGS,
                                                                    FILE_OPEN,
                                                                    FILE_OPEN_REPARSE_POINT | FILE_OPEN_BY_FILE_ID,
                                                                    ( PVOID )NULL,
                                                                    0L,
                                                                    IO_IGNORE_SHARE_ACCESS_CHECK );
			}
			else
			{
                status = FltCreateFile( GlobalContext.Filter,
                                        Data->Iopb->TargetInstance,
                                        &handle,
                                        FILE_READ_ATTRIBUTES,
                                        &objectAttributes,
                                        &ioStatus,
                                        ( PLARGE_INTEGER )NULL,
                                        0L,
                                        FILE_SHARE_VALID_FLAGS,
                                        FILE_OPEN,
                                        FILE_OPEN_REPARSE_POINT | FILE_OPEN_BY_FILE_ID,
                                        ( PVOID )NULL,
                                        0L,
                                        IO_IGNORE_SHARE_ACCESS_CHECK );
			}
		}

		if( NT_SUCCESS( status ) )
		{

			status = FltClose( handle );
			ASSERT( NT_SUCCESS( status ) );
		}

		DfFreeUnicodeString( &fileIdString );

		return status;
    }

    NTSTATUS DfGetFileNameInformation( PFLT_CALLBACK_DATA Data, PCTX_STREAM_CONTEXT StreamContext )
    {
		NTSTATUS status;
		PFLT_FILE_NAME_INFORMATION oldNameInfo;
		PFLT_FILE_NAME_INFORMATION newNameInfo;

		PAGED_CODE();

		//
		//  FltGetFileNameInformation - this is enough for a file name.
		//

		status = FltGetFileNameInformation( Data,
											( FLT_FILE_NAME_OPENED |
											  FLT_FILE_NAME_QUERY_DEFAULT ),
											&newNameInfo );

		if( !NT_SUCCESS( status ) )
		{
			return status;
		}

		//
		//  FltParseFileNameInformation - this fills in the other gaps, like the
		//  stream name, if present.
		//

		status = FltParseFileNameInformation( newNameInfo );

		if( !NT_SUCCESS( status ) )
		{
			return status;
		}

		//
		//  Now that we have a good NameInfo, set it in the context, replacing
		//  the previous one.
		//

		oldNameInfo = (PFLT_FILE_NAME_INFORMATION)InterlockedExchangePointer( &StreamContext->NameInfo,
												  newNameInfo );

		if( NULL != oldNameInfo )
		{

			FltReleaseFileNameInformation( oldNameInfo );
		}

		return status;
    }

    NTSTATUS DfGetFileId( PFLT_CALLBACK_DATA Data, PCTX_STREAM_CONTEXT StreamContext )
    {
		NTSTATUS status = STATUS_SUCCESS;
		FILE_INTERNAL_INFORMATION fileInternalInformation;

		PAGED_CODE();

		//
		//  Only query the file system for the file ID for the first time.
		//  This is just an optimization.  It doesn't need any real synchronization
		//  because file IDs don't change.
		//

		if( !StreamContext->FileIdSet )
		{

			//
			//  Querying for FileInternalInformation gives you the file ID.
			//

			status = FltQueryInformationFile( Data->Iopb->TargetInstance,
											  Data->Iopb->TargetFileObject,
											  &fileInternalInformation,
											  sizeof( FILE_INTERNAL_INFORMATION ),
											  FileInternalInformation,
											  NULL );

			if( NT_SUCCESS( status ) )
			{

				//
				//  ReFS uses 128-bit file IDs.  FileInternalInformation supports 64-
				//  bit file IDs.  ReFS signals that a particular file ID can only
				//  be represented in 128 bits by returning FILE_INVALID_FILE_ID as
				//  the file ID.  In that case we need to use FileIdInformation.
				//

				if( fileInternalInformation.IndexNumber.QuadPart == nsW32API::FILE_INVALID_FILE_ID )
				{
                    nsW32API::FILE_ID_INFORMATION fileIdInformation;

					status = FltQueryInformationFile( Data->Iopb->TargetInstance,
													  Data->Iopb->TargetFileObject,
													  &fileIdInformation,
													  sizeof( nsW32API::FILE_ID_INFORMATION ),
                                                      (FILE_INFORMATION_CLASS)nsW32API::FileIdInformation,
													  NULL );

					if( NT_SUCCESS( status ) )
					{

						//
						//  We don't use DfSizeofFileId() here because we are not
						//  measuring the size of a DF_FILE_REFERENCE.  We know we have
						//  a 128-bit value.
						//

						RtlCopyMemory( &StreamContext->FileId,
									   &fileIdInformation.FileId,
									   sizeof( StreamContext->FileId ) );

						//
						//  Because there's (currently) no support for 128-bit values in
						//  the compiler we need to ensure the setting of the ID and our
						//  remembering that the file ID was set occur in the right order.
						//

						KeMemoryBarrier();

						StreamContext->FileIdSet = TRUE;
					}

				}
				else
				{

					StreamContext->FileId.FileId64.Value = fileInternalInformation.IndexNumber.QuadPart;
					StreamContext->FileId.FileId64.UpperZeroes = 0ll;

					//
					//  Because there's (currently) no support for 128-bit values in
					//  the compiler we need to ensure the setting of the ID and our
					//  remembering that the file ID was set occur in the right order.
					//

					KeMemoryBarrier();

					StreamContext->FileIdSet = TRUE;
				}
			}
		}

		return status;
    }

    NTSTATUS DfIsFileDeleted( PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PCTX_STREAM_CONTEXT StreamContext, BOOLEAN IsTransaction )
    {
		NTSTATUS status = STATUS_SUCCESS;
		FILE_OBJECTID_BUFFER fileObjectIdBuf;

		FLT_FILESYSTEM_TYPE fileSystemType = FLT_FSTYPE_UNKNOWN;

		PAGED_CODE();

		//
		//  We need to know whether we're on ReFS or NTFS.
		//

		if( nsUtils::VerifyVersionInfoEx( 6, ">="	) == true )
		{
			status = nsW32API::FltMgrAPIMgr.pfnFltGetFileSystemType( FltObjects->Instance, ( nsW32API::PFLT_FILESYSTEM_TYPE ) & fileSystemType );

			if( status != STATUS_SUCCESS )
			{

				return status;
			}
		}
		else
		{
			PCTX_INSTANCE_CONTEXT InstanceContext = NULLPTR;

			CtxGetContext( FltObjects, NULLPTR, FLT_INSTANCE_CONTEXT, ( PFLT_CONTEXT* )&InstanceContext );

			if( InstanceContext != NULLPTR )
			{
				fileSystemType = InstanceContext->VolumeFileSystemType;
				CtxReleaseContext( InstanceContext );
			}
		}

		//
		//  FSCTL_GET_OBJECT_ID does not return STATUS_FILE_DELETED if the
		//  file was deleted in a transaction, and this is why we need another
		//  method for detecting if the file is still present: opening by ID.
		//
		//  If we're on ReFS we also need to open by file ID because ReFS does not
		//  support object IDs.
		//

		if( IsTransaction ||
			( fileSystemType == nsW32API::FLT_FSTYPE_REFS ) )
		{

			status = DfDetectDeleteByFileId( Data,
											 FltObjects,
											 StreamContext );

			switch( status )
			{

				case STATUS_INVALID_PARAMETER:

					//
					//  The file was deleted. In this case, trying to open it
					//  by ID returns STATUS_INVALID_PARAMETER.
					//

					return STATUS_FILE_DELETED;

				case STATUS_DELETE_PENDING:

					//
					//  In this case, the main file still exists, but is in
					//  a delete pending state, so we return STATUS_SUCCESS,
					//  signaling it still exists and wasn't deleted by this
					//  operation.
					//

					return STATUS_SUCCESS;

				default:

					return status;
			}

		}
		else
		{

			//
			//  When not in a transaction, attempting to get the object ID of the
			//  file is a cheaper alternative compared to opening the file by ID.
			//

			status = FltFsControlFile( Data->Iopb->TargetInstance,
									   Data->Iopb->TargetFileObject,
									   FSCTL_GET_OBJECT_ID,
									   NULL,
									   0,
									   &fileObjectIdBuf,
									   sizeof( FILE_OBJECTID_BUFFER ),
									   NULL );

			switch( status )
			{

				case STATUS_OBJECTID_NOT_FOUND:

					//
					//  Getting back STATUS_OBJECTID_NOT_FOUND means the file
					//  still exists, it just doesn't have an object ID.

					return STATUS_SUCCESS;

				default:

					//
					//  Else we just get back STATUS_FILE_DELETED if the file
					//  doesn't exist anymore, or some error status, so no
					//  status conversion is necessary.
					//

					NOTHING;
			}
		}

		return status;
    }
} // nsUtils
