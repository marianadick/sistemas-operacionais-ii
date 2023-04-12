#include <utility/heap.h>
#include <utility/ostream.h>

using namespace EPOS;

OStream cout;



int main()
{
    cout << "Starting E6b worst fit allocation test" << endl;

    const unsigned int HEAP_SIZE = 4096*2;
    char heap_buffer[HEAP_SIZE];

   
    Heap heap(heap_buffer, HEAP_SIZE);

    char **array = new char*[3];
    cout << "Initial address of the array: " << array << endl;
    cout << "After initializing array, heap size = " << heap.grouped_size() << " bytes" << endl;
     
    array[0] = reinterpret_cast<char *>(heap.alloc(1024));
    cout << "Allocating 1024 bytes - array[0] = " << static_cast<void*>(array[0]) << endl;
    array[1] = reinterpret_cast<char *>(heap.alloc(2048));
    cout << "Allocating 2048 bytes - array[1] = " << static_cast<void*>(array[1]) << endl;
    array[2] = reinterpret_cast<char *>(heap.alloc(3072));
    cout << "Allocating 3072 bytes - array[2] = " << static_cast<void*>(array[2]) << endl;
    
    heap.free(reinterpret_cast<void *>(array[1]), 2048);
    cout << "After freeing array[1], heap size = " << heap.grouped_size() << " bytes" << endl;
     
    array[1] = reinterpret_cast<char *>(heap.alloc(512));
    cout << "Allocating 512 bytes - array[1] = " << static_cast<void*>(array[1]) << endl;
    cout << "Heap size: " << heap.grouped_size() << " bytes" << endl;

    heap.free(reinterpret_cast<void *>(array[0]), 1024);
    heap.free(reinterpret_cast<void *>(array[1]), 512);
    heap.free(reinterpret_cast<void *>(array[2]), 3072);
    
    cout << "After freeing array[0] to array[2], heap size = " << heap.grouped_size() << " bytes" << endl;
    return 0;
}

