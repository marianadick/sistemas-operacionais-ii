#include <machine/riscv/riscv_spi.h>
#include <utility/ostream.h>
#include <utility/ostream.h>

using namespace EPOS;
OStream cout;

int main() {
    

    // Configure SPI
    SPI_E spi;
    
    char init_cmd[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95, 0xff, 0x00}; //send command data to the MMC chip
    spi.write(init_cmd, 6);
    
    char data[15] = {0}; // create an data array to read the response data
    spi.read(data, 15); // read the response data from the device

    for(int i=0; i<15; i++) {
        if (data[i] != 0){ //verify if read got the data
            cout << "Receive data in position " << i << " is " << (int) data[i] << endl;
            /*
                Return data from this test, in our computer is:
                Some links indicates is a common initialize error, and as we dont know the manual of the mmc
                https://stackoverflow.com/questions/5048450/c-initializing-an-sd-card-in-spi-mode-always-reads-back-0xff

                Receive data in position 0 is 255
                Receive data in position 1 is 255
                Receive data in position 2 is 255
                Receive data in position 3 is 255
                Receive data in position 4 is 255
                Receive data in position 5 is 255
                Receive data in position 6 is 255
                Receive data in position 7 is 4
                The last thread has exited!
                Rebooting the machine ...
                Machine::reboot()
            
            
            */
        }
    }
    


    return 0;
    
}
