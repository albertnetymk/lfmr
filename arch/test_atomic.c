#include "atomic.h"

atomic_t foo = 0;

int
main(int argc, char *argv[])
{
    atomic_t result = 0xff;

    result = atomic_xadd4(&foo, 1);
    result = atomic_cmpxchg4(&foo, 1, 3);
    result = atomic_cmpxchg4(&foo, 1, 10);
    result = atomic_xchg4(&foo, 123);
    result = atomic_read4(&foo);
}
