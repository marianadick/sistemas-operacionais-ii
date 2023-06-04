// EPOS Scheduler Test Program

#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

OStream cout;

int fun(int argc, char * argv[]);

int main()
{
    char one[] = "1";
    char two[] = "2";
    char three[] = "3";

    int argc1 = 1;
    char ** argv1 = new char*[argc1];
    argv1[0] = one;

    int argc2 = 2;
    char ** argv2 = new char*[argc2];
    argv2[0] = one;
    argv2[1] = two;

    int argc3 = 3;
    char ** argv3 = new char*[argc3];
    argv3[0] = one;
    argv3[1] = two;
    argv3[2] = three;

    Task *task1 = new Task(Task::self(), &fun, argc2, argv2);
    Task *task2 = new Task(task1, &fun, argc3, argv3);

    fun(argc1, argv1);

    task1->join();
    task2->join();

    delete task1;
    delete task2;

    return 0;
}

int fun(int argc, char * argv[])
{
    for (int i = 0; i < argc; i++)
    {
        cout << argv[i] << " ";
    }
    cout << endl;

    return 1;
}
