/*
 * The simple thread scheduler.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "sthread.h"

/*
 * By default, create threads with 1MB of stack space.
 */
#define DEFAULT_STACKSIZE       (1 << 20)

/************************************************************************
 * Interface to the assembly.
 */

/*
 * The structure of the thread's machine-context is opaque to the C code.
 * Thread structs reference the context by a ThreadContext *, i.e. a void *
 * (untyped pointer).
 */
typedef void ThreadContext;

/*
 * This function must be called to start thread
 * processing.
 */
void __sthread_start(void);

/*
 * The is the entry point into the scheduler, to be called
 * whenever a thread action is required.
 */
void __sthread_schedule(void);

/*
 * Initialize the context for a new thread.
 *
 * The stackp pointer should point to the *end* of the area
 * allocated for the thread's stack.  (Don't forget that IA32
 * stacks grow downward in memory.)
 *
 * The function f is initially called with the argument given.
 * When f returns, __sthread_finish() is automatically called by
 * the threading library to ensure that the thread is cleaned up and
 * deallocated properly.
 */
ThreadContext *__sthread_initialize_context(void *stackp, ThreadFunction f,
                                            void *arg);

/************************************************************************
 * Internal helper functions.
 */

/*
 * The initial thread context is initialized such that this function
 * will be called automatically when a thread's function returns.
 */
void __sthread_finish(void);

/*
 * This function deallocates the memory associated with a thread
 * when it terminates.
 */
void __sthread_delete(Thread *threadp);

/************************************************************************
 * Types and global variables.
 */

/*
 * A thread can be running, ready, blocked, or finished.
 */
typedef enum {
    /*!
     * The thread is currently running on the CPU.  Only one thread should be
     * in this state.
     */
    ThreadRunning,
    
    /*!
     * The thread is ready to run, but doesn't have access to the CPU yet.  The
     * thread will be kept in the ready-queue.
     */
    ThreadReady,
    
    /*!
     * The thread is blocked and currently unable to progress, so it is not
     * scheduled on the CPU.  Blocked threads are kept in the blocked-queue.
     */
    ThreadBlocked,
    
    /*!
     * The thread's function has returned, and therefore the thread is ready to
     * be permanently removed from the scheduling mechanism and deallocated.
     */
    ThreadFinished
} ThreadState;

/*
 * Descriptive information for a thread.
 */
struct _thread {
    /* State of the process */
    ThreadState state;

    /*
     * The start of the memory region being used for the thread's stack
     * and machine context.
     */
    void *memory;

    /*
     * The machine context itself.  This will be some address within the
     * memory region referenced by the previous field.
     */
    ThreadContext *context;

    /* The processes are linked in a doubly-linked list. */
    struct _thread *prev;
    struct _thread *next;
};

/*
 * The queue has a pointer to the head and the last element.
 */
typedef struct _queue {
    Thread *head;
    Thread *tail;
} Queue;

/*
 * The thread that is currently running.
 *
 * Invariant: during normal operation, there is exactly one thread in
 * the ThreadRunning state, and this variable points to that thread.
 */
static Thread *current;

/************************************************************************
 * Queue operations.
 */

/*
 * Queues for ready and blocked threads.
 *
 * Invariants:
 *     All threads in the ready queue are in state ThreadReady.
 *     All threads in the blocked queue are in state ThreadBlocked.
 */
static Queue ready_queue;
static Queue blocked_queue;

/*
 * Returns true (1) if the specified queue is empty.  Otherwise, returns
 * false (0).
 */
static int queue_empty(Queue *queuep) {
    assert(queuep != NULL);
    return (queuep->head == NULL);
}

/*
 * Add the process to the head of the queue.
 * If the queue is empty, add the singleton element.
 * Otherwise, add the element as the tail.
 */
static void queue_append(Queue *queuep, Thread *threadp)
{
    if(queuep->head == NULL) {
        threadp->prev = NULL;
        threadp->next = NULL;
        queuep->head = threadp;
        queuep->tail = threadp;
    }
    else {
        queuep->tail->next = threadp;
        threadp->prev = queuep->tail;
        threadp->next = NULL;
        queuep->tail = threadp;
    }
}

/*
 * Add a thread to the queue based on its state.
 */
static void queue_add(Thread *threadp) {
    assert(threadp != NULL);

    switch(threadp->state) {
    case ThreadReady:
        queue_append(&ready_queue, threadp);
        break;
    case ThreadBlocked:
        queue_append(&blocked_queue, threadp);
        break;
    default:
        fprintf(stderr, "Thread state has been corrupted: %d\n", threadp->state);
        exit(1);
    }
}

/*
 * Get the first thread from the queue.  Returns NULL if the queue is empty.
 */
static Thread *queue_take(Queue *queuep) {
    Thread *threadp;

    assert(queuep != NULL);
    
    /* Return NULL if the queue is empty */
    if(queuep->head == NULL)
        return NULL;

    /* Go to the final element */
    threadp = queuep->head;
    if(threadp == queuep->tail) {
        queuep->head = NULL;
        queuep->tail = NULL;
    }
    else {
        threadp->next->prev = NULL;
        queuep->head = threadp->next;
    }
    return threadp;
}

/*
 * Remove a thread from a queue.
 */
static void queue_remove(Queue *queuep, Thread *threadp) {
    assert(queuep != NULL);
    assert(threadp != NULL);

    /* Unlink */
    if(threadp->prev != NULL)
        threadp->prev->next = threadp->next;
    if(threadp->next != NULL)
        threadp->next->prev = threadp->prev;

    /* Reset head and tail pointers */
    if(queuep->head == threadp)
        queuep->head = threadp->next;
    if(queuep->tail == threadp)
        queuep->tail = threadp->prev;
}

/************************************************************************
 * Scheduler.
 */

/*
 * The scheduler is called with the context of the current thread,
 * or NULL when the scheduler is first started.
 *
 * The general operation of this function is:
 *   1.  Save the context argument into the current thread.
 *   2.  Either queue up or deallocate the current thread,
 *       based on its state.
 *   3.  Select a new "ready" thread to run, and set the "current"
 *       variable to that thread.
 *        - If no "ready" thread is available, examine the system
 *          state to handle this situation properly.
 *   4.  Return the context of the thread to run, so that a context-
 *       switch will be performed to start running the next thread.
 *
 * This function is global because it needs to be called from the assembly.
 */
ThreadContext *__sthread_scheduler(ThreadContext *context) {

    /* Replace these lines with your implementation */
    if (context == NULL) {
        if (queue_empty(&ready_queue) == 1) {
            if (queue_empty(&blocked_queue) == 1) {
                printf("There are no ready threads or blocked threads. " 
                    "All threads have completed successfully. \n");
                exit(0); /* If no threads are ready or blocked */
            }
            else{
                printf("Program has deadlocked. Exiting \n");
                exit(1); /* If no threads are ready, but some are blocked */
            }
        }
        else
            current = queue_take(&ready_queue);
            current->state = ThreadRunning; /* If some threads are ready */
    }
    else {
        current->context = context; /* If context isn't NULL, proceed below */
        if (current->state == ThreadFinished) {
            __sthread_delete(current); /* If thread is finished, delete it */
        }
        else if (current->state == ThreadBlocked) {  
            queue_add(current); /* Add thread to blocked_queue */
        }
        else if (current->state == ThreadRunning) {
            current->state = ThreadReady;
            queue_add(current); /* Add thread to ready_queue */
        }

        if (queue_empty(&ready_queue) == 1) {
            if (queue_empty(&blocked_queue) == 1) {
                printf("There are no ready threads or blocked threads. "
                    "All threads have completed successfully. \n");
                exit(0); /* If no threads are ready or blocked */
            }
            else {
                printf("Program has deadlocked. Exiting \n");
                exit(1); /* If no threads are ready, but some are blocked */
            }
        }
        else { /* If there are threads that are ready */
            /* Set the current thread to the first ready thread */
            current = queue_take(&ready_queue);
            current->state = ThreadRunning; /* Thread is now running */
        }
    }
    /* Return the next thread to resume executing. */
    return current->context;
}


/************************************************************************
 * Thread operations.
 */

/*
 * Start the scheduler.
 */
void sthread_start(void)
{
    __sthread_start();
}

/*
 * Create a new thread.
 *
 * This function allocates a new context, and a new Thread
 * structure, and it adds the thread to the Ready queue.
 */
Thread * sthread_create(void (*f)(void *arg), void *arg) {
    /* Replace this function's body with your implementation */
    struct _thread *newThread;
    void *mem = malloc(DEFAULT_STACKSIZE); /* Make space for 1MB thread */
    newThread = malloc(sizeof(Thread)); /* Make new thread */
    newThread->state = ThreadReady; /* Change state of thread to ready */
    newThread->memory = mem;
    newThread->context = 
    __sthread_initialize_context((unsigned char *)mem + DEFAULT_STACKSIZE, f, arg);
    queue_add(newThread); /* Add thread to ready_queue */
    return newThread;
}


/*
 * This function is called automatically when a thread's function returns,
 * so that the thread can be marked as "finished" and can then be reclaimed.
 * The scheduler will call __sthread_delete() on the thread before scheduling
 * a new thread for execution.
 *
 * This function is global because it needs to be referenced from assembly.
 */
void __sthread_finish(void) {
    printf("Thread 0x%08x has finished executing.\n", (unsigned int) current);
    current->state = ThreadFinished;
    __sthread_schedule();
}


/*
 * This function is used by the scheduler to release the memory used by the
 * specified thread.  The function deletes the memory used for the thread's
 * context, as well as the memory for the Thread struct.
 */
void __sthread_delete(Thread *threadp) {
    free(threadp->memory); /* Free memory of thread */
    free(threadp); /* Free space taken up by thread */
}


/*
 * Yield, so that another thread can run.  This is easy: just
 * call the scheduler, and it will pick a new thread to run.
 */
void sthread_yield() {
    __sthread_schedule();
}


/*
 * Block the current thread.  Set the state of the current thread
 * to Blocked, and call the scheduler.
 */
void sthread_block() {
    current->state = ThreadBlocked;
    __sthread_schedule();
}

/*
 * Unblock a thread that is blocked.  The thread is placed on
 * the ready queue.
 */
void sthread_unblock(Thread *threadp) {

    /* Make sure the thread was blocked */
    assert(threadp->state == ThreadBlocked);

    /* Remove from the blocked queue */
    queue_remove(&blocked_queue, threadp);

    /* Re-queue it */
    threadp->state = ThreadReady;
    queue_add(threadp);
}
