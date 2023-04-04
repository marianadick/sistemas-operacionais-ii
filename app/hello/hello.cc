#include <utility/heap.h>
#include <utility/ostream.h>

using namespace EPOS;

OStream cout;

int main()
{
    // Especifica tamanho da heap manualmente
    const unsigned int HEAP_SIZE = 4096;

    // Aloca espaço para a heap na memória
    char heap_buffer[HEAP_SIZE];

    // Inicializa a heap
    Heap heap(reinterpret_cast<void*>(heap_buffer), HEAP_SIZE);

    cout << "Tamanho do bloco livre inicial: " << heap.grouped_size() << " bytes" << endl;

    // Teste de alocação e liberação de memória
    char * block1 = reinterpret_cast<char *>(heap.alloc(1024));
    cout << "Tamanho do bloco livre após a alocação do primeiro bloco de 1024 bytes: " << heap.grouped_size() << " bytes" << endl;

    char * block2 = reinterpret_cast<char *>(heap.alloc(512));
    cout << "Tamanho do bloco livre após a alocação do segundo bloco de 512 bytes: " << heap.grouped_size() << " bytes" << endl;

    char * block3 = reinterpret_cast<char *>(heap.alloc(256));
    cout << "Tamanho do bloco livre após a alocação do terceiro bloco de 256 bytes: " << heap.grouped_size() << " bytes" << endl;

    if (block2 == block3) {
        cout << "Erro: o bloco alocado pelo first fit é o mesmo que foi alocado anteriormente pelo worst fit" << endl;
    } else {
        cout << "Sucesso: o bloco alocado pelo first fit é diferente do bloco alocado anteriormente pelo worst fit" << endl;
    }

    heap.free(reinterpret_cast<void *>(block1), 1024);
    heap.free(reinterpret_cast<void *>(block2), 512);
    heap.free(reinterpret_cast<void *>(block3), 256);

    cout << "Tamanho do bloco livre após a liberação de 3 blocos: " << heap.grouped_size() << " bytes" << endl;

    return 0;
}
