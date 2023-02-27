// CPSC 3220 resource allocation using pthread mutex and
//   condition variable
//
// a simple test case driver
//
// compile with "gcc -Wall resource.c driver.c -pthread"
// run with "./a.out" or "valgrind --tool=helgrind ./a.out"
//
// THREADS is the number of worker threads to create
// for this driver, there will be an additional observer thread
//
// RESOURCES is the number of resources managed by the single
//   resource instance

#include "resource.h"

#define THREADS 20
#define RESOURCES 4


// argument vector for threads to combine multiple parameters
//   since only one thread function parameter is allowed for
//   pthread_create()

typedef struct{
    resource_t *rp;
    int id;
} argvec_t;


// worker thread - obtain resource, sleep, release resource

void *worker( void *ap ){
    resource_t *resource = ((argvec_t *)ap)->rp;
    int thread_id = ((argvec_t *)ap)->id;
    int resource_id;

    resource_id = (resource->allocate)( resource, thread_id );

    printf( "thread #%d uses resource #%d\n", thread_id, resource_id );
    sleep( 1 );

    (resource->release)( resource, thread_id, resource_id );

    return NULL;
}


// observer thread - for resource instance, periodically print
//   the status info

void *observer( void *ap ){
    resource_t *resource = ((argvec_t *)ap)->rp;
    int i;

    for( i = 0; i < 2; i++ ){
        sleep( 2 );
        (resource->print)( resource );
    }

    return NULL;
}


// --------------------------------------------
// -- test driver with one resource instance --
// --------------------------------------------

// command line arguments are not used for this driver

int main( int argc, char **argv ){
    pthread_t threads[ THREADS + 1 ];  // thread control blocks for pthreads
    argvec_t args[ THREADS + 1 ];      // unique argument vectors
    resource_t *resource_1;            // resource instance
    int i;

    // construct a resource instance with identifier 1
    resource_1 = resource_init( 1, RESOURCES );

    // initial print of status info
    (resource_1->print)( resource_1 );

    // create threads with a pointer to the resource instance and unique id
    for( i = 0; i < THREADS; i++ ){
        args[i].rp = resource_1;
        args[i].id = i;
        if( pthread_create( &threads[i], NULL, &worker, (void *)(&args[i]) ) ){
            printf( "**** could not create worker thread %d\n", i );
            exit( 0 );
        }
    }

    // create a single observer thread
    i = THREADS;
    args[i].rp = resource_1;
    args[i].id = i;
    if( pthread_create( &threads[i], NULL, &observer, (void *)(&args[i]) ) ){
        printf( "**** could not create observer thread\n" );
        exit( 0 );
    }

    // join all worker threads and observer thread
    for( i = 0; i < (THREADS+1); i++ ){
        if( pthread_join( threads[i], NULL ) ){
            printf( "**** could not join thread %d\n", i );
            exit( 0 );
        }
    }

    // final print of status info
    (resource_1->print)( resource_1 );

    // call the destructor-like function for the resource instance
    resource_reclaim( resource_1 );

    return 0;
}
