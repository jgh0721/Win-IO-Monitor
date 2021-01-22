#pragma once

#include "base.hpp"

extern "C"
{
    int __cdecl _vsnprintf_s( char* dest, size_t size, size_t max_count, const char* format, va_list args );
    int __cdecl _vsnwprintf_s( wchar_t* dest, size_t size, size_t max_count, const wchar_t* format, va_list args );

}

namespace nsUtils
{
    size_t strlength( __in_z const wchar_t* str );
    size_t strlength( __in_z const char* str );
}

template< typename CharT >
class String
{
private:
    static const CharT NullChar = 0;

#ifdef _NTDDK_
    static constexpr ULONG StrPoolTag = 'RTS_';
#endif

    // Small string optimization (using stack memory for small strings):
    static constexpr unsigned char SSO_SIZE = 32;
    __declspec( align( MEMORY_ALLOCATION_ALIGNMENT ) ) CharT SsoBuffer[ SSO_SIZE ];

    static constexpr unsigned short AllocationGranularity = 64;

    using STRING_INFO = struct {
        CharT* Buffer = NULLPTR;

        size_t Length = 0;
        size_t BufferSize = 0;
        POOL_TYPE Type = NonPagedPool;

        BOOLEAN IsOwn = FALSE;
        BOOLEAN IsSSO = FALSE;
    };

    STRING_INFO                         Data;

    static inline VOID SetupSso( OUT STRING_INFO* StringInfo, const CharT* SsoBuffer )
    {
        StringInfo->Buffer = const_cast< CharT* >( SsoBuffer );
        StringInfo->Length = 0;
        StringInfo->BufferSize = SSO_SIZE * sizeof( CharT );
        StringInfo->IsOwn = TRUE;
        StringInfo->IsSSO = TRUE;
        StringInfo->Buffer[ 0 ] = NullChar;
        StringInfo->Buffer[ SSO_SIZE - 1 ] = NullChar;
    }

    static inline CharT* StrAllocMem( size_t Bytes, POOL_TYPE Type = NonPagedPool )
    {
#ifdef _NTDDK_
#ifdef POOL_NX_OPTIN
        return static_cast< CharT* >( ExAllocatePoolWithTag( ExDefaultNonPagedPoolType, Bytes, StrPoolTag ) );
#else
        return static_cast< CharT* >( ExAllocatePoolWithTag( Type, Bytes, StrPoolTag ) );
#endif
#else
        return reinterpret_cast< CharT* >( new BYTE[ Bytes ] );
#endif
    }

    static inline VOID StrFreeMem( CharT* Memory )
    {
#ifdef _NTDDK_
        ExFreePoolWithTag( Memory, StrPoolTag );
#else
        delete[] Memory;
#endif
    }

    static bool Alloc( OUT STRING_INFO* StringInfo, size_t Characters, POOL_TYPE Type = NonPagedPool )
    {
        if( !StringInfo || !Characters ) return false;
        *StringInfo = {};
        size_t Size = ( Characters + 1 ) * sizeof( CharT ); // Null-terminated buffer
        Size = ( ( Size / AllocationGranularity ) + 1 ) * AllocationGranularity;
        CharT* Buffer = StrAllocMem( Size, Type );
        if( !Buffer ) return false;
        Buffer[ 0 ] = NullChar;
        Buffer[ Characters ] = NullChar;
        StringInfo->Buffer = Buffer;
        StringInfo->Length = Characters;
        StringInfo->BufferSize = Size;
        StringInfo->IsOwn = TRUE;
        StringInfo->IsSSO = FALSE;
        StringInfo->Type = Type;
        return true;
    }

    static VOID Free( IN OUT STRING_INFO* StringInfo )
    {
        if( StringInfo && StringInfo->Buffer && !StringInfo->SsoUsing )
        {
            if( StringInfo->IsOwn != FALSE )
                StrFreeMem( StringInfo->Buffer );
        }
    }

    static VOID Copy( OUT CharT* Dest, const IN CharT* Src, size_t Characters, bool Terminate = true )
    {
        if( !Dest || !Src || !Characters ) return;
        RtlCopyMemory( Dest, Src, Characters * sizeof( CharT ) );
        if( Terminate ) Dest[ Characters ] = NullChar;
    }

public:

    String()
    {
        SetupSso( &Data, SsoBuffer );
    };
    ~String()
    {
        Free( &Data );
    }
    String( const CharT* Str )
        : String( Str, Length( Str ) )
    {

    }
    String( const CharT* Str, size_t StrLength ) : String()
    {
        if( StrLength < SSO_SIZE || Alloc( &Data, StrLength ) )
            Copy( Data.Buffer, Str, StrLength );

        Data.Length = StrLength;
    }
    String( const String& Str ) : String()
    {
        if( Str.Data.Length < SSO_SIZE || Alloc( &Data, Str.Data.Length ) )
            Copy( Data.Buffer, Str.Data.Buffer, Str.Data.Length );

        Data.Length = Str.Data.Length;
    }
    String( String&& Str ) noexcept : String()
    {
        if( Str.Data.SsoUsing || Str.Data.Length < SSO_SIZE )
        {
            Copy( Data.Buffer, Str.Data.Buffer, Str.Data.Length );
        }
        else
        {
            Data = Str.Data;
        }
        Data.Length = Str.Data.Length;
        Str.Data = {};
    }

    static inline size_t Length( const CharT* String )
    {
        return nsUtils::strlength( String );
    }

    VOID Clear()
    {
        Free( &Data );
        SetupSso( &Data, SsoBuffer );
    }

    void Shrink()
    {
        if( Data.SsoUsing ) return;
        size_t RequiredSize = ( ( ( ( Data.Length + 1 ) * sizeof( CharT ) ) / AllocationGranularity ) + 1 ) * AllocationGranularity;
        if( RequiredSize < Data.BufferSize )
        {
            STRING_INFO StringInfo = {};
            if( Data.Length < SSO_SIZE )
            {
                SetupSso( &StringInfo, SsoBuffer );
                Copy( StringInfo.Buffer, Data.Buffer, Data.Length );
                StringInfo.Length = Data.Length;
                Free( &Data );
                Data = StringInfo;
            }
            else
            {
                if( Alloc( &StringInfo, Data.Length ) )
                {
                    Copy( StringInfo.Buffer, Data.Buffer, Data.Length );
                    Free( &Data );
                    Data = StringInfo;
                }
            }
        }
    }

    void Resize( size_t Characters, CharT Filler = 0, bool AutoShrink = false )
    {
        if( Characters == Data.Length )
        {
            if( AutoShrink ) Shrink();
            return;
        }

        if( !Characters )
        {
            if( AutoShrink )
            {
                Clear();
            }
            else
            {
                Data.Buffer[ 0 ] = NullChar;
                Data.Length = 0;
            }
            return;
        }

        if( Characters > Data.Length )
        {
            size_t RequiredSize = ( Characters + 1 ) * sizeof( CharT );
            if( RequiredSize <= Data.BufferSize )
            {
                if( Filler == 0 )
                    RtlZeroMemory( &Data.Buffer[ Data.Length ], ( Characters - Data.Length ) * sizeof( CharT ) );
                else
                {
                    for( size_t Index = Data.Length; Index < Characters; ++Index )
                        Data.Buffer[ Index ] = Filler;
                }
                Data.Buffer[ Characters ] = NullChar;
                Data.Length = Characters;
                return;
            }

            STRING_INFO StringInfo = {};
            if( Alloc( &StringInfo, Characters ) )
            {
                Copy( StringInfo.Buffer, Data.Buffer, Data.Length );
                if( Filler == 0 )
                    RtlZeroMemory( &StringInfo.Buffer[ Data.Length ], ( Characters - Data.Length ) * sizeof( CharT ) );
                else
                {
                    for( size_t Index = Data.Length; Index < Characters; ++Index )
                        StringInfo.Buffer[ Index ] = Filler;
                }
                StringInfo.Buffer[ Characters ] = NullChar;
                StringInfo.Length = Characters;
                Free( &Data );
                Data = StringInfo;
            }
        }
        else
        {
            Data.Buffer[ Characters ] = NullChar;
            Data.Length = Characters;
        }
        if( AutoShrink ) Shrink();
    }

};

class AnsiString : public String<CHAR>
{
public:
    using String::String;
    AnsiString( PCANSI_STRING Ansi ) : String( Ansi->Buffer, Ansi->Length / sizeof( CHAR ) ) {}
    AnsiString( const String& Str ) : String( Str ) {}
    AnsiString( String&& Str ) : String( Str ) {}
    AnsiString() : String() {}
};

class WideString : public String<WCHAR>
{
public:
    using String::String;
    WideString( PCUNICODE_STRING Wide ) : String( Wide->Buffer, Wide->Length / sizeof( WCHAR ) ) {}
    WideString( const String& Str ) : String( Str ) {}
    WideString( String&& Str ) : String( Str ) {}
    WideString() : String() {}
};
