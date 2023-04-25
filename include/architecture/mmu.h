// EPOS MMU Mediator Common Package

#ifndef __mmu_h
#define __mmu_h

#include <architecture/cpu.h>
#include <utility/string.h>
#include <utility/list.h>

__BEGIN_SYS

// This common package provides operations to implement MMUs assuming a design with the following elements:
// * A Page_Directory (PD), which is the higher level of the paging hierarchy, associated to an Address_Space via a Directory;
// * A viable size Page_Table (PT), which is the lower level of the paging hierarchy, associated with a Segment via a Chunk;
// * An eventual Attacher (AT), which comprises all the intermediate levels of the paging hierarchy (non existent for two-level paging, second level for three-level paging, second and third levels for four-level paging).
template<unsigned int PD_BITS, unsigned int PT_BITS, unsigned int OFFSET_BITS, unsigned int AT_BITS = 0>
class MMU_Common
{
protected:
    MMU_Common() {}

protected:
    // CPU imports
    typedef CPU::Log_Addr Log_Addr;
    typedef CPU::Phy_Addr Phy_Addr;

    // Address Constants
    static const unsigned long LA_BITS  = OFFSET_BITS + PT_BITS + AT_BITS + PD_BITS;
    static const unsigned long PT_SHIFT = OFFSET_BITS;
    static const unsigned long AT_SHIFT = OFFSET_BITS + PT_BITS;
    static const unsigned long PD_SHIFT = OFFSET_BITS + PT_BITS + AT_BITS;

    static const unsigned long PG_SIZE = 1UL << OFFSET_BITS;
    static const unsigned long PT_SPAN = 1UL << (OFFSET_BITS + PT_BITS);
    static const unsigned long AT_SPAN = 1UL << (OFFSET_BITS + PT_BITS + AT_BITS);
    static const unsigned long PD_SPAN = 1UL << (OFFSET_BITS + PT_BITS + AT_BITS + PD_BITS);

public:
    // Memory page
    typedef unsigned char Page[PG_SIZE];
    typedef Page Frame;

    // Page_Table, Attacher and Page_Directory entries
    typedef Phy_Addr PT_Entry;
    typedef Phy_Addr AT_Entry;
    typedef Phy_Addr PD_Entry;

    // Number of entries in Page_Table, Attacher and Page_Directory
    static const unsigned int PT_ENTRIES = 1 << PT_BITS;
    static const unsigned int AT_ENTRIES = 1 << AT_BITS;
    static const unsigned int PD_ENTRIES = 1 << PD_BITS;

    // Page Flags
    class Flags
    {
    public:
        enum {
            PRE  = 1 << 0, // Present
            RD   = 1 << 1, // Readable
            RW   = 1 << 2, // Writable
            EX   = 1 << 3, // Executable
            USR  = 1 << 4, // Access Control (0=supervisor, 1=user)
            CD   = 1 << 5, // Cache disable (0=cacheable, 1=non-cacheable)
            CWT  = 1 << 6, // Cache mode (0=write-back, 1=write-through)
            CT   = 1 << 7, // Contiguous (0=non-contiguous, 1=contiguous)
            IO   = 1 << 8, // Memory Mapped I/O (0=memory, 1=I/O)
            NLP  = ~(PRE | RW | EX), // next level pointer if equals to 0 point to next-page
            SYS  = (PRE | RD | RW | EX),
            APP  = (PRE | RD | RW | EX | USR),
            APPC = (PRE | RD | EX | USR),
            APPD = (PRE | RD | RW | USR)
        };

    public:
        Flags() {}
        Flags(const Flags & f) : _flags(f._flags) {}
        Flags(unsigned int f) : _flags(f) {}

        operator unsigned int() const { return _flags; }

        friend Debug & operator<<(Debug & db, Flags f) { db << hex << f._flags << dec; return db; }

    private:
        unsigned int _flags;
    };

    // Page types
    enum Page_Type {PG, PT, AT, PD};

public:
    // Functions to calculate quantities
    constexpr static unsigned int pds(unsigned int ats) { return 1; }
    constexpr static unsigned int ats(unsigned int pts) { return AT_BITS ? (pts + AT_ENTRIES - 1) / AT_ENTRIES : 0; }
    constexpr static unsigned int pts(unsigned int pages) { return PT_BITS ? (pages + PT_ENTRIES - 1) / PT_ENTRIES : 0; }
    constexpr static unsigned long pages(unsigned long bytes) { return (bytes + sizeof(Page) - 1) / sizeof(Page); }

    // Functions to handle logical addresses
    constexpr static unsigned long ind(Log_Addr addr) { return addr & ~(sizeof(Page) - 1); }
    constexpr static unsigned long off(Log_Addr addr) { return addr & (sizeof(Page) - 1); }
    constexpr static unsigned long pti(Log_Addr addr) { return (addr >> PT_SHIFT) & (PT_ENTRIES - 1); }
    constexpr static unsigned long pti(Log_Addr base, Log_Addr addr) { return pti(addr - base); }
    constexpr static unsigned long ati(Log_Addr addr) { return (addr >> AT_SHIFT) & (AT_ENTRIES - 1); }
    constexpr static unsigned long pdi(Log_Addr addr) { return (addr >> PD_SHIFT) & (PD_ENTRIES - 1); }

    constexpr static Log_Addr align_page(Log_Addr addr) { return (addr + sizeof(Page) - 1) & ~(sizeof(Page) - 1); }
    constexpr static Log_Addr align_segment(Log_Addr addr) { return (addr + PT_ENTRIES * sizeof(Page) - 1) &  ~(PT_ENTRIES * sizeof(Page) - 1); }
    constexpr static Log_Addr directory_bits(Log_Addr addr) { return (addr & ~((1 << PD_BITS) - 1)); }

    // AUXILIAR FUNCTIONS
    // Quantity of tables for the quantity of pages
    constexpr static unsigned long page_tables(unsigned int pages) { return PT_BITS ? (pages + PT_ENTRIES - 1) / PT_ENTRIES : 0; }
    constexpr static unsigned long addr_without_off(Log_Addr addr) { return ((addr >> OFFSET_BITS) << OFFSET_BITS); }
};

class No_MMU: public MMU_Common<0, 0, 0>
{
    friend class CPU;
    friend class Setup;

private:
    typedef Grouping_List<unsigned int> List;

public:
    // Page Flags
    typedef Flags Page_Flags;

    // Page_Table
    class Page_Table {
        friend OStream & operator<<(OStream & os, Page_Table & pt) {
            os << "{}";
            return os;
        }
    };

    // Chunk (for Segment)
    class Chunk
    {
    public:
        Chunk() {}
        Chunk(unsigned int bytes, Flags flags, Color color = WHITE): _phy_addr(alloc(bytes)), _bytes(bytes), _flags(flags) {}
        Chunk(Phy_Addr phy_addr, unsigned int bytes, Flags flags): _phy_addr(phy_addr), _bytes(bytes), _flags(flags) {}
        Chunk(Phy_Addr pt, unsigned int from, unsigned int to, Flags flags): _phy_addr(0), _bytes(0), _flags(flags) {}

        ~Chunk() { free(_phy_addr, _bytes); }

        unsigned int pts() const { return 0; }
        Flags flags() const { return _flags; }
        Page_Table * pt() const { return 0; }
        unsigned int size() const { return _bytes; }
        Phy_Addr phy_address() const { return _phy_addr; } // always CT
        int resize(unsigned int amount) { return 0; } // no resize in CT

    private:
        Phy_Addr _phy_addr;
        unsigned int _bytes;
        Flags _flags;
    };

    // Page Directory
    typedef Page_Table Page_Directory;

    // Directory (for Address_Space)
    class Directory
    {
    public:
        Directory() {}
        Directory(Page_Directory * pd) {}

        Page_Table * pd() const { return 0; }

        void activate() {}

        Log_Addr attach(const Chunk & chunk) { return chunk.phy_address(); }
        Log_Addr attach(const Chunk & chunk, Log_Addr addr) { return (addr == chunk.phy_address())? addr : Log_Addr(false); }
        void detach(const Chunk & chunk) {}
        void detach(const Chunk & chunk, Log_Addr addr) {}

        Phy_Addr physical(Log_Addr addr) { return addr; }
    };

    // DMA_Buffer (straightforward without paging)
    class DMA_Buffer: public Chunk
    {
    public:
        DMA_Buffer(unsigned int s): Chunk(s, Flags::CT) {}

        Log_Addr log_address() const { return phy_address(); }

        friend Debug & operator<<(Debug & db, const DMA_Buffer & b) {
            db << "{phy=" << b.phy_address() << ",log=" << b.log_address() << ",size=" << b.size() << ",flags=" << b.flags() << "}";
            return db;
        }
    };

    // Class Translation performs manual logical to physical address translations for debugging purposes only
    class Translation
    {
    public:
        Translation(Log_Addr addr, bool pt = false, Page_Directory * pd = 0): _addr(addr) {}

        friend OStream & operator<<(OStream & os, const Translation & t) {
            os << "{addr=" << static_cast<void *>(t._addr) << ",pd=0" << ",pd[???]=0" << ",pt[???]=0" << ",*addr=" << hex << *static_cast<unsigned int *>(t._addr) << "}";
            return os;
        }

    private:
        Log_Addr _addr;
    };

public:
    No_MMU() {}

    static Phy_Addr alloc(unsigned int bytes = 1, Color color = WHITE) {
        Phy_Addr phy(false);
        if(bytes) {
            List::Element * e = _free.search_decrementing(bytes);
            if(e)
                phy = reinterpret_cast<unsigned long>(e->object()) + e->size();
            else
                db<MMU>(ERR) << "MMU::alloc() failed!" << endl;
        }
        db<MMU>(TRC) << "MMU::alloc(bytes=" << bytes << ") => " << phy << endl;

        return phy;
    };

    static Phy_Addr calloc(unsigned int bytes = 1, Color color = WHITE) {
        Phy_Addr phy = alloc(bytes);
        memset(phy, 0, bytes);
        return phy;
    }

    static void free(Phy_Addr addr, unsigned int n = 1) {
        db<MMU>(TRC) << "MMU::free(addr=" << addr << ",n=" << n << ")" << endl;

        // No unaligned addresses if the CPU doesn't support it
        assert(Traits<CPU>::unaligned_memory_access || !(addr % (Traits<CPU>::WORD_SIZE / 8)));

        // Free blocks must be large enough to contain a list element
        assert(n > sizeof (List::Element));

        if(addr && n) {
            List::Element * e = new (addr) List::Element(addr, n);
            List::Element * m1, * m2;
            _free.insert_merging(e, &m1, &m2);
        }
    }

    static unsigned int allocable(Color color = WHITE) { return _free.head() ? _free.head()->size() : 0; }

    static Page_Directory * volatile current() { return 0; }

    static Phy_Addr physical(Log_Addr addr) { return addr; }

    static PT_Entry phy2pte(Phy_Addr frame, Flags flags) { return frame; }
    static Phy_Addr pte2phy(PT_Entry entry) { return entry; }
    static PD_Entry phy2pde(Phy_Addr frame) { return frame; }
    static Phy_Addr pde2phy(PD_Entry entry) { return entry; }

    static Log_Addr phy2log(Phy_Addr phy) { return phy; }
    static Phy_Addr log2phy(Log_Addr log) { return log; }

private:
    static Phy_Addr pd() { return 0; }
    static void pd(Phy_Addr pd) {}

    // ### Maybe should be declared as public (?)
    static void flush_tlb() {}
    static void flush_tlb(Log_Addr addr) {}

    static void init();

private:
    static List _free;
};

__END_SYS

#endif

#if defined(__MMU_H) && !defined(__mmu_common_only__)
#include __MMU_H
#endif
