#pragma once

// helper small vector view for x86 pointer-sized entries
struct VectorView {
    uintptr_t* first; // pointer to first element
    uintptr_t* last;  // pointer past last
    uintptr_t* end;   // pointer end (capacity)
    size_t count( ) const { return ( last - first ); } // number of entries
};

struct RawVector {
    uintptr_t _Myfirst;   // pointer (address of element 0)
    uintptr_t _Mylast;    // pointer (one past last)
    uintptr_t _Myend;     // pointer (capacity end)
};

// read-only SSO-friendly string view (minimal, to display)
struct GameStringView {
    // We don't assume std::string layout here; this just stores a pointer to chars.
    const char* c_str( ) const { return ( const char* )this; } // hack: only valid if string inlined, so be careful
};

struct GameString
{
    union
    {
        char ssoData[ 16 ];   // Inline storage (Small String Optimization)
        char* heapPtr;      // Used when string is too large for SSO
    };

    int length;             // Number of characters in use
    int capacity;           // Buffer size (without null terminator)

    // Helper to get a readable C-string
    const char* c_str( ) const
    {
        // If length fits in the inline buffer, use SSO
        if ( length <= 15 )
            return ssoData;

        // Otherwise use the heap pointer
        return heapPtr;
    }

    std::string to_std( ) const
    {
        return std::string( c_str( ), length );
    }
};

class Tab;
class Element;
class Form {
public:
    /*0x00*/ double m_opacity;       // 8 bytes
    /*0x08*/ int one;
    /*0x0C*/ int two;
    /*0x10*/ int three;
    /*0x14*/ int m_x;
    /*0x18*/ int m_y;
    /*0x1C*/ int m_width;
    /*0x20*/ int m_height;
    /*0x24*/ int m_tick;
    /*0x28*/ std::vector<Tab*> m_tabs; // 0x0C (32-bit!)
    /*0x34*/ char pad01[ 0x1C ];        // to reach 0x50
    /*0x50*/ int m_tab_count;
    /*0x54*/ Tab* m_active_tab;
    /*0x58*/ Element* m_active_element;
};

class Tab {
public:
    /*0x00*/ char m_title[ 0x18 ];  // fixed-size char array
    /*0x18*/ int one;
    /*0x1C*/ std::vector<Element*> m_elements; // 0x0C
};

class Element {
public:
private:
    char pad0[ 0x28 ];
public:
    // void* m_parent;
    GameString m_label;
    GameString m_file_id;
private:
    char pad1[ 0x7C ];

public:
    // Accessors
    std::string label( ) const { return m_label.to_std( ); }
    std::string file_id( ) const { return m_file_id.to_std( ); }
}; // total: 0xD4

class Checkbox : public Element {
public:
    int enabled;              // 0xD4 (based on CE view)
    char pad2[ 0x28 ];          // padding until next pointer/field
}; // ends around 0xF0

class Dropdown : public Element {
public:
    int open;               // 0xD4: Dropdown open/closed state
    char pad1[ 0x28 ];
    int selected_index;     // 0x100: Selected index value ← YOUR NEW FINDING
}; // ends around 0xF0

class MultiDropdown : public Element {
public:
    // Element base fields are before 0xD4 (not repeated here)
    // 0xD4: open flag (byte)
    uint8_t m_open;              // 0xD4
    uint8_t pad_after_open[ 0xF0 - 0xD5 ];

    // 0xF0: m_items vector<string> (first,last,end)
    RawVector m_items;           // at 0xF0 (first=_Myfirst, last=_Mylast, end=_Myend)
    // Each string object size = 24 bytes -> count = (last-first)/24

// there are more fields 0xF8..0x10B (label, base h, etc) - we skip to known ones
    uint8_t pad_to_anim[ 0x10C - ( 0xF0 + sizeof( RawVector ) ) ];

    // 0x10C: animation height / progress (float)
    float m_anim_height;         // 0x10C

    // 0x110: m_active_items vector<size_t>
    RawVector m_active_items;    // _Myfirst/_Mylast/_Myend at 0x110/0x114/0x118
    // For this build size_t == 4 => count = (last-first)/4

// 0x11C: pointer used by think() (treated as pointer; follow it in CE)
    uintptr_t m_some_ptr;        // 0x11C  (interpreted as pointer in the code)
    int m_sentinel;              // 0x120 (constructor sets 0xFFFFFFFF)

public:

    inline std::string ReadStdStringAtAddr( uintptr_t addr ) {
        if ( !addr )
            return {};

        // interpret the address as GameString
        const GameString* gs = reinterpret_cast< const GameString* >( addr );
        // validate length
        if ( gs->length < 0 || gs->length > 0x10000 ) // arbitrary sanity check
            return {};

        return gs->to_std( );
    }

    // read_vector_from_raw: returns {first,last} as pointers (safe)
    static bool read_raw_vector( uintptr_t base, uint32_t offset, uintptr_t& out_first, uintptr_t& out_last ) {
        __try {
            out_first = *reinterpret_cast< uint32_t* >( base + offset );
            out_last = *reinterpret_cast< uint32_t* >( base + offset + 4 );
            return true;
        }
        __except ( EXCEPTION_EXECUTE_HANDLER ) {
            out_first = out_last = 0;
            return false;
        }
    }

    // read active indices (size_t/int)
    std::vector<size_t> ReadActiveIndices( uintptr_t multiBase ) {
        uintptr_t first = 0, last = 0;
        if ( !read_raw_vector( multiBase, 0x110, first, last ) )
            return {};
        if ( !first || !last || last <= first ) return {};
        size_t count = ( last - first ) / sizeof( uint32_t ); // x86: 4 bytes
        std::vector<size_t> out;
        out.reserve( count );
        uint32_t* arr = reinterpret_cast< uint32_t* >( first );
        for ( size_t i = 0; i < count; ++i )
            out.push_back( static_cast< size_t >( arr[ i ] ) );

        return out;
    }

    // read items (vector<std::string>) and get string at index
    std::string ReadItemStringAtIndex( uintptr_t multiBase, size_t index ) {
        uintptr_t items_first = 0, items_last = 0;
        if ( !read_raw_vector( multiBase, 0xF0, items_first, items_last ) ) return {};
        if ( !items_first || !items_last || items_last <= items_first ) return {};
        size_t item_count = ( items_last - items_first ) / 24; // 24 bytes per std::string
        if ( index >= item_count ) return {};
        uintptr_t string_obj_addr = items_first + index * 24;
        // Use your GameString reader here: if string_obj_addr points to std::string object,
        // you may need to parse the std::string structure. If you implemented ReadStdStringAtAddr(addr) that reads the std::string at addr, call it:
        return ReadStdStringAtAddr( string_obj_addr );
    }

    // get list of selected item strings
    std::vector<std::string> GetActiveItemNames( uintptr_t multiBase ) {
        auto indices = ReadActiveIndices( multiBase );
        std::vector<std::string> out;
        for ( auto idx : indices ) {
            auto s = ReadItemStringAtIndex( multiBase, idx );
            if ( !s.empty( ) ) out.push_back( std::move( s ) );
        }
        return out;
    }

    // get item index by name
    size_t GetItemIndexByName( const std::string& name ) {
        if ( !this ) return ( size_t )-1;

        RawVector& items = this->m_items;
        size_t count = ( items._Mylast - items._Myfirst ) / 24; // 24 bytes per GameString

        for ( size_t i = 0; i < count; ++i ) {
            uintptr_t addr = items._Myfirst + i * 24;
            GameString* gs = reinterpret_cast< GameString* >( addr );
            if ( gs->to_std( ) == name ) {
                return i;
            }
        }

        return ( size_t )-1; // not found
    }

    // add item to list
    void AddActiveItem( const std::string& name ) {
        if ( !this ) return;

        size_t idx = GetItemIndexByName( name );
        if ( idx == ( size_t )-1 ) return; // item not found

        RawVector& active = this->m_active_items;
        uint32_t* data = reinterpret_cast< uint32_t* >( active._Myfirst );

        // check if already active
        size_t currentCount = ( active._Mylast - active._Myfirst ) / sizeof( uint32_t );
        for ( size_t i = 0; i < currentCount; ++i ) {
            if ( data[ i ] == idx ) return; // already active
        }

        // append new index if space
        size_t capacity = ( active._Myend - active._Myfirst ) / sizeof( uint32_t );
        if ( currentCount < capacity ) {
            data[ currentCount ] = static_cast< uint32_t >( idx );
            active._Mylast = active._Myfirst + ( currentCount + 1 ) * sizeof( uint32_t );
        }
    }

};

class Slider : public Element {
public:
    int enabled;              // 0xD4 (based on CE view)
    char pad2[ 0x28 ];          // padding until next pointer/field
}; // ends 