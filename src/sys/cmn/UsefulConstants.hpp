#ifndef HDR_USEFUL_CONSTANTS
#define HDR_USEFUL_CONSTANTS

#if defined(_MSC_VER)
#   pragma execution_character_set( "utf-8" )
#endif

#define MODULE_NAME "[iMonFSD]"

#define WIN32_PATH_UNICODE_PREFIX L"\\??\\"
#define WIN32_PATH_UNICODE_PREFIX_SIZE 4

#define WINNT_PATH_DEVICE_PREFIX L"\\Device\\"
#define WINNT_PATH_DEVICE_PREFIX_SIZE 8

const LARGE_INTEGER ZERO_INTEGER = { 0, 0 };

#endif // HDR_USEFUL_CONSTANTS