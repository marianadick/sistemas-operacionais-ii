#include <machine/riscv/riscv_spi.h>
#include <utility/ostream.h>

using namespace EPOS;

int main() {
    OStream cout;

    // Configure SPI
    SPI_E spi;

    spi.put(1);

    int count = spi.get();

    // Print data
    cout << "Data received: " << count << endl;

    return 0;
}
