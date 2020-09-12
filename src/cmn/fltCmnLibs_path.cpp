#include "fltCmnLibs_path.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsUtils
{
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

} // nsUtils