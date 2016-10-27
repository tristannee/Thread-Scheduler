/* This is the second test for the threading APIs. It attempts to  pass
 * an argument to the thread-functions, to ensure that the argument is
 * positioned properly in the thread's machine context such that the functions
 * can retrieve and use it. */

#include <stdio.h>
#include "sthread.h"

/*! This thread-function prints "Hello" over and over again! */
static void increment(void *arg) {
    for (int i = 0; i < 10; i++) {
    	*((int *)arg) += 1;
    	printf("%d (Incremented by 1)\n", *((int *)arg));
        sthread_yield();
    }
}

/*! This thread-function prints "Goodbye" over and over again! */
static void decrement_2(void *arg) {
    for (int i = 0; i < 10; i++) {
    	*((int *)arg) -= 2;
    	printf("%d (Decremented by 2)\n", *((int *)arg));
        sthread_yield();
    }
}

/*
 * The main function starts the two functions in separate threads,
 * the start the thread scheduler.
 */
int main(int argc, char **argv) {
	int ten = 10;
    sthread_create(increment, (void *)&ten);
    sthread_create(decrement_2, (void *)&ten);
    sthread_start();
    return 0;
}