#pragma once

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
    int open;               // 0xD4
    char pad1[ 0x28 ];
    int selected_index;     // 0x100 (seems UI-specific)

    // Very likely here:
    struct {
        size_t* first;      // pointer to array of active indices
        size_t* last;       // one past last
        size_t* end;        // end of allocated
    } m_active_items;       // ~0x104 or 0x108
};

class Slider : public Element {
public:
    int enabled;              // 0xD4 (based on CE view)
    char pad2[ 0x28 ];          // padding until next pointer/field
}; // ends 