#include <iostream>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using std::cout;
using std::endl;

mutex mutex1;
cv cv1;

int child_done = 0;		// global variable; shared between the two threads

void child(void *a)
{
    // with a C-style cast, this would be written as:
    // auto message = (char *) a;
    auto message = static_cast<char *>(a);

    mutex1.lock();
    cout << "child called with message " << message << ", setting child_done = 1" << endl;
    child_done = 1;
    cv1.signal();
    mutex1.unlock();
}

void parent(void *a)
{
    // with a C-style cast, this would be written as:
    // auto arg = (intptr_t) a;
    auto arg = reinterpret_cast<intptr_t>(a);

    mutex1.lock();
    cout << "parent called with arg " << arg << endl;
    mutex1.unlock();

    // with a C-style cast, this would be written as:
    // thread t1 (child, (void *) "test message");
    thread t1 (child, static_cast<void *>(const_cast<char *>("test message")));

    mutex1.lock();
    while (!child_done) {
        cout << "parent waiting for child to run\n";
        cv1.wait(mutex1);
    }
    cout << "parent finishing" << endl;
    mutex1.unlock();
}

int main()
{
    // with a C-style cast, this would be written as:
    // cpu::boot(parent, (void *) 100, 0);
    cpu::boot(parent, reinterpret_cast<void *>(100), 0);
}