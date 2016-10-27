# Thread-Scheduler
Implementation a thread scheduler in C.

Implemented a thread scheduler responsible for creating and deleting threads.
Also performed different operations depending on whether a current thread was
in a blocked state, running state, or finished state.

*test_arg.c* is a test file attempting to pass an argument to the thread-functions,
to ensure that the argument is positioned properly in the thread's machine context so
that functions can retrieve it and use it.

*test_ret.c* is a test file attempting to create 4 threads, ensuring that each thread runs
for a different length of time, and then terminate the threads by returning from the
thread-function.
