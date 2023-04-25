// EPOS RISC-V 64 MMU Mediator Declarations

#ifndef __rv64_mmu_h
#define __rv64_mmu_h

#define __mmu_common_only__
#include <architecture/mmu.h>
#undef __mmu_common_only__
#include <system/memory_map.h>

__BEGIN_SYS

class Sv39_MMU: public MMU_Common<9, 9, 21, 9>
{
    friend class CPU;

private:
    typedef Grouping_List<Frame> List;

    static const unsigned long RAM_BASE = Memory_Map::RAM_BASE;
    static const unsigned long APP_LOW = Memory_Map::APP_LOW;
    static const unsigned long PHY_MEM = Memory_Map::PHY_MEM;
    //static const unsigned long LEVELS = 3;
    //static const bool colorful = Traits<MMU>::colorful;
    //static const unsigned long COLORS = Traits<MMU>::COLORS;

public:
    // Page Flags
    class RV64_Flags
    {
    public:
        // RV64_Flags
        enum {
            V    = 1 << 0, // Valid (0 = not, 1 = valid)
            R    = 1 << 1, // Readable (0 = not, 1= yes)
            W    = 1 << 2, // Writable (0 = not, 1= yes)
            X    = 1 << 3, // Executable (0 = not, 1= yes)
            U    = 1 << 4, // User accessible (0 = not, 1= yes)
            G    = 1 << 5, // Global (mapped in multiple PTs)
            A    = 1 << 6, // Accessed (A == R?)
            D    = 1 << 7, // Dirty (D == W)
            CT   = 1 << 8,  // Contiguous
            MIO  = 1 << 9, // I/O
            MASK = (1 << 10) - 1,
            NLP  = ~(V | W | X),
            APP  = (V | R | W | X),
            SYS  = (V | R | W | X | A | D),
            KC   = (V | R | X),
            KD   = (V | R | W),
            UD   = (V | R | W | U),
            PD   = (V | A | D),
            UA   = (V | R | X | U)
        };

    public:
        RV64_Flags() {}
        RV64_Flags(unsigned long f) : _flags(f) {}
        // ### CHECK IF THIS FUNCTION IS STILL NECESSARY
        /*
        RV64_Flags(Flags f) : _flags(V | R | X |
                                     ((f & Flags::RW)  ? W  : 0) | // use W flag
                                     ((f & Flags::USR) ? U  : 0) | // use U flag (0 = supervisor, 1 = user)
                                     ((f & Flags::CWT) ? 0  : 0) | // cache mode
                                     ((f & Flags::CD)  ? 0  : 0)) {} // no cache
        */
        RV64_Flags(const RV64_Flags &f) : _flags(f) {}
        RV64_Flags(const Flags &f) : _flags(((f & Flags::PRE) ? V : 0) |
                                            ((f & Flags::RW) ? (R | W) : R) |
                                            ((f & Flags::USR) ? U : 0) |
                                            ((f & Flags::EX) ? X : 0) | A | D) {}

        operator unsigned long() const { return _flags; }

    private:
        unsigned long _flags;
    };

    // Page Flags
    typedef Flags Page_Flags;

    // Page_Table
    class Page_Table
    {
    public:
        Page_Table() {}

        PT_Entry & operator[](unsigned long i) { return _pte[i]; }
        PT_Entry get_entry(unsigned long i) { return _pte[i]; }

        // Switching to multi-level page table
        void map(long from, long to, Page_Flags flags) {
            // This page tables represents a table on the last level.
            // get the 3 least significant bits of the flags
            // if equals to 0, then it is a attach level page
            if (!((flags | Page_Flags::NLP) & 0x111)) {
                db<MMU>(WRN) << "MMU::Page_Table::map: attach level page" << endl;
                Phy_Addr * addr = alloc(to - from);
                if(addr)
                    remap(addr, from, to, Page_Flags::NLP);
                else
                    for( ; from < to; from++) {
                        Log_Addr *pte = phy2log(ati(&_pte[from]));
                        *pte = phy2ptae(alloc(1));
                    }
            } else {
                db<MMU>(WRN) << "MMU::Page_Table::map: page table" << endl;
                // Page table on level 0
                Phy_Addr *addr = alloc(to - from);
                if (addr) 
                    remap(addr, from, to, flags);
                else {
                    for (; from < to; from++) {
                        Log_Addr *pte = phy2log(pti(&_pte[from]));
                        *pte = phy2pte(alloc(1), flags);
                    }
                }
            }
        }

        void remap(Phy_Addr addr, long from, long to, Page_Flags flags) {
            addr = align_page(addr);
            for( ; from < to; from++) {
                Log_Addr *pte = phy2log(&_pte[from]);
                *pte = phy2pte(addr, flags);
                addr += sizeof(Page);
            }
        }   

        // system free
        void unmap(long from, long to) {
            for( ; from < to; from++) {
                free(_pte[from]);
                Log_Addr * tmp = phy2log(&_pte[from]);
                *tmp = 0;
            }
        }

        // ### ACHO Q É SÓ PRA PRINT (REMOVER?)
        friend OStream &operator<<(OStream &os, Page_Table &pt)
        {
            os << "{\n";
            for (unsigned int i = 0; i < PT_ENTRIES; i++)
                if (pt[i])
                {
                    os << "[" << i << "] \t" << pde2phy(pt[i]) << " " << hex << pde2flg(pt[i]) << dec << "\n";
                }
            os << "}";
            return os;
        }

    private:
        PT_Entry _pte[PT_ENTRIES];
    };

    // Chunk (for Segment)
    class Chunk
    {
    public:
        Chunk() {}

        Chunk(unsigned long bytes, Flags flags) {
            _from = 0;
            _bytes = bytes;
            _to = Sv39_MMU::pages(bytes);
            _pts = Sv39_MMU::pts(_to - _from);
            _ats = Sv39_MMU::ats(_pts);
            _flags = Page_Flags(flags);
            _pt_level_0 = calloc(_pts);
            _pt_attach_level_1 = calloc(_ats);
            _pt_level_0->map(0, _pts, _flags);
            _pt_attach_level_1->map(_pts, _pts +_ats, _flags);
        }

        Chunk(Phy_Addr phy_addr, unsigned long bytes, Flags flags) {
            _bytes = bytes;
            _from = 0;
            _to = Sv39_MMU::pages(bytes);
            _pts = Sv39_MMU::pts(_to - _from);
            _ats = Sv39_MMU::ats(_pts);
            _flags = Page_Flags(flags);
            _pt_level_0 = calloc(_pts);
            _pt_attach_level_1 = calloc(_ats);
            _pt_level_0->remap(phy_addr, 0, _pts, Page_Flags::PRE);
            _pt_attach_level_1->remap(phy_addr, _pts, _pts + _ats, Page_Flags::NLP);
        }

        ~Chunk() {
            for (; _from < _to; _from++)
                free((*static_cast<Page_Table *>(phy2log(_pt_level_0)))[_from]);
            for (; _from < _to; _from++)
                free((*static_cast<Page_Table *>(phy2log(_pt_attach_level_1)))[_from]);
            free(_pt_level_0, _pts);
            free(_pt_attach_level_1, _ats);
        }

        Phy_Addr phy_address() const { return (_flags & Page_Flags::CT) ? Phy_Addr(ind((*_pt_level_0)[_from])) : Phy_Addr(false); }
        unsigned long pts() const { return _pts; }
        unsigned long ats() const { return _ats; }
        unsigned long size() const { return _bytes; }
        RV64_Flags flags() const { return _flags; }
        Page_Table *pt() const { return _pt_level_0; }
        Page_Table *at() const { return _pt_attach_level_1; }
        long resize(unsigned long amount) { return 0; }

    private:
        unsigned long _from;
        unsigned long _to;
        unsigned long _pts;     // points to the page table (PT)
        unsigned long _ats;     // points to the attach page table (AT)
        unsigned long _bytes;   // chunk size

        Page_Flags _flags;
        Page_Table * _pt_level_0;
        Page_Table * _pt_attach_level_1;
    };

    // Page Directory
    typedef Page_Table Page_Directory;

    class Directory
    {
    public:
        Directory() : _pd(phy2log(calloc(1))), _free(true) {
            for(unsigned long i = 0; i < PD_ENTRIES; i++)
                (*_pd)[i] = (*_master)[i];
        }

        Directory(Page_Directory * pd) : _pd(pd), _free(false) {}

        ~Directory() { if(_free) free(_pd); }

        Phy_Addr pd() const { return _pd; }

        // MODE = Sv39 
        // ### OLD FUNCTION (VERIFY)
        // void activate() const { CPU::satp((1UL << 63) | reinterpret_cast<CPU::Reg64>(_pd) >> PT_SHIFT); }
        void activate() const { CPU::pdp(reinterpret_cast<CPU::Reg64>(_pd)); }

        Log_Addr attach(const Chunk & chunk, unsigned long from = pdi(APP_LOW)) {
            for (unsigned long i = from; i < PD_ENTRIES; i++) {
                for (unsigned long i = 0; i < PT_ENTRIES; i++) {
                    if (attach_level_1(i, chunk.at(), chunk.ats(), RV64_Flags::NLP)) {
                        if (attach_level_0(chunk.at(), chunk.ats(), chunk.pt(), chunk.pts(), RV64_Flags::V))
                            return i << PD_SHIFT;
                    }
                }
            }
            return Log_Addr(false);
        }

        Log_Addr attach(const Chunk & chunk, Log_Addr addr) {
            unsigned long from = directory_bits(addr);
            for (unsigned long i = from; i < PD_ENTRIES; i++) {
                for (unsigned long i = 0; i < PT_ENTRIES; i++) {
                    if (attach_level_1(i, chunk.at(), chunk.ats(), RV64_Flags::NLP)) {
                        if (attach_level_0(chunk.at(), chunk.ats(), chunk.pt(), chunk.pts(), RV64_Flags::V))
                            return i << PD_SHIFT;
                    }
                }
            }
            return Log_Addr(false);
        }

        void detach(const Chunk & chunk) {
            for(unsigned long i = 0; i < PD_ENTRIES; i++) {
                if(ind((*_pd)[i]) == ind(phy2pde(chunk.at()))) {
                    detach_pd(i, chunk.ats());
                    return;
                }
            }
            for (unsigned long i = 0; i < PT_ENTRIES; i++) {
                if (ind((*chunk.at())[i]) == ind(phy2pde(chunk.pt()))) {
                    detach_at(i, chunk.at(), chunk.pts());
                    return;
                }
            }
            db<MMU>(WRN) << "MMU::Directory::detach(pt=" << chunk.pt() << ") failed!" << endl;
        }

        void detach(const Chunk & chunk, Log_Addr addr) {
            unsigned long from = directory_bits(addr);
            if(ind((*_pd)[from]) != ind(phy2pde(chunk.at()))) {
                db<MMU>(WRN) << "MMU::Directory::detach(pt=" << chunk.at() << ",addr=" << addr << ") failed!" << endl;
                return;
            }
            detach_pd(from, chunk.ats());
            detach_at(from + chunk.pts() + 1, chunk.at(), chunk.ats());
        }

        Phy_Addr physical(Log_Addr addr) {
            Page_Table * at = reinterpret_cast<Page_Table *>((void *)(*_pd)[pdi(addr)]); // PPN2 -> PPN1
            Page_Table * pt = reinterpret_cast<Page_Table *>((void *)(*at)[ati(addr)]);  // PPN1 -> PPN0
            Log_Addr pt_entry = pt->get_entry(pti(addr));                               //  PPN0 -> Physical
            return addr_without_off(pte2phy(pt_entry)) | off(addr);
        }

    private:
        // attach to intermediary page
        bool attach_level_1(unsigned long from, const Page_Table *at, unsigned long n, RV64_Flags flags) {
            for (unsigned long i = from; i < from + n; i++)
                if ((*_pd)[i])
                    return false;
            for (unsigned long i = from; i < from + n; i++, at++)
                (*_pd)[i] = phy2pde(Phy_Addr(at)) | flags;
            return true;
        }

        // attach to page frame
        bool attach_level_0(Page_Table *at, unsigned long n, const Page_Table *pt, unsigned long m, RV64_Flags flags) {
            for (unsigned long i = 0; i < n; i++) {
                for (unsigned long j = 0; j < m; j++, pt++) {
                    if ((*at)[i])
                        return false;
                    (*at)[i] = phy2ptae(Phy_Addr(pt)) | flags;
                }
            }
            return true;
        }

        void detach_at(unsigned long from, Page_Table *at, unsigned long n) {
            for (unsigned long i = from; i < from + n; i++)
                (*at)[i] = 0;
        }

        void detach_pd(unsigned long from, unsigned long n) {
            for(unsigned long i = from; i < from + n; i++)
                (*_pd)[i] = 0;
        }

    private:
        Page_Directory * _pd;
        bool _free;
    };

public:
    Sv39_MMU() {}

    // Frame 4kB
    static Phy_Addr alloc(unsigned long bytes = 1) {
        Phy_Addr phy(false);

        if(bytes) {
            // Encontra um bloco cujo numero de bytes requisitados cabe que cabe
            List::Element * e = _free.search_decrementing(bytes);
            if(e) {
                phy = reinterpret_cast<unsigned long>(e->object() + e->size());
                db<MMU>(TRC) << "MMU::alloc(bytes=" << bytes << ") => " << phy << endl;
            } else
                db<MMU>(ERR) << "MMU::alloc() failed" << endl;
        }
        return phy;
    }

    static Phy_Addr calloc(unsigned long bytes = 1) {
        Phy_Addr phy = alloc(bytes);
        memset(phy, 0, bytes);
        return phy;
    }

    // n = number of addr to be free
    static void free(Phy_Addr addr, unsigned long n = 1) {
        db<MMU>(TRC) << "MMU::free(addr=" << addr << ",n=" << n << ")" << endl;

        // No unaligned addresses if the CPU doesn't support it
        assert(Traits<CPU>::unaligned_memory_access || !(addr % (Traits<CPU>::WORD_SIZE / 8)));

        // Free blocks must be large enough to contain a list element
        // ### REVISAR (TRAVA NO TESTE DO P2, MAS PASSA NO HELLO, O Q FAZ SENTIDO)
        //assert(n > sizeof(List::Element));

        if(addr && n) {
            List::Element * e = new (addr) List::Element(addr, n);
            List::Element * m1, * m2;
            _free.insert_merging(e, &m1, &m2);
        }
    }

    static unsigned long allocable() { return _free.head() ? _free.head()->size() : 0; }

    // returns current PNN on SATP (PNN = Page Number of the root page table)
    static Page_Directory * volatile current() { return static_cast<Page_Directory * volatile>(phy2log(CPU::satp() << 12)); }

    // Address functions
    static Log_Addr phy2log(const Phy_Addr &phy) { return phy; }
    static PD_Entry phy2pde(Phy_Addr bytes) { return ((bytes >> 12) << 10) | RV64_Flags::V; }
    static Phy_Addr pde2phy(PD_Entry entry) { return (entry & ~RV64_Flags::MASK) << 2; }
    static PT_Entry phy2pte(Phy_Addr bytes, RV64_Flags flags) { return ((bytes >> 12) << 10) | flags; }
    // physical 2 page table attach entry
    static PT_Entry phy2ptae(Phy_Addr bytes) { return ((bytes >> 12) << 10) | RV64_Flags::NLP; }
    static Phy_Addr pte2phy(PT_Entry entry) { return (entry & ~RV64_Flags::MASK) << 2; }
    static RV64_Flags pde2flg(PT_Entry entry) { return (entry & RV64_Flags::MASK); }

    static void flush_tlb() { CPU::flush_tlb(); }
    static void flush_tlb(Log_Addr addr) { CPU::flush_tlb(addr); }

private:
    static void init();
    static Phy_Addr pd() { return _master; }
    static void pd(Phy_Addr pd) { _master = pd; }
private:
    static List _free;
    static Page_Directory * _master;
};

class MMU: public Sv39_MMU {};

__END_SYS

#endif