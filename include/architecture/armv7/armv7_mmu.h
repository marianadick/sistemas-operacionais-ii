// EPOS ARMv7 MMU Mediator Declarations

#ifndef __armv7_mmu_h
#define __armv7_mmu_h

#define __mmu_common_only__
#include <architecture/mmu.h>
#undef __mmu_common_only__
#include <system/memory_map.h>

__BEGIN_SYS

class ARMv7_MMU: public MMU_Common<12, 8, 12>
{
    friend class CPU;
    friend class Setup;

private:
    typedef Grouping_List<Frame> List;
    typedef MMU_Common<12, 8, 12> Common;

    static const bool colorful = Traits<MMU>::colorful;
    static const unsigned long COLORS = Traits<MMU>::COLORS;
    static const unsigned long RAM_BASE = Memory_Map::RAM_BASE;
    static const unsigned long PHY_MEM = Memory_Map::PHY_MEM;
    static const unsigned long APP_LOW = Memory_Map::APP_LOW;
    static const unsigned long APP_HIGH = Memory_Map::APP_HIGH;

public:
    // Page Flags (for ARMv7-A small pages)
    class Page_Flags
    {
    public:
        // Page Table entry flags
        enum : unsigned long {
            XN   = 1 << 0,      // not executable
            PTE  = 1 << 1,      // identifies entry as Small Page == Page Table Entry
            // Access Permission bits, assuming SCTLR.AFE = 0
            AP0  = 1 << 4,  
            AP1  = 1 << 5,
            AP2  = 1 << 9,
            WR   = AP0,         // read/write, system
            RO   = AP2,         // read/only, system
            USR  = (AP1 | AP0),
            // TEX[2:0], C, B, S --> set shareability/cacheability
            B    = 1 << 2,      // bufferable
            C    = 1 << 3,      // cacheable
            TEX0 = 1 << 6,
            TEX1 = 1 << 7,
            TEX2 = 1 << 8,
            S    = 1 << 10,     // shareable
            nG   = 1 << 11,     // non-global (entry in the TLB)

            SDEV = B,           // shareable device memory (should not be used along with CT or CWT)
            CD   = TEX2,        // cache disable
            CWT  = (TEX2 | TEX1 | TEX0 | C | B),  // cacheable write through

            // Page Table flags
            APP  = (nG | S | AP1 | AP0 | CWT | PTE),        // S, RWX  All, Normal WT
            APPD = (nG | S | AP1 | AP0 | CWT | XN  | PTE),  // S, RWnX All, Normal WT
            APPC = (nG | S | AP2 | AP1 | AP0 | CWT | PTE),  // S, RnWX All, Normal WT
            SYS  = (nG | S | AP0 | CWT | PTE),              // S, RWX  SYS, Normal WT
            IO   = (nG | AP0 | SDEV | PTE),                 // Device Memory = Shareable, RWX, SYS
            DMA  = (nG | AP0 | SDEV | PTE),                 // Device Memory no cacheable / Old Peripheral = Shareable, RWX, B ?
            PT_MASK = (1 << 12) - 1
        };

        // Short-descriptor format | Page Directory entry flags
        enum {
            PDE  = 1 << 0,         // Set descriptor as Page Directory entry
            NS   = 1 << 3,         // NonSecure Memory Region
            PD_FLAGS = (NS | PDE),
            PD_MASK = (1 << 10) -1
        };

    public:
        Page_Flags() {}
        Page_Flags(unsigned long f) : _flags(f) {}
        Page_Flags(Flags f) : _flags(nG |
                                    ((f & Flags::WR)  ? WR   : RO) |
                                    ((f & Flags::USR) ? USR  : 0) |
                                    ((f & Flags::CWT) ? CWT  : 0) |
                                    ((f & Flags::CD)  ? CD   : 0) |
                                    ((f & Flags::EX)  ? 0    : XN) |
                                    ((f & Flags::IO)  ? SDEV : S) ) {}

        operator unsigned long() const { return _flags; }

        friend OStream & operator<<(OStream & os, const Page_Flags & f) { os << hex << f._flags; return os; }

    private:
        unsigned long _flags;
    };

    // Section Flags (for single-level, flat memory mapping)
    class Section_Flags
    {
    public:
        // Page Table entry flags
        enum : unsigned long {
            ID   = 0b10 << 0,   // memory section identifier
            B    = 1 << 2,      // bufferable
            C    = 1 << 3,      // cacheable
            XN   = 1 << 4,      // execute never
            AP0  = 1 << 10,
            AP1  = 1 << 11,
            TEX0 = 1 << 12,
            TEX1 = 1 << 13,
            TEX2 = 1 << 14,
            AP2  = 1 << 15,
            S    = 1 << 16,     // shareable
            nG   = 1 << 17,     // non-global (entry in the TLB)
            nS   = 1 << 19,     // non-secure
            FLAT_MEMORY_MEM = (nS | S | AP1 | AP0 |       C | B | ID),
            FLAT_MEMORY_DEV = (nS | S | AP1 | AP0 |       C |     ID),
            FLAT_MEMORY_PER = (nS | S | AP1 | AP0 |  XN |     B | ID)
        };

    public:
        Section_Flags(unsigned long f) : _flags(f) {}
        operator unsigned long() const { return _flags; }

    private:
        unsigned long _flags;
    };

    // Page Table
    template<unsigned int ENTRIES, Page_Type TYPE = PT>
    class _Page_Table
    {
    public:
        _Page_Table() {}

        PT_Entry & operator[](unsigned int i) { return _entry[i]; }
        _Page_Table & log() { return *static_cast<_Page_Table *>(phy2log(this)); }

        void map(int from, int to, Page_Flags flags, Color color) {
            Phy_Addr * addr = alloc(to - from, color);
            if(addr)
                remap(addr, from, to, flags);
            else
                for( ; from < to; from++) {
                    Log_Addr * pte = phy2log(&_entry[from]);
                    *pte = phy2pte(alloc(1, color), flags);
                }
        }

        void map_contiguous(int from, int to, Page_Flags flags, Color color) {
            remap(alloc(to - from, color), from, to, flags);
        }

        void remap(Phy_Addr addr, int from, int to, Page_Flags flags) {
            addr = align_page(addr);
            for( ; from < to; from++) {
                Log_Addr * pte = phy2log(&_entry[from]);
                *pte = phy2pte(addr, flags);
                addr += sizeof(Page);
            }
        }

        void reflag(int from, int to, Page_Flags flags) {
            for( ; from < to; from++) {
                Log_Addr * pte = phy2log(&_entry[from]);
                *pte = phy2pte(pte2phy(_entry[from]), flags);
            }
        }

        void unmap(int from, int to) {
            for( ; from < to; from++) {
                free(_entry[from]);
                Log_Addr * pte = phy2log(&_entry[from]);
                *pte = 0;
            }
        }

        friend OStream & operator<<(OStream & os, _Page_Table & pt) {
            os << "{\n";
            for(unsigned int i = 0; i < ENTRIES; i++)
                if(pt[i]) {
                    switch(TYPE) {
                    case PT:
                        os << "[" << i << "] \t" << pte2phy(pt[i]) << " " << hex << pte2flg(pt[i]) << dec << "\n";
                        break;
                    case PD:
                        os << "[" << i << "] \t" << pde2phy(pt[i]) << " " << hex << pde2flg(pt[i]) << dec << "\n";
                    }
                }
            os << "}";
            return os;
        }

    private:
        PT_Entry _entry[ENTRIES]; // the Phy_Addr in each entry passed through phy2pte()
    };

    typedef _Page_Table<PT_ENTRIES, PT> Page_Table;
    typedef _Page_Table<PD_ENTRIES, PD> Page_Directory;

    // Chunk (for Segment)
    class Chunk
    {
    public:
        Chunk(unsigned long bytes, Flags flags, Color color = WHITE)
        : _free(true), _from(0), _to(pages(bytes)), _pts(Common::pts(_to - _from)), _flags(Page_Flags(flags)), _pt(calloc(_pts, WHITE)) {
            if(!((_flags & Page_Flags::CWT) || (_flags & Page_Flags::CD))) // CT == Strongly Ordered == C/B/TEX bits are 0
                _pt->map_contiguous(_from, _to, _flags, color);
            else
                _pt->map(_from, _to, _flags, color);
        }

        Chunk(Phy_Addr phy_addr, unsigned long bytes, Flags flags)
        : _free(true), _from(0), _to(pages(bytes)), _pts(Common::pts(_to - _from)), _flags(Page_Flags(flags)), _pt(calloc(_pts, WHITE)) {
            _pt->remap(phy_addr, _from, _to, flags);
        }

        Chunk(Phy_Addr pt, unsigned int from, unsigned int to, Flags flags)
        : _free(false), _from(from), _to(to), _pts(Common::pts(_to - _from)), _flags(flags), _pt(pt) {}

        Chunk(Phy_Addr pt, unsigned int from, unsigned int to, Flags flags, Phy_Addr phy_addr)
        : _free(false), _from(from), _to(to), _pts(Common::pts(_to - _from)), _flags(flags), _pt(pt) {
            _pt->remap(phy_addr, _from, _to, flags);
        }

        ~Chunk() {
            if(_free) {
                if(!(_flags & Page_Flags::IO)) {
                    if(!((_flags & Page_Flags::CWT) || (_flags & Page_Flags::CD))) // CT == Strongly Ordered == C/B/TEX bits are 0
                        free((*_pt)[_from], _to - _from);
                    else
                        for( ; _from < _to; _from++)
                            free((*_pt)[_from]);
                }
                free(_pt, _pts);
            }
        }

        unsigned int pts() const { return _pts; }
        Page_Flags flags() const { return _flags; }
        Page_Table * pt() const { return _pt; }
        unsigned long size() const { return (_to - _from) * sizeof(Page); }
        
        void reflag(Flags flags) {
            _flags = flags;
            _pt->reflag(_from, _to, _flags);
        }

        Phy_Addr phy_address() const {
            return (!((_flags & Page_Flags::CWT) || (_flags & Page_Flags::CD))) ? Phy_Addr(unflag((*_pt)[_from])) : Phy_Addr(false);
            // CT == Strongly Ordered == C/B/TEX bits are 0
        }

        unsigned long resize(long amount) {
            if(!((_flags & Page_Flags::CWT) || (_flags & Page_Flags::CD))) // CT == Strongly Ordered == C/B/TEX bits are 0
                return 0;

            unsigned long pgs = pages(amount);

            Color color = colorful ? phy2color(_pt) : WHITE;

            unsigned long free_pgs = _pts * PT_ENTRIES - _to;
            if(free_pgs < pgs) { // resize _pt
                unsigned long pts = _pts + Common::pts(pgs - free_pgs);
                Page_Table * pt = calloc(pts, color);
                memcpy(phy2log(pt), phy2log(_pt), _pts * sizeof(Page));
                free(_pt, _pts);
                _pt = pt;
                _pts = pts;
            }

            _pt->map(_to, _to + pgs, _flags, color);
            _to += pgs;

            return size();
        }

    private:
        bool _free;
        unsigned int _from;
        unsigned int _to;
        unsigned int _pts;
        Page_Flags _flags;
        Page_Table * _pt; // this is a physical address
    };

    // Directory (for Address_Space)
    class Directory
    {
    public:
        Directory() : _free(true) {
            // Page Directories have 4096 32-bit entries and must be aligned to 16Kb, thus, we need 7 frame in the worst case
            Phy_Addr pd = calloc(sizeof(Page_Directory) / sizeof(Frame) + ((sizeof(Page_Directory) / sizeof(Frame)) - 1), WHITE);
            unsigned int free_frames = 0;
            while(pd & (sizeof(Page_Directory) - 1)) { // pd is not aligned to 16 Kb
                Phy_Addr * tmp = pd;
                pd += sizeof(Frame); // skip this frame
                free(tmp); // return this frame to the free list
                free_frames++;
            }
            if(free_frames != 3)
                free(pd + 4 * sizeof(Page), 3 - free_frames); // return exceeding frames at the tail to the free list

            _pd = static_cast<Page_Directory *>(pd);

            for(unsigned int i = 0; i < PD_ENTRIES; i++)
                if(!((i >= pdi(APP_LOW)) && (i <= pdi(APP_HIGH))))
                    _pd->log()[i] = _master->log()[i];
        }

        Directory(Page_Directory * pd) : _free(false), _pd(pd) {}

        ~Directory() { if(_free) free(_pd, sizeof(Page_Directory) / sizeof(Page)); }

        Phy_Addr pd() const { return _pd; }

        void activate() const { ARMv7_MMU::pd(_pd); }

        Log_Addr attach(const Chunk & chunk, unsigned int from = pdi(APP_LOW)) {
            for(unsigned int i = from; i < pdi(APP_HIGH); i++)
                if(attach(i, chunk.pt(), chunk.pts(), chunk.flags()))
                    return i << PD_SHIFT;
            return Log_Addr(false);
        }

        Log_Addr attach(const Chunk & chunk, Log_Addr addr) {
            unsigned int from = pdi(addr);
            if((from + chunk.pts()) > PD_ENTRIES)
                return Log_Addr(false);
            if(attach(from, chunk.pt(), chunk.pts(), chunk.flags()))
                return from << PD_SHIFT;
            return Log_Addr(false);
        }

        void detach(const Chunk & chunk) {
            for(unsigned int i = 0; i < PD_ENTRIES; i++) {
                if(unflag(pte2phy((*_pd)[i])) == unflag(chunk.pt())) {
                    detach(i, chunk.pt(), chunk.pts());
                    return;
                }
            }
            db<MMU>(WRN) << "MMU::Directory::detach(pt=" << chunk.pt() << ") failed!" << endl;
        }

        void detach(const Chunk & chunk, Log_Addr addr) {
            unsigned int from = pdi(addr);
            if(unflag(pte2phy((*_pd)[from])) != unflag(chunk.pt())) {
                db<MMU>(WRN) << "MMU::Directory::detach(pt=" << chunk.pt() << ",addr=" << addr << ") failed!" << endl;
                return;
            }
            detach(from, chunk.pt(), chunk.pts());
        }

        Phy_Addr physical(Log_Addr addr) {
            PD_Entry pde = (*_pd)[pdi(addr)];
            Page_Table * pt = static_cast<Page_Table *>(pde2phy(pde));
            PT_Entry pte = pt->log()[pti(addr)];
            return pte | off(addr);
        }

    private:
        bool attach(unsigned int from, const Page_Table * pt, unsigned int n, Page_Flags flags) {
            for(unsigned int i = from; i < from + n; i++)
                if(_pd->log()[i])
                    return false;
            for(unsigned int i = from; i < from + n; i++, pt++)
                _pd->log()[i] = phy2pde(Phy_Addr(pt));
            return true;
        }

        void detach(unsigned int from, const Page_Table * pt, unsigned int n) {
            for(unsigned int i = from; i < from + n; i++) {
                _pd->log()[i] = 0;
                flush_tlb(i << PD_SHIFT);
            }
            CPU::isb();
            CPU::dsb();
        }

    private:
        bool _free;
        Page_Directory * _pd;  // this is a physical address, but operator*() returns a logical address
    };

    // DMA_Buffer
    class DMA_Buffer: public Chunk
    {
    public:
        DMA_Buffer(unsigned long s) : Chunk(s, Page_Flags::DMA) {
            Directory dir(current());
            _log_addr = dir.attach(*this);
            db<MMU>(TRC) << "MMU::DMA_Buffer() => " << *this << endl;
        }

        DMA_Buffer(unsigned long s, Log_Addr d): Chunk(s, Page_Flags::DMA) {
            Directory dir(current());
            _log_addr = dir.attach(*this);
            memcpy(_log_addr, d, s);
            db<MMU>(TRC) << "MMU::DMA_Buffer(phy=" << *this << " <= " << d << endl;
        }

        Log_Addr log_address() const { return _log_addr; }

        friend OStream & operator<<(OStream & os, const DMA_Buffer & b) {
            os << "{phy=" << b.phy_address() << ",log=" << b.log_address() << ",size=" << b.size() << ",flags=" << b.flags() << "}";
            return os;
        }

    private:
        Log_Addr _log_addr;
    };

    // Class Translation performs manual logical to physical address translations for debugging purposes only
    class Translation
    {
    public:
        Translation(Log_Addr addr, bool pt = false, Page_Directory * pd = 0): _addr(addr), _show_pt(pt), _pd(pd) {}

        friend OStream & operator<<(OStream & os, const Translation & t) {
            Page_Directory * pd = t._pd ? t._pd : current();
            PD_Entry pde = pd->log()[pdi(t._addr)];
            Page_Table * pt = static_cast<Page_Table *>(pde2phy(pde));
            PT_Entry pte = pt->log()[pti(t._addr)];

            os << "{addr=" << static_cast<void *>(t._addr) << ",pd=" << pd << ",pd[" << pdi(t._addr) << "]=" << pde << ",pt=" << pt;
            if(t._show_pt)
                os << "=>" << pt->log();
            os << ",pt[" << pti(t._addr) << "]=" << pte << ",f=" << pte2phy(pte) << ",*addr=" << hex << *static_cast<unsigned int *>(t._addr) << "}";
            return os;
        }

    private:
        Log_Addr _addr;
        bool _show_pt;
        Page_Directory * _pd;
    };

public:
    ARMv7_MMU() {}

    static Phy_Addr alloc(unsigned long frames = 1, Color color = WHITE) {
        Phy_Addr phy(false);

        if(frames) {
            List::Element * e = _free[color].search_decrementing(frames);
            if(e) {
                phy = e->object() + e->size();
                db<MMU>(TRC) << "MMU::alloc(frames=" << frames << ",color=" << color << ") => " << phy << endl;
            } else
                if(colorful)
                    db<MMU>(INF) << "MMU::alloc(frames=" << frames << ",color=" << color << ") => failed!" << endl;
                else
                    db<MMU>(WRN) << "MMU::alloc(frames=" << frames << ",color=" << color << ") => failed!" << endl;
        }

        return phy;
    }

    static Phy_Addr calloc(unsigned long frames = 1, Color color = WHITE) {
        Phy_Addr phy = alloc(frames, color);
        memset(phy2log(phy), 0, sizeof(Frame) * frames);
        return phy;
    }

    static void free(Phy_Addr frame, unsigned long n = 1) {
        // Clean up MMU flags in frame address
        frame = unflag(frame);
        Color color = colorful ? phy2color(frame) : WHITE;

        db<MMU>(TRC) << "MMU::free(frame=" << frame << ",color=" << color << ",n=" << n << ")" << endl;

        if(frame && n) {
            List::Element * e = new (phy2log(frame)) List::Element(frame, n);
            List::Element * m1, * m2;
            _free[color].insert_merging(e, &m1, &m2);
        }
    }

    static void white_free(Phy_Addr frame, unsigned long n) {
        // Clean up MMU flags in frame address
        frame = unflag(frame);

        db<MMU>(TRC) << "MMU::free(frame=" << frame << ",color=" << WHITE << ",n=" << n << ")" << endl;

        if(frame && n) {
            List::Element * e = new (phy2log(frame)) List::Element(frame, n);
            List::Element * m1, * m2;
            _free[WHITE].insert_merging(e, &m1, &m2);
        }
    }

    static unsigned long allocable(Color color = WHITE) { return _free[color].head() ? _free[color].head()->size() : 0; }

    static Page_Directory * volatile current() { return static_cast<Page_Directory * volatile>(pd()); }

    static Phy_Addr physical(Log_Addr addr) {
        Page_Directory * pd = current();
        Page_Table * pt = pd->log()[pdi(addr)];
        return pt->log()[pti(addr)] | off(addr);
    }

    static PT_Entry phy2pte(Phy_Addr frame, Page_Flags flags) { return (frame) | flags | Page_Flags::PTE; }
    static Phy_Addr pte2phy(PT_Entry entry) { return (entry & ~Page_Flags::PT_MASK); }
    static Page_Flags pte2flg(PT_Entry entry) { return (entry & Page_Flags::PT_MASK); }
    static PD_Entry phy2pde(Phy_Addr frame) { return (frame) | Page_Flags::PD_FLAGS; }
    static Phy_Addr pde2phy(PD_Entry entry) { return (entry & ~Page_Flags::PD_MASK); }
    static Page_Flags pde2flg(PT_Entry entry) { return (entry & Page_Flags::PD_MASK); }

#ifdef __setup__
    // SETUP may use the MMU to build a primordial memory model before turning the MMU on, so no log vs phy adjustments are made
    static Log_Addr phy2log(Phy_Addr phy) { return Log_Addr((RAM_BASE == PHY_MEM) ? phy : (RAM_BASE > PHY_MEM) ? phy : phy ); }
    static Phy_Addr log2phy(Log_Addr log) { return Phy_Addr((RAM_BASE == PHY_MEM) ? log : (RAM_BASE > PHY_MEM) ? log : log ); }
#else
    static Log_Addr phy2log(Phy_Addr phy) { return Log_Addr((RAM_BASE == PHY_MEM) ? phy : (RAM_BASE > PHY_MEM) ? phy - (RAM_BASE - PHY_MEM) : phy + (PHY_MEM - RAM_BASE)); }
    static Phy_Addr log2phy(Log_Addr log) { return Phy_Addr((RAM_BASE == PHY_MEM) ? log : (RAM_BASE > PHY_MEM) ? log + (RAM_BASE - PHY_MEM) : log - (PHY_MEM - RAM_BASE)); }
#endif

    static Color phy2color(Phy_Addr phy) { return static_cast<Color>(colorful ? ((phy >> PT_SHIFT) & 0x7f) % COLORS : WHITE); } // TODO: what is 0x7f

    static Color log2color(Log_Addr log) {
        if(colorful) {
            Page_Directory * pd = current();
            Page_Table * pt = pd->log()[pdi(log)];
            Phy_Addr phy = pt->log()[pti(log)] | off(log);
            return static_cast<Color>(((phy >> PT_SHIFT) & 0x7f) % COLORS);
        } else
            return WHITE;
    }

private:
    static Phy_Addr pd() { return CPU::pd(); }
    static void pd(Phy_Addr pd) { CPU::pd(pd); CPU::flush_tlb(); CPU::isb(); CPU::dsb(); }

    static void flush_tlb() { CPU::flush_tlb(); }
    static void flush_tlb(Log_Addr addr) { CPU::flush_tlb(directory_bits(addr)); } // only bits from 31 to 12, all ASIDs

    static void init();

private:
    static List _free[colorful * COLORS + 1]; // +1 for WHITE
    static Page_Directory * _master;
};

class MMU: public IF<Traits<System>::multitask || (Traits<Build>::MODEL == Traits<Build>::Raspberry_Pi3), ARMv7_MMU, No_MMU>::Result {};

__END_SYS

#endif
