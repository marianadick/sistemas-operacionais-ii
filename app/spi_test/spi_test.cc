#include <machine/riscv/riscv_spi.h>
#include <utility/ostream.h>

using namespace EPOS;

int main() {
    

    // Configure SPI
    SPI_E spi;
    
    char init_cmd[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95, 0xff};
    spi.write(init_cmd, 7);
    
    /*
    char write_cmd[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95}; // Command to write to the first address of the SD card
    spi.write(write_cmd, 6); // Send the command to the SD card
    char data1[5] = {0x01,0x01,0x01,0x01,0x01}; // Data to be written to the SD card
    spi.write(data1, 5); // Send the data to the SD card
    
    
    */
    char read_cmd[] = {0x51, 0x00, 0x00, 0x00, 0x00, 0xff};
    spi.write(read_cmd, 6);
    
    char data[530];
    spi.read(data, 530);
    /*    
    char read_cmd[] = {0x51, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00};
    spi.write(read_cmd, 6);
    char data1[406];
    spi.read(data1, 406);
    */
    return 0;
    
}