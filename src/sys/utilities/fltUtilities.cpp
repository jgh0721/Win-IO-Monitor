#include "fltUtilities.hpp"

#include "bufferMgr.hpp"

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


	TyGenericBuffer<WCHAR> ExtractFileFullPath( __in PFILE_OBJECT FileObject, __in_opt PCTX_INSTANCE_CONTEXT InstanceContext, __in bool IsInCreate )
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

} // nsUtils
