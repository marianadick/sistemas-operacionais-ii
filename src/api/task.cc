// EPOS Task Implementation
#include <framework/main.h>
#include <process.h>

__BEGIN_SYS

Task * volatile Task::_current;

Task::~Task()
{
    db<Task>(TRC) << "~Task(this=" << this << ")" << endl;

    // Clears Task' thread queue
    while (!_threads.empty()) {
        delete _threads.remove()->object();
    }

    if (!shares_seg()) {
        delete _cs;
        delete _ds;
    }

    delete _as;

}

__END_SYS
