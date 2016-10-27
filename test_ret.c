/* This test creates four threads, ensures that each thread runs for a
 * different amount of time, and finally returns. */

#include <stdio.h>
#include "sthread.h"

/*! This function loops three times */
static void loop1(void *arg) {
    for (int i = 0; i < 3; i++) {
        printf("First thread\n");
        sthread_yield();
    }
}

/*! This funciton loops 6 times */
static void loop2(void *arg) {
    for (int i = 0; i < 6; i++) {
        printf("Second thread\n");
        sthread_yield();
    }
}

/*! This function loops 9 times */
static void loop3(void *arg) {
    for (int i = 0; i < 9; i++) {
        printf("Third thread\n");
        sthread_yield();
    }
}

/*! This funciton loops 12 times */
static void loop4(void *arg) {
    for (int i = 0; i < 12; i++) {
        printf("Four thread\n");
        sthread_yield();
    }
}

/*
 * The main function starts the four funcitons in separate threads,
 * the start the thread scheduler.
 */
int main(int argc, char **argv) {
    sthread_create(loop1, NULL);
    sthread_create(loop2, NULL);
    sthread_create(loop3, NULL);
    sthread_create(loop4, NULL);
    sthread_start();
    return 0;
}