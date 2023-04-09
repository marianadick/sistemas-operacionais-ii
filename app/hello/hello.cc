#include <utility/heap.h>
#include <utility/ostream.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "Starting E6b worst fit alocation test" << endl;

    // Especifica tamanho da heap manualmente
    const unsigned int HEAP_SIZE = 4096;

    // Aloca espaço para a heap na memória
    char heap_buffer[HEAP_SIZE];

    // Inicializa a heap
    Heap heap(reinterpret_cast<void*>(heap_buffer), HEAP_SIZE);

    char **array = new char*[4];
    cout << "Endereço inicial do array: " << array << endl;

    array[0] = reinterpret_cast<char *>(heap.alloc(512));
    cout << "Alocando 512 bytes - array[0] = " << static_cast<void*>(array[0]) << endl; // imprime o endereço de memória
    array[1] = reinterpret_cast<char *>(heap.alloc(512));
    cout << "Alocando 512 bytes - array[1] = " << static_cast<void*>(array[1]) << endl; // imprime o endereço de memória
    array[2] = reinterpret_cast<char *>(heap.alloc(2048));
    cout << "Alocando 2048 bytes - array[2] = " << static_cast<void*>(array[2]) << endl; // imprime o endereço de memória
    array[3] = reinterpret_cast<char *>(heap.alloc(512));
    cout << "Alocando 512 bytes - array[3] = " << static_cast<void*>(array[3]) << endl; // imprime o endereço de memória

    heap.free(reinterpret_cast<void *>(array[2]), 2048);
    cout << "After freeing array[2], heap size = " << heap.grouped_size() << " bytes" << endl;

    array[2] = reinterpret_cast<char *>(heap.alloc(512));
    cout << "Alocando 512 bytes - array[2] = " << static_cast<void*>(array[2]) << endl; // imprime o endereço de memória
    cout << "Heap size: " << heap.grouped_size() << " bytes" << endl;

    heap.free(reinterpret_cast<void *>(array[0]), 512);
    heap.free(reinterpret_cast<void *>(array[1]), 512);
    heap.free(reinterpret_cast<void *>(array[2]), 512);
    heap.free(reinterpret_cast<void *>(array[3]), 512);

    cout << "After freeing array[0] to and array[3], heap size = " << heap.grouped_size() << " bytes" << endl;
    return 0;
}
