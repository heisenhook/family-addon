#pragma once
#include "logging.h"
#include <sstream>
#include <iomanip>

// helper small vector view for x86 pointer-sized entries
struct VectorView {
    uintptr_t* first; // pointer to first element
    uintptr_t* last;  // pointer past last
    uintptr_t* end;   // pointer end (capacity)
    size_t count( ) const { return ( last - first ); } // number of entries
};

template<typename T>
struct RawVector {
    T* first;
    T* last;
    T* capacity;

    size_t size() const {
        return (last && first) ? (last - first) : 0;
    }

    T& operator[](size_t i) {
        return first[i];
    }
    const T& operator[](size_t i) const {
        return first[i];
    }

    bool valid() const {
        return first != nullptr && last != nullptr && last >= first;
    }
};

static RawVector<size_t>& GetActiveIndicesRef(uintptr_t multiBase) {
    // m_active_items is at offset 0x110
    return *reinterpret_cast<RawVector<size_t>*>(multiBase + 0x110);
}

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

    unsigned int length;             // Number of characters in use
    unsigned int capacity;           // Buffer size (without null terminator)

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

// ------------------------ Helper functions (free / inline) ------------------------

// Safe reader of "GameString" object placed at `addr` (addr points at the GameString object)
static inline std::string ReadStdStringAtAddr(uintptr_t addr) {
    if (!addr) return {};
    const GameString* gs = reinterpret_cast<const GameString*>(addr);
    // sanity checks
    if (gs->length < 0 || gs->length > 0x10000) return {};
    // if SSO or heap pointer missing this still works (c_str handles it)
    return gs->to_std();
}

// Read a RawVector<T> stored at base + offset (2 pointers for x86: first,last)
// returns true and fills out_first/out_last if successful.
static inline bool read_raw_vector_ptrs(uintptr_t base, uint32_t offset, uintptr_t& out_first, uintptr_t& out_last) {
    __try {
        out_first = *reinterpret_cast<uint32_t*>(base + offset);
        out_last = *reinterpret_cast<uint32_t*>(base + offset + 4);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        out_first = out_last = 0;
        return false;
    }
}

// Read active indices from m_active_items (offset 0x110). Returns vector of indices.
static inline std::vector<size_t> ReadActiveIndices(uintptr_t multiBase) {
    uintptr_t first = 0, last = 0;
    if (!read_raw_vector_ptrs(multiBase, 0x110, first, last)) return {};
    if (!first || !last || last <= first) return {};
    size_t count = (last - first) / sizeof(uint32_t); // x86: 4-byte entries
    std::vector<size_t> out;
    out.reserve(count);
    uint32_t* arr = reinterpret_cast<uint32_t*>(first);
    for (size_t i = 0; i < count; ++i) out.push_back(static_cast<size_t>(arr[i]));
    return out;
}

// Read item string at index from m_items vector (offset 0xF0). Each element is a GameString object (24 bytes typical on x86)
static inline std::string ReadItemStringAtIndex(uintptr_t multiBase, size_t index) {
    uintptr_t items_first = 0, items_last = 0;
    if (!read_raw_vector_ptrs(multiBase, 0xF0, items_first, items_last)) return {};
    if (!items_first || !items_last || items_last <= items_first) return {};
    const size_t elementSize = sizeof(GameString); // should be 24 on x86; this keeps it portable to your struct
    size_t item_count = (items_last - items_first) / elementSize;
    if (index >= item_count) return {};
    uintptr_t string_obj_addr = items_first + index * elementSize;
    return ReadStdStringAtAddr(string_obj_addr);
}

// Get names of active items as strings
static inline std::vector<std::string> GetActiveItemNames(uintptr_t multiBase) {
    auto indices = ReadActiveIndices(multiBase);
    std::vector<std::string> out;
    out.reserve(indices.size());
    for (auto idx : indices) {
        auto s = ReadItemStringAtIndex(multiBase, idx);
        out.push_back(std::move(s));
    }
    return out;
}

// Safe read of a GameString located at addr (addr -> GameString object)
static inline std::string ReadStdStringAtAddrSafe(uintptr_t addr) {
    if (!addr) return {};
    __try {
        const GameString* gs = reinterpret_cast<const GameString*>(addr);
        if (gs->length < 0 || gs->length > 0x10000) return {};
        return gs->to_std();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return {};
    }
}

// Read raw pair (first,last) from vector-like structure at base+offset (x86)
static inline bool ReadVectorFirstLast(uintptr_t base, uint32_t offset, uintptr_t& out_first, uintptr_t& out_last, uintptr_t& out_cap) {
    out_first = out_last = out_cap = 0;
    __try {
        out_first = static_cast<uintptr_t>(*reinterpret_cast<uint32_t*>(base + offset));
        out_last = static_cast<uintptr_t>(*reinterpret_cast<uint32_t*>(base + offset + 4));
        out_cap = static_cast<uintptr_t>(*reinterpret_cast<uint32_t*>(base + offset + 8));
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Read active indices from m_active_items at offset 0x110
static inline std::vector<uint32_t> ReadActiveIndicesSafe(uintptr_t multiBase) {
    uintptr_t first = 0, last = 0, cap = 0;
    if (!ReadVectorFirstLast(multiBase, 0x110, first, last, cap)) return {};
    if (!first || !last || last <= first) return {};
    size_t count = (last - first) / sizeof(uint32_t);
    std::vector<uint32_t> out;
    out.reserve(count);
    __try {
        uint32_t* arr = reinterpret_cast<uint32_t*>(first);
        for (size_t i = 0; i < count; ++i) out.push_back(arr[i]);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        out.clear();
    }
    return out;
}

// Read item strings from m_items vector at offset 0xF0. Each element is GameString (24 bytes on x86)
static inline std::string ReadItemStringAtIndexSafe(uintptr_t multiBase, size_t index) {
    uintptr_t first = 0, last = 0, cap = 0;
    if (!ReadVectorFirstLast(multiBase, 0xF0, first, last, cap)) return {};
    if (!first || !last || last <= first) return {};
    const size_t elementSize = sizeof(GameString); // expected 24
    size_t count = (last - first) / elementSize;
    if (index >= count) return {};
    uintptr_t elemAddr = first + index * elementSize;
    return ReadStdStringAtAddrSafe(elemAddr);
}

// Dump everything: addresses, counts, items and active indices
static inline void DumpMultiDropdownDebug(uintptr_t multiBase) {
    if (!multiBase) { Log() << "DumpMultiDropdownDebug: null base"; return; }

    // layout checks (logs only)
    std::ostringstream ss;
    ss << std::hex << "MultiDropdown @ 0x" << multiBase << std::dec << "\n";

    // read items vector
    uintptr_t items_first = 0, items_last = 0, items_cap = 0;
    if (ReadVectorFirstLast(multiBase, 0xF0, items_first, items_last, items_cap)) {
        size_t item_count = (items_last > items_first) ? (items_last - items_first) / sizeof(GameString) : 0;
        ss << " m_items.first=0x" << std::hex << items_first
            << " last=0x" << items_last << " cap=0x" << items_cap
            << " count=" << std::dec << item_count << "\n";

        for (size_t i = 0; i < item_count; ++i) {
            uintptr_t objAddr = items_first + i * sizeof(GameString);
            std::string s = ReadStdStringAtAddrSafe(objAddr);
            ss << "  [" << i << "] addr=0x" << std::hex << objAddr << std::dec << " '" << (s.empty() ? "<empty>" : s) << "'\n";
        }
    }
    else {
        ss << " m_items: failed to read vector pointers at 0xF0\n";
    }

    // read active indices
    uintptr_t act_first = 0, act_last = 0, act_cap = 0;
    if (ReadVectorFirstLast(multiBase, 0x110, act_first, act_last, act_cap)) {
        size_t active_count = (act_last > act_first) ? (act_last - act_first) / sizeof(uint32_t) : 0;
        ss << " m_active_items.first=0x" << std::hex << act_first
            << " last=0x" << act_last << " cap=0x" << act_cap
            << " count=" << std::dec << active_count << "\n";

        // print indices
        __try {
            uint32_t* arr = reinterpret_cast<uint32_t*>(act_first);
            for (size_t i = 0; i < active_count; ++i) {
                uint32_t idx = arr[i];
                std::string s = ReadItemStringAtIndexSafe(multiBase, idx);
                ss << "  active[" << i << "] = idx=" << idx << " -> '" << (s.empty() ? "<invalid-or-empty>" : s) << "'\n";
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            ss << "  (exception while reading active array)\n";
        }

    }
    else {
        ss << " m_active_items: failed to read vector pointers at 0x110\n";
    }

    // log m_open and other small fields
    __try {
        uint8_t m_open = *reinterpret_cast<uint8_t*>(multiBase + 0xD4);
        uint32_t some_ptr = *reinterpret_cast<uint32_t*>(multiBase + 0x11C);
        int sentinel = *reinterpret_cast<int*>(multiBase + 0x120);
        ss << std::hex << " m_open=" << static_cast<int>(m_open) << " m_some_ptr=0x" << some_ptr << " sentinel=0x" << sentinel << "\n";
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ss << " failed to read m_open / other small fields\n";
    }

    Log() << ss.str();
}

class Form;
class Element {
public:
private:
    char pad0[0x24];
public:
    Form* m_parent;
    GameString m_label;
    GameString m_file_id;
private:
    char pad1[0x7C];

public:
    // Accessors
    std::string label() const { return m_label.to_std(); }
    std::string file_id() const { return m_file_id.to_std(); }
}; // total: 0xD4

class Tab {
public:
    /*0x00*/ GameString m_title;  // fixed-size char array
    /*0x18*/ int one;
    /*0x1C*/ RawVector<Element*> m_elements; // 0x0C

public:
    std::vector<Element*> GetElements() {
        uintptr_t first = 0, last = 0;
        if (!read_raw_vector_ptrs((uintptr_t)this, 0x1C, first, last))
            return {};

        if (!first || !last || last <= first)
            return {};

        size_t count = (last - first) / sizeof(void*); // x86: 4-byte pointers
        std::vector<Element*> out;
        out.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            uintptr_t element_address = first + i * sizeof(void*);
            out.push_back(*reinterpret_cast<Element**>(element_address));
            Element* e = *reinterpret_cast<Element**>(element_address);
            if (reinterpret_cast<Element*>(element_address))
                Log() << "Found element " << (*reinterpret_cast<Element**>(element_address))->m_file_id.c_str() << " at " << std::format("{:X}", (DWORD)element_address);
        }

        return out;
    }
};

class Form {
public:
    /*0x00*/ bool m_open;       // 8 bytes
    /*0x04*/ float m_opacity;
    /*0x08*/ int m_alpha;
    /*0x0C*/ int m_key1; // value of 0x2E / 46, VK_DELETE
    /*0x10*/ int m_key2; // value of 0x2D / 45, VK_INSERT
    /*0x14*/ int m_x;
    /*0x18*/ int m_y;
    /*0x1C*/ int m_width;
    /*0x20*/ int m_height;
    /*0x24*/ int m_tick;
    /*0x28*/ RawVector<Tab*> m_tabs; // 0x0C (32-bit!)
    /*0x34*/ char pad01[ 0x1C ];        // to reach 0x50
    /*0x50*/ int m_tab_count;
    /*0x54*/ Tab* m_active_tab;
    /*0x58*/ Element* m_active_element;
public:
    std::vector<Tab*> GetTabs() {
        uintptr_t first = 0, last = 0;
        if (!read_raw_vector_ptrs((uintptr_t)this, 0x28, first, last))
            return {};

        if (!first || !last || last <= first)
            return {};

        size_t count = (last - first) / sizeof(void*); // x86: 4-byte pointers
        std::vector<Tab*> out;
        out.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            uintptr_t tab_address = first + i * sizeof(void*);
            out.push_back(*reinterpret_cast<Tab**>(tab_address));
            if (reinterpret_cast<Tab*>(tab_address))
                Log() << "Found tab " << reinterpret_cast<Tab*>(tab_address)->m_title.c_str() << " at " << std::format("{:X}", (DWORD)tab_address);
        }

        return out;
    }
};

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
    // 0xD4: open flag (1 byte)
    uint8_t m_open;              // 0xD4
    uint8_t pad_after_open[0xF0 - 0xD5];

    // 0xF0: m_items vector<GameString> (RawVector<GameString>)
    RawVector<GameString> m_items;   // _Myfirst/_Mylast/_Myend stored at 0xF0..0xFB

    // padding until 0x10C (there are other fields here in the real class)
    uint8_t pad_to_anim[0x10C - (0xF0 + sizeof(RawVector<GameString>))];

    // 0x10C: animation height / progress (float)
    float m_anim_height;         // 0x10C

    // padding until 0x110
    // uint8_t pad_to_active[0x110 - (0x10C + sizeof(float))];

    // 0x110: m_active_items vector<size_t> (x86 size_t == uint32_t)
    RawVector<GameString> m_active_items; // _Myfirst/_Mylast/_Myend at 0x110..0x11B

    // 0x11C: some pointer used by think() (interpreted in IDA as pointer)
    uintptr_t m_some_ptr;        // 0x11C

    // 0x120: sentinel (constructor sets 0xFFFFFFFF)
    int m_sentinel;              // 0x120
};

class Slider : public Element {
public:
    bool drag;              // 0xD4 (based on CE view)
    float value;     // 0x100: Selected index value ← YOUR NEW FINDING
}; // ends 

static_assert(sizeof(GameString) <= 32, "Expect GameString small (SSO) on x86 builds");
static_assert(sizeof(GameString) == 24, "Expect GameString == 24 on x86");
static_assert(sizeof(uintptr_t) == 4, "Expect 32-bit build");
static_assert(sizeof(RawVector<GameString>) == 12, "RawVector must be 12 bytes on x86");