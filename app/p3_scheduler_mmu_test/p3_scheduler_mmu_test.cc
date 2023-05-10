#include <utility/ostream.h>
#include <process.h>
#include <memory.h>

using namespace EPOS;

OStream cout;

Thread * testers[5];

int mmu_tester(int id) {
    cout << "Tester: " << id << endl;
    cout << "-------------------------------" << endl;
    size_t * value_page_directory = reinterpret_cast<size_t *>(MMU::current());
    Address_Space self(MMU::current());
    cout << "Page directory is located at: " << value_page_directory << endl;
    Segment * test_segment = new (SYSTEM) Segment((id * 1000), Segment::Flags::SYS);
    CPU::Log_Addr * new_segment = self.attach(test_segment);
    cout << "The test segment is located at: " << value_page_directory << endl;

    cout << "Tester is writing data using the MMU " << endl;
    CPU::Phy_Addr phy_address_to_write = MMU::calloc(1);
    CPU::Log_Addr log_address_to_write = MMU::phy2log(phy_address_to_write);

    int array_to_save[6] = {'t', 'e', 's', 't', 'e', id};
    memcpy(log_address_to_write, array_to_save, 5 * sizeof(int));
    
    cout << "Tester is cleaning segment" << endl;
    memset(new_segment, 0, (id*1000));
    self.detach(test_segment);
    delete test_segment;
    cout << "-------------------------------" << endl;

    return 0;
}

int main()
{
    cout << "-------------------------------" << endl;
    cout << "Tests for P3 -> threads and MMU" << endl << endl;

    cout << "Threads for MMU testers are being created" << endl;

    for (int t = 0; t < 5; t++){
        testers[t] = new Thread(&mmu_tester, t);
    }


    cout << "Threads for MMU testers are joining" << endl;

    for (int t = 0; t < 5; t++) {
        testers[t]->join();
    }

    cout << "Threads for MMU testers are being deleted" << endl;

    for (int t = 0; t < 5; t++){
         delete testers[t];
    }

    cout << "MMU was tested by testers" << endl;
    cout << "-------------------------------" << endl;

    return 0;
}