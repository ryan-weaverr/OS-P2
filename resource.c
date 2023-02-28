// CPSC 3220 resource allocation code
//
// you need to add synchronization variables and calls to the
//   pthread lock, unlock, wait, and signal functions
//
// search for "ADD" in the comments

#include "resource.h"


// helper functions

int resource_check( resource_t *r ){
    int problem_code = 0;
    int audit_available_count = 0;
    int i;

    // does not need locking operations since this is a helper
    //   function and is called by the functions acting as the
    //   public methods and the destructor

    // returning 0 represents a valid and consistent resource instance
    // returning nonzero indicates a problem

    // signature checks
    if( r->signature != 0x1a2b3c4d )                         problem_code = 1;
    if( r->status[r->total_count + 1] != (char)( 0xab ) )    problem_code = 2;
    if( r->owner[r->total_count] != (char) 0xcd )            problem_code = 3;

    // validity and consistency checks
    for( i = 0; i < r->total_count; i++ ){

        // encoding range checks
        if( ( r->status[i] < 0 ) || ( r->status[i] > 1 ) )   problem_code = 4;
        if( ( r->owner[i] < -1 ) || ( r->owner[i] > 127 ) )  problem_code = 5;

        // status and ownership consistency checks
        if( ( r->status[i] == 0 ) && ( r->owner[i] != -1 ) ) problem_code = 6;
        if( ( r->status[i] == 1 ) && ( r->owner[i] == -1 ) ) problem_code = 7;

        // sum available units for audit
        if( r->status[i] == 0 ) audit_available_count++;
    }

    // consistency check on count
    if( audit_available_count != r->available_count )        problem_code = 8;

    // print diagnostic info
    if( problem_code ){
        printf( "problem code is %d\n", problem_code );
        printf( "signatures are %x %2x %2x\n",
            r->signature, r->status[r->total_count + 1],
            r->owner[r->total_count] );
        printf( "audit count = %d, available count = %d\n",
            audit_available_count, r->available_count );
    }

    return problem_code;
}

void resource_error( int code ){

    // does not need locking operations since this is a helper
    //   function and is called by the functions acting as the
    //   constructor, public methods, and the destructor

    switch( code ){
        case 0:  printf( "**** malloc error for resource instance\n" );
                 break;
        case 1:  printf( "**** malloc error for status vector\n" );
                 break;
        case 2:  printf( "**** malloc error for owner vector\n" );
                 break;
        case 3:  printf( "**** could not initialize lock for resource\n" );
                 break;
        case 4:  printf( "**** could not initialize condition for resource\n" );
                 break;
        case 5:  printf( "**** reclaim argument is not a valid resource\n" );
                 break;
        case 6:  printf( "**** print argument is not a valid resource\n" );
                 break;
        case 7:  printf( "**** allocate signature check failed\n" );
                 break;
        case 8:  printf( "**** search for resource failed\n" );
                 break;
        case 9:  printf( "**** release signature check failed\n" );
                 break;
        case 10: printf( "**** release rid bounds check failed\n" );
                 break;
        case 11: printf( "**** release ownership check failed\n" );
                 break;
        default: printf( "**** unknown error code\n" );
    }
    exit( 0 );
}


// init function serves the role of a constructor

resource_t * resource_init( int type, int total ){
    resource_t *r;
    int i, rc;

    // does not need locking operations since the lock is created
    //   in this function

    r = malloc( sizeof( resource_t ) );   // obtain memory for struct
    if( r == NULL ) resource_error( 0 );

    r->type = type;                       // set data fields
    r->total_count = total;
    r->available_count = total;

    r->status = malloc( total + 2 );      // obtain memory for vector
    if( r->status == NULL )               //     first extra entry allows
        resource_error( 1 );              //     faster searching
    for( i = 0; i <= total; i++ )         // 0 = available, 1 = in use
        r->status[i] = 0;
    r->status[total+1] = 0xab;            // simple signature at end of
                                          //   extended status vector

    r->owner = malloc( total + 1 );       // obtain memory for vector
    if( r->owner == NULL )
        resource_error( 2 );
    for( i = 0; i < total; i++ )          // -1 = invalid owner id
        r->owner[i] = -1;
    r->owner[total] = 0xcd;               // simple signature at end of
                                          //   extended owner vector

    r->signature = 0x1a2b3c4d;            // word-length signature for
                                          //   resource structure

    rc = pthread_mutex_init( &r->lock, NULL );
    if( rc != 0 ) resource_error( 3 );

    rc = pthread_cond_init( &r->condition, NULL );
    if( rc != 0 ) resource_error( 4 );

    r->print = &resource_print;           // set method pointers
    r->allocate = &resource_allocate;
    r->release = &resource_release;

    return r;
}


// reclaim function serves the role of a destructor

void resource_reclaim( resource_t *r ){

    // does not need locking operations since the lock is destroyed
    //   in this function

    if( resource_check( r ) ) resource_error( 5 );

    pthread_cond_destroy( &r->condition );
    pthread_mutex_destroy( &r->lock );

    free( r->owner );
    free( r->status );
    free( r );
}


// allocate, release, and print would be public methods in an object

int resource_allocate( resource_t *self, int tid ){

    int rid;

    // you will need to add synchronization operations to this
    //   function as appropriate; follow the patterns in the textbook

    // ADD acquire the lock at the beginning of the method
    pthread_mutex_lock(&self->lock);


    if( resource_check( self ) )          // signature check
        resource_error( 7 );

    // assertion before proceeding: self->available_count != 0
    //assert(self->available_count != 0);
    // ADD loop to test assertion and otherwise wait on the
    //   condition variable
    while(self->available_count != 0)
    {
        pthread_cond_wait(&self->condition, &self->lock);
    }

    rid = 0;                              // initialize search index
    self->status[self->total_count] = 0;  // extra entry is always available
    while( self->status[rid] != 0 )       // search until available found
        rid++;
    if( rid >= self->total_count )        // bounds check of result
        resource_error( 8 );

    self->status[rid] = 1;                // mark this entry as in use
    self->owner[rid] = tid;               // record which thread has it
    self->available_count--;              // decr count of available resources

    // ADD release the lock at then end of the method
    pthread_mutex_unlock(&self->lock);

    return rid;
}

void resource_release( resource_t *self, int tid, int rid ){

    // you need to add synchronization operations to this function
    //   as appropriate; follow the patterns in the textbook

    // ADD acquire the lock at the beginnning of the method
    pthread_mutex_lock(&self->lock);

    if( resource_check( self ) )          // signature check
        resource_error( 9 );
    if( rid < 0 )                         // bounds check of argument
        resource_error( 10 );
    if( rid >= self->total_count )        // bounds check of argument
        resource_error( 10 );
    if( self->owner[rid] != tid )         // check ownership match
        resource_error( 11 );

    self->status[rid] = 0;                // mark this entry as available
    self->owner[rid] = -1;                // reset ownership
    self->available_count++;              // incr count of available resources

    // ADD signal the condition variable
    pthread_cond_signal(&self->condition);

    // ADD release the lock at the end of the method
    pthread_mutex_unlock(&self->lock);
}

void resource_print(resource_t *self){
    int i;

    // obtain lock at the start of the method
    pthread_mutex_lock(&self->lock);

    if( resource_check( self ) )          // signature check
        resource_error( 6 );

    printf( "-- resource table for type %d --\n", self->type );
    for( i = 0; i < self->total_count; i++ ){
        printf( " resource #%d: %d,%d\n", i, self->status[i], self->owner[i] );
    }
    printf( "-------------------------------\n" );

    //release the lock at the end of the method
    pthread_mutex_unlock(&self->lock);
}
