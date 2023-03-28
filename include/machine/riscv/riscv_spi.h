// EPOS RISC-V SPI Mediator Declarations

#ifndef __riscv_spi_h
#define __riscv_spi_h

#include <architecture/cpu.h>
#include <machine/spi.h>
#include <system/memory_map.h>
#include <utility/ostream.h>

using namespace EPOS;
OStream cout;

__BEGIN_SYS


class SPI_E : public SPI_Common
{
private:
    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg32 Reg32;
public:

    static const unsigned int RXWM_MASK = 0x000000ff;
    static const unsigned int TXWM_MASK = 0x0000ff00;

      // SPI registers offsets from SPI_BASE
    enum {
        SCKDIV  = 0x00, // Clock divisor register
        SCKMODE = 0x04, // SPI mode and control register
        CSID    = 0x10, // Chip select ID register
        CSDEF   = 0x14, // Chip select default register
        CSMODE  = 0x18, // Chip select mode register
        DELAY0  = 0x28, // Delay control register 0
        DELAY1  = 0x2c, // Delay control register 1
        FMT     = 0x40, // Frame format
        TXDATA  = 0x48, // Transmit data register
        RXDATA  = 0x4c, // Receive data register
        TXMARK  = 0x50, // Transmit watermark register
        RXMARK  = 0x54, // Receive watermark register
        FCTRL   = 0x60, // Flash interface control register
        FFMT     = 0x64, // Flash interface timing register
        IE      = 0x70, // Interrupt enable register
        IP      = 0x74, // Interrupt pending register
        DIV     = 0x80, // Set the SPI clock frequency
        DATA    = 0xff << 0
    };

    SPI_E() {
        config(50000000, 0, 0, 1000000, 8);
    }

    void config(unsigned int clock, unsigned int protocol, unsigned int mode, unsigned int bit_rate, unsigned int data_bits) {
        // Set the clock divisor
        unsigned int sckdiv = clock / bit_rate - 1;
        reg(SCKDIV) = sckdiv;

        // Set the SPI mode and control register
        unsigned int sckmode =  0x0;
        reg(SCKMODE) = sckmode;

        // Enable loopback mode
        reg(SCKMODE) |= (1 << 7);

        // Set the frame format
        //unsigned int fmt = (data_bits - 1) << 16;
            //reg(FMT) = fmt;
        unsigned int fmt = 0x0;  // clear all the bits
        fmt |= (7 << 16);       // set frame size to 8 bits
        reg(FMT) = fmt;

        // Set the SPI clock frequency
        //reg(DIV) = sckdiv;
        reg(IE) = 0x03;
        reg(TXMARK) =  0x01;
        reg(RXMARK) = 0x0;
        }

    void put(unsigned char data) {
        // Wait until there is space to write
        while ((reg(IP) & 0x1) == 0) {}

        // Write the data
        reg(TXDATA) = data;
    }

    unsigned char get() {
        // Wait until there is data to read
        while ((reg(IP) & 0x2) == 0) {}

        // Read the data
        unsigned char data = reg(RXDATA);
        cout << "RXDATA:" << data << endl;
    
        return data;
    }
    /*char get() {
        // Wait until there is data to read
        while ((reg(IP) & 0x2) != 0x2) {
            cout << "IP:" << reg(IP) << endl;
    
        }
        
        // Read the data
        int data = reg(RXDATA);

        cout << "RXDATA:" << data << endl;
    
        return data;
    }

    */

    bool try_get(int * data) {
        // Check if there is data to read
        if (!ready_to_get()){return false;}

        // Read the data
        *data = reg(RXDATA);
        return true;
    }

    void put(int data) {
        // Wait until there is space to write
        // while ((reg(IP) & 0x2) == 0) {}
        while (!ready_to_put()) {}

        // Write the data
        reg(TXDATA) = data;
    }

    bool try_put(int data) {
        // Check if there is space to write
        if (!ready_to_put()){return false;}

        // Write the data
        reg(TXDATA) = data & DATA;
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
        while ((reg(IP) & 0x3) != 0) {}
    }

    bool ready_to_get()
    {
        //if (!(reg(IP) & RXWM_MASK))
          //  return false;

        //return true;
        return ((reg(IP) & (1 << 0)) != 0);
    }

    bool ready_to_put() {
        // Check if TXFIFO is not full
       return ((reg(TXDATA) & 1) == 0);
    }
    

private:
    static volatile CPU::Reg32 & reg(unsigned int o) { return reinterpret_cast<volatile CPU::Reg32 *>(Memory_Map::QSPI1_BASE)[o / sizeof(CPU::Reg32)]; 
}
};

__END_SYS

#endif
