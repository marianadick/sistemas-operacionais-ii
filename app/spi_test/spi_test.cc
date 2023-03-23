#include <machine/riscv/riscv_spi.h>
#include <utility/ostream.h>

using namespace EPOS;

int main() {
    OStream cout;

    // Configure SPI
    SPI_E spi(0x10000000);
    spi.config(50000000, 0, 0, 1000000, 8);

    // Write data
    const char * data_out = "Hello, world!";
    spi.write(data_out, strlen(data_out));

    // Read data
    char data_in[256];
    int count = spi.read(data_in, sizeof(data_in));
    data_in[count] = '\0';

    // Print data
    cout << "Data received: " << data_in << endl;

    return 0;
}
