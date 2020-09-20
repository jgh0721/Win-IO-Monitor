#include "fltCmnLibs_base.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

namespace nsUtils
{
    PVOID MakeUserBuffer( PFLT_CALLBACK_DATA Data )
    {
        NTSTATUS                Status;
        PMDL                    MdlAddress;
        PVOID                   Buffer;
        PMDL*                   MdlAddressPointer;
        PVOID*                  BufferPointer;
        PULONG                  Length;
        LOCK_OPERATION          DesiredAccess;

        Status = FltDecodeParameters( Data, &MdlAddressPointer, &BufferPointer, &Length, &DesiredAccess );

        if( !NT_SUCCESS( Status ) )
        {
            return NULL;
        }

        MdlAddress = *MdlAddressPointer;
        Buffer = *BufferPointer;

        if( MdlAddress )
            Buffer = MmGetSystemAddressForMdlSafe( MdlAddress, NormalPagePriority );

        return Buffer;
    }
} // nsUtils
