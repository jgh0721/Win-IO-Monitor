#include "NameChanger.hpp"

#include "fltCmnLibs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

BOOLEAN CheckNameChanger( IRP_CONTEXT* IrpContext, TyGenericBuffer<WCHAR>* SrcFileFullPath )
{
    BOOLEAN IsConcerned = FALSE;

    do
    {
        if( IrpContext == NULLPTR || SrcFileFullPath == NULLPTR || SrcFileFullPath->Buffer == NULLPTR || SrcFileFullPath->BufferSize == 0 )
            break;

        /*!
            Name Change Condition ( AND Condition )

            1. File Name : ABCDEF.XYZ.EXE
            2. Must be exist metadata info in file( Hidden File )
        */

        if( IrpContext->Fcb == NULLPTR )
            break;

        if( !FlagOn( IrpContext->Fcb->Flags, FCB_STATE_METADATA_ASSOC ) )
            break;

        auto Suffix = nsUtils::EndsWithW( SrcFileFullPath->Buffer, L".exe" );
        if( Suffix == NULLPTR )
            break;

        auto Midfix = nsUtils::ForwardFindW( SrcFileFullPath->Buffer, L'.' );
        if( Midfix == Suffix )
            break;

        IsConcerned = TRUE;

    } while( false );

    return IsConcerned;
}
