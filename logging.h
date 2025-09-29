#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <sstream>
#include <vector>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <ctime>
#include <format>

#undef ERROR

enum eLogType : int {
    INFO = 0,
    WARNING,
    ERR,
    DEBUG
};

enum eLogColor {
    FORE_BLUE = FOREGROUND_BLUE,
    FORE_GREEN = FOREGROUND_GREEN,
    FORE_RED = FOREGROUND_RED,
    FORE_INTENSITY = FOREGROUND_INTENSITY,
    FORE_GRAY = FOREGROUND_INTENSITY,
    FORE_CYAN = FOREGROUND_BLUE | FOREGROUND_GREEN,
    FORE_MAGENTA = FOREGROUND_BLUE | FOREGROUND_RED,
    FORE_YELLOW = FOREGROUND_GREEN | FOREGROUND_RED,
    FORE_BLACK = 0U,
    FORE_WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    DEFAULT = FORE_WHITE
};

inline std::string CurrentTimeString( ) {
    using namespace std::chrono;

    auto now = system_clock::now( );
    auto ms = duration_cast< milliseconds >( now.time_since_epoch( ) ) % 1000;
    std::time_t t = system_clock::to_time_t( now );

    std::tm localTime{};
    localtime_s( &localTime, &t );

    char buffer[ 64 ];
    strftime( buffer, sizeof( buffer ), "[%H:%M:%S", &localTime );

    std::ostringstream oss;
    oss << buffer << "." << std::setfill( '0' ) << std::setw( 3 ) << ms.count( ) << "]";

    return oss.str( );
}

class Logger {
private:
    HANDLE hConsole;
    std::vector<std::string> logs;

public:
    class stream {
    private:
        bool firstLine = true;

    public:
        HANDLE hConsole;
        int oPrecision;
        int precision = -1;

        void output( const std::string& out ) {
            WriteConsoleA( hConsole, out.c_str( ), ( DWORD )out.size( ), nullptr, nullptr );
        }

        void setOutputColor( eLogColor color ) {
            SetConsoleTextAttribute( hConsole, color | FORE_INTENSITY );
        }

        stream& operator()( const eLogType type = INFO ) {
            if ( firstLine ) {
                firstLine = false;
            }
            else {
                *this << "\n";
            }

            // prepend timestamp
            *this << CurrentTimeString( ) << " ";

            // set color + prepend type
            switch ( type ) {
            case INFO:
                setOutputColor( FORE_GREEN );
                *this << "[INFO] ";
                break;
            case WARNING:
                setOutputColor( FORE_YELLOW );
                *this << "[WARN] ";
                break;
            case ERR:
                setOutputColor( FORE_RED );
                *this << "[ERR ] ";
                break;
            case DEBUG:
                setOutputColor( FORE_CYAN );
                *this << "[DBG ] ";
                break;
            }
            setOutputColor( FORE_WHITE ); // reset to default text color
            return *this;
        }

        stream& operator<<( const char* cstr ) {
            output( cstr );
            return *this;
        }

        stream& operator<<( const std::string& str ) {
            output( str );
            return *this;
        }

        stream& operator<<( int i ) {
            output( std::to_string( i ) );
            return *this;
        }

        stream& operator<<( float f ) {
            std::ostringstream oss;
            if ( precision == -1 )
                oss << f;
            else
                oss << std::fixed << std::setprecision( precision ) << f;
            output( oss.str( ) );
            return *this;
        }

        stream& operator<<( double d ) {
            std::ostringstream oss;
            if ( precision == -1 )
                oss << d;
            else
                oss << std::fixed << std::setprecision( precision ) << d;
            output( oss.str( ) );
            return *this;
        }

        stream& operator<<( void* ptr ) {
            std::ostringstream oss;
            oss << std::format( "0x{:X}", ( uintptr_t )ptr );
            output( oss.str( ) );
            return *this;
        }

        void setprecision( int i ) { precision = i; }
        int getprecision( ) { return precision; }
    };

    stream stream;

    bool attach( std::string title ) {
        AllocConsole( );
        SetConsoleTitleA( title.c_str( ) );
        FILE* f;
        freopen_s( &f, "CONOUT$", "w", stdout );

        hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
        SetConsoleMode( hConsole, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING );
        stream.hConsole = hConsole;
        return true;
    }

    bool detach( ) {
        fclose( stdout );
        return FreeConsole( );
    }

    void setprecision( int i ) { stream.setprecision( i ); }
    int getprecision( ) { return stream.getprecision( ); }
};
inline Logger gLogger;

#define Log(type) gLogger.stream(type)

#define ERROR