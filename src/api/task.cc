// EPOS Task Implementation
#include <framework/main.h>
#include <process.h>

__BEGIN_SYS

Task * volatile Task::_current;

Task::~Task()
{
    db<Task>(TRC) << "~Task(this=" << this << ")" << endl;

    while (!this->_threads.empty()) {
        auto threadObject = this->_threads.remove();
        delete threadObject->object();
    }

    if (this->is_owns_cs()) { delete this->_cs;}
    if (this->is_owns_ds()) { delete this->_ds;}


    delete this->_as;

}

__END_SYS
