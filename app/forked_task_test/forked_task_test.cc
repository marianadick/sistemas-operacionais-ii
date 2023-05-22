// EPOS Scheduler Test Program

#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

OStream cout;

Task * task;

int func_a(void);

int main()
{
    task = new Task(task, &func_a);

    task->join();

    cout << endl;

    return 0;
}

int func_a(void)
{
    for(int i = 100; i > 0; i--) {
        for(int i = 0; i < 79; i++)
            cout << "a";
        cout << endl;
    }

    return 'A';
}
