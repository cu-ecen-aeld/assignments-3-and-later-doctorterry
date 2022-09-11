#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)


// Note that https://w3.cs.jmu.edu/kirkpams/OpenCSF/Books/csf/html/ThreadArgs.html has a good explanation
// for this section.
void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // Cast args into a meaningful pointer type that we can use
    struct thread_data *args = (struct thread_data *) thread_param;

    // Delay before attempting to get the mutex
    //sleep(args->waitMxGet); // This will fail because it is blocking
    usleep(args->waitMxGet*1000);

    printf("Get mutex\n");
    int mutex_get = pthread_mutex_lock(args->mutex);
    // Test if mutex lock was acquired
    if (mutex_get != 0){
        printf("Mutex lock failed\n");
        args->thread_complete_success = false;
    }
    else{
        printf("Mutex lock success\n");
        //sleep(args->waitMxRls); // This will fail because it is blocking
        usleep(args->waitMxRls*1000);
        int mutex_rls = pthread_mutex_unlock(args->mutex);
        // Test if unlock successful
        if (mutex_rls != 0){
            printf("Mutex unlock failed\n");
            args->thread_complete_success = false;
        }
        else{
            printf("Mutex unlocked\n");
            args->thread_complete_success = true;
        }
    }

    // Return the struct back to the calling function with the updated data
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    int ret;

    // Allocate memory for the struct
    struct thread_data* threadInit = (struct thread_data *)malloc(sizeof(struct thread_data));
    if (threadInit!= NULL){
        printf("Memory allocated for the struct data.\n");
    }
    else{
        printf("Failed to allocate memory\n");
        return false;
    }

    threadInit->mutex = mutex;
    threadInit->waitMxGet = wait_to_obtain_ms;
    threadInit->waitMxRls = wait_to_release_ms;

    // Start thread
    if ((ret = pthread_create(thread, NULL, threadfunc, threadInit)) == 0){
        printf("Thread started\n");
        return true;
    }
    else{
        printf("Failed to start thread\n");
        return false;
    }
}

