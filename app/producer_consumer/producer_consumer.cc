// EPOS Synchronizer Component Test Program

#include <machine.h>
#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

const int iterations = 128;

OStream cout;

int producer();
int consumer();
void produce(unsigned long long n);
void consume(unsigned long long n);
unsigned long long busy_wait(unsigned long long n);

const int BUF_SIZE = 16;
char buffer[BUF_SIZE];
Semaphore empty(BUF_SIZE);
Semaphore full(0);

int main()
{
    cout << "Producer x Consumer" << endl;

    Thread * prod = new Thread(&producer);
    Thread * cons = new Thread(&consumer);


    prod->join();
    cons->join();

    cout << "The end!" << endl;

    delete cons;

    return 0;
}

int producer()
{
    int in = 0;

    for(int i = 0; i < iterations; i++) {
        empty.p();
        consume(100000);
        buffer[in] = 'a' + in;
        cout << "P->" << buffer[in] << " ";
        in = (in + 1) % BUF_SIZE;
        full.v();
    }

    return 0;
}

int consumer()
{
    int out = 0;

    for(int i = 0; i < iterations; i++) {
        full.p();
        cout << "C<-" << buffer[out] << " ";
        out = (out + 1) % BUF_SIZE;
        produce(100000);
        empty.v();
    }

    return 0;
}

void produce(unsigned long long n) {
    busy_wait(n);
}

void consume(unsigned long long n) {
    busy_wait(n);
}

unsigned long long busy_wait(unsigned long long n)
{
    volatile unsigned long long v;
    for(unsigned long long int j = 0; j < 20 * n; j++)
        v &= 2 ^ j;
    return v;
}
