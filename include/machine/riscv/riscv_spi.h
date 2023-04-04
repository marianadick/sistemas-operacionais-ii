// EPOS RISC-V SPI Mediator Declarations

#ifndef __riscv_spi_h
#define __riscv_spi_h

#include <architecture/cpu.h>
#include <machine/spi.h>
#include <system/memory_map.h>
#include <utility/ostream.h>

using namespace EPOS;


__BEGIN_SYS


class SPI_E : public SPI_Common
{
private:
    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg32 Reg32;
public:

    enum {
        SCKDIV  = 0x00, // clock divisor
        SCKMODE = 0x04, // SPI mode 
        CSID    = 0x10, // Chip select 
        CSDEF   = 0x14, // Chip select default 
        CSMODE  = 0x18, // Chip select mode 
        DELAY0  = 0x28, // Delay control 0
        DELAY1  = 0x2c, // Delay control 1
        FMT     = 0x40, // Frame format
        TXDATA  = 0x48, // Transmit data 
        RXDATA  = 0x4c, // Receive data 
        TXMARK  = 0x50, // Transmit watermark 
        RXMARK  = 0x54, // Receive watermark 
        FCTRL   = 0x60, // Flash interface control 
        FFMT     = 0x64, // Flash interface timing 
        IE      = 0x70, // Interrupt enable 
        IP      = 0x74, // Interrupt pending 
        DIV     = 0x80 // Set the SPI clock frequency
        
    };

    static const unsigned int CLOCK = Traits<SPI>::CLOCK;

    SPI_E() {
        config(CLOCK, 0, 0, 250000, 8);
    }

    void config(unsigned int clock, unsigned int protocol, unsigned int mode, unsigned int bit_rate, unsigned int data_bits) {
        // Set the clock divisor
        unsigned int sckdiv = (clock / (2 * bit_rate) ) - 1;
        reg(SCKDIV) = sckdiv & 0xffffffff;
        
        // Set the SPI mode and control register
        unsigned int sckmode =  0x0;
        reg(SCKMODE) = sckmode;


        reg(CSMODE) = 0x01; //mode hold
        reg(CSID) = 0x00; //choose first id (see device tree and the spi@10050000 device)

        // Set the frame format
        unsigned int fmt = 0x0;  // clear all the bits
        fmt = ((protocol << 0) | (0 << 2) | (mode << 3) | (data_bits << 16)); // set endian, mode, protocol and data_bits
        reg(FMT) = fmt;
        
        // Enable interruptions
        reg(IE) = 0x03;
        reg(TXMARK) =  0x01;
        reg(RXMARK) = 0x00;
    }

    void put(int data) {
        // Wait until there is space to write
        // another approach is described in the ready_to_put function
        while ((reg(IP) & 0x1) == 0) {
            /*
                The IP register is a read-only register that 
                stores the status of various interrupt flags. 
                In this case, the IP[0] bit indicates whether 
                the transmit buffer is full or not. If the IP[0] 
                bit is set to 0, it means there is space available 
                in the buffer to write new data
            
            */
        }

        // Write the data
        reg(TXDATA) = data;
    }

    int get() {
        // Wait until there is data to read
        // another approach is described in the ready_to_get function
        while ((reg(IP) & 0x2) == 0) {
            // same explanation of the put function
            // we used this approach because we could skip reading the register
            // if we dont want to consume its data
        }

        // Read the data
        int data = reg(RXDATA);
    
        return data;
    }
    

    bool try_get(int * data) {
        // Check if there is data to read
        if (!ready_to_get()){return false;}

        // Read the data
        *data = reg(RXDATA);

        return true;
    }


    bool try_put(int data) {
        // Check if there is space to write
        if (!ready_to_put()){return false;}

        // Write the data
        reg(TXDATA) = data;
        return true;
    }

    int read(char * data, unsigned int max_size) {
        unsigned int count = 0;

        // Read data until we reach the maximum size or there is no more data
        while (count < max_size && try_get((int *) (data + count))) {
            count++;
        }

        return count;
    }

    int write(const char * data, unsigned int size) {
        unsigned int count = 0;

        // Write data until we reach the maximum size or there is no more space
        while (count < size && try_put((int) *(data + count))) {
            count++;
        }

        return count;
    }

    void flush() {
        // Flush the SPI
        //The purpose of this loop is to wait until the SPI interface is 
        //not busy before proceeding with the next operation.
        while ((reg(IP) & 0x3) != 0) {}
    }

    bool ready_to_get()
    {
        // we decided to use the IP for dont read the rxdata
        // another approach is the !(reg(RXDATA) & (1 << 31)) 
        // but using it would make we consume the register
        return ((reg(IP) & 0x2) != 0);
    }

    bool ready_to_put() {
        // Check if TXFIFO is not full
        // another approach is the !(reg(TXDATA) & (1 << 31)) 
       return (((reg(IP) & 0x1) != 0));
    }
    

private:
    static volatile CPU::Reg32 & reg(unsigned int o) { return reinterpret_cast<volatile CPU::Reg32 *>(Memory_Map::QSPI1_BASE)[o / sizeof(CPU::Reg32)]; 
}
};

__END_SYS

#endif
