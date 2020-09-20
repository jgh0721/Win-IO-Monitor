#ifndef HDR_POOL_TAG
#define HDR_POOL_TAG

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define POOL_MAIN_TAG       'isTG'

#define POOL_IRPCONTEXT_TAG 'irTG'
#define POOL_FILENAME_TAG   'fnTG'
#define POOL_PROCNAME_TAG   'pnTG'
#define POOL_MSG_SEND_TAG   'snTG'
#define POOL_MSG_REPLY_TAG  'rpTG'
#define POOL_FCB_TAG        'fbTG'

#define POOL_READ_TAG       'rdTG'
#define POOL_WRITE_TAG      'wrTG'

#define CTX_STRING_TAG                          'tSxC'
#define CTX_RESOURCE_TAG                        'cRxC'
#define CTX_INSTANCE_CONTEXT_TAG                'cIxC'
#define CTX_VOLUME_CONTEXT_TAG                  'cVxc'
#define CTX_FILE_CONTEXT_TAG                    'cFxC'
#define CTX_STREAM_CONTEXT_TAG                  'cSxC'
#define CTX_STREAMHANDLE_CONTEXT_TAG            'cHxC'
#define CTX_TRANSACTION_CONTEXT_TAG             'cTxc'

#endif // HDR_POOL_TAG