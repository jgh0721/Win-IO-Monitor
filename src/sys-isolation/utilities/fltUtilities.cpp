#include "fltUtilities.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

FAST_IO_POSSIBLE CheckIsFastIOPossible( FCB* Fcb )
{
    FAST_IO_POSSIBLE FastIoPossible = FastIoIsNotPossible;

    ASSERT( Fcb != NULLPTR );
    if( Fcb == NULLPTR )
        return FastIoPossible;

    // based on MSDN CDFS sample
    //
    //  The following macro is used to set the fast i/o possible bits in the
    //  FsRtl header.
    //
    //      FastIoIsNotPossible - If the Fcb is bad or there are oplocks on the file.
    //
    //      FastIoIsQuestionable - If there are file locks.
    //
    //      FastIoIsPossible - In all other cases.
    //

    ExAcquireFastMutex( Fcb->AdvFcbHeader.FastMutex );

    do
    {
        if( FltOplockIsFastIoPossible( &Fcb->FileOplock ) )
        {
            if( FsRtlAreThereCurrentFileLocks( &Fcb->FileLock ) != FALSE )
                FastIoPossible = FastIoIsQuestionable;
            else
                FastIoPossible = FastIoIsPossible;
        }
        else
        {
            FastIoPossible = FastIoIsNotPossible;
        }

    } while( false );

    ExReleaseFastMutex( Fcb->AdvFcbHeader.FastMutex );

    return FastIoPossible;
}

PVOID FltMapUserBuffer( PFLT_CALLBACK_DATA Data )
{
    PVOID           UserBuffer = NULLPTR;
    NTSTATUS        Status = STATUS_SUCCESS;

    PMDL*           MdlAddressPointer = NULLPTR;
    PVOID*          BufferPointer = NULLPTR;
    PULONG          Length = NULLPTR;
    LOCK_OPERATION  DesiredAccess;

    do
    {
        Status = FltDecodeParameters( Data, &MdlAddressPointer, &BufferPointer, &Length, &DesiredAccess );
        if( !NT_SUCCESS( Status ) )
            break;

        if( MdlAddressPointer != NULLPTR && *MdlAddressPointer != NULLPTR )
            UserBuffer = MmGetSystemAddressForMdlSafe( *MdlAddressPointer, HighPagePriority );
        else
            UserBuffer = *BufferPointer;
        
    } while( false );

    return UserBuffer;
}
