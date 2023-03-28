#include <machine/riscv/riscv_spi.h>
#include <utility/ostream.h>

using namespace EPOS;

int main() {
    

    // Configure SPI
    SPI_E spi(0x30000000);
    spi.config(1000000, 0, 0, 8, 8);

    // Write data
    //const char * data_out = "Hello, world!";
    char a = 'a';
    spi.put(a);
    spi.put(a);
    spi.put(a);

    spi.put(a);
   
    // Read data
    //char data_in[256];
    //int count = spi.read(data_in, sizeof(data_in));
    //data_in[count] = '\0';
    char data_in = 'b';
    data_in = spi.get();
    data_in = spi.get();
    data_in = spi.get();
    data_in = spi.get();
    
    data_in = spi.get();
    
    data_in = spi.get();
    
        /* code */

    cout << "Data received: " << (int)data_in << endl;
    
    
    
    // Print data

    return 0;
}
