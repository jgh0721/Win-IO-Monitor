#ifndef HDR_ISOLATION_NAME_CHANGER
#define HDR_ISOLATION_NAME_CHANGER

#include "fltBase.hpp"

#include "irpContext_Defs.hpp"
#include "NameChanger_Defs.hpp"

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

/**
 * @brief 
 * @param IrpContext 
 * @param SrcFileFullPath 
 * @return TRUE : NameChanger 의 대상
*/
BOOLEAN CheckNameChanger( __in IRP_CONTEXT* IrpContext, __in TyGenericBuffer<WCHAR>* SrcFileFullPath );

#endif // HDR_ISOLATION_NAME_CHANGER