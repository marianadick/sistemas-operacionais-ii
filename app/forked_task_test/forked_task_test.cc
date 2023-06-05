// EPOS Scheduler Test Program

#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

OStream cout;

int hi_message(int argc, char * argv[]);

int main()
{
    char o[] = "O"; char i[] = "i";
    int oi_size = 2;
    char ** arg_oi = new char*[oi_size];
    arg_oi[0] = o; arg_oi[1] = i;


    int id_first_task = 1;
    Task *first_task = new Task(Task::self(), &hi_message, id_first_task, arg_oi);
    Task *second_task = new Task(first_task, &hi_message, (id_first_task+1), arg_oi);
    Task *third_task = new Task(second_task, &hi_message, (id_first_task+2), arg_oi);

    hi_message(0, arg_oi);

    first_task->join();
    second_task->join();
    third_task->join();

    delete first_task;
    delete second_task;
    delete third_task;

    return 0;
}

int hi_message(int argc, char * argv[])
{
    cout <<"Task com id "<< argc << " diz: " << endl;
    cout << argv[0] << argv[1] << endl;
    
    return 1;
}
