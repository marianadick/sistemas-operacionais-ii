// EPOS Scheduler Test Program

#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

OStream cout;



int my_address(void);

int main()
{

    cout << "I am the main task" << endl;
    Task * task0 = Task::self();
    Address_Space * as0 = task0->address_space();
    cout << "Main task page directory is located at " << as0->pd() << endl;

    cout << "Forking main task...." << endl;
    Task * forked_task = new Task(task0, &my_address, 0);


    forked_task->join();

    return 0;
}

int my_address(void)
{
    cout << "----------------------------" << endl;
    cout << "I am the forked task" << endl;
    //Task * task0 = Task::self();
    //Address_Space * as0 = task0->address_space();
    //cout << "Forked task got page directory from main and is located at " << as0->pd() << endl;

    cout << "----------------------------" << endl;
    return 0;
}
