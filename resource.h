// CPSC 3220 resource allocation header

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// ----------------------------------------
// -- resource instance implemented in C --
// ----------------------------------------

typedef struct resource_type_tag{

    // synchronization variables

    pthread_mutex_t lock;
    pthread_cond_t condition;

    // state variables

    int type;             // field to distinguish resource type
    int total_count;      // total number of resources of this type
    int available_count;  // number of currently available resources
    char *status;         // pointer to status vector on heap; encoding:
                          //     0 = available, 1 = in use
                          //   will have two extra entries:
                          //     first: for Knuth's fast linear search
                          //     second: signature to catch overflows
    char *owner;          // pointer to owner vector on heap; encoding:
                          //     0 to 127 are valid owner ids,
                          //     -1 is invalid owner id
                          //   will have an extra entry for signature
    int signature;        // instance signature for run-time checking

    // methods other than init (constructor) and reclaim (destructor)

    int (*allocate)( struct resource_type_tag *self, int tid );
    void (*release)( struct resource_type_tag *self, int tid, int rid );
    void (*print)( struct resource_type_tag *self );

} resource_t;

// constructor-like and destructor-like functions
resource_t * resource_init( int, int );
void resource_reclaim( resource_t * );

// helper functions
int resource_check( resource_t * );
void resource_error( int );

// functions that act like the public methods
int resource_allocate( resource_t *, int );
void resource_release( resource_t *, int, int );
void resource_print( resource_t * );
