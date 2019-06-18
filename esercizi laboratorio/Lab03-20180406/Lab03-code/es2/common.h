#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// macros for handling errors
#define handle_error_en(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

// macros for producer.c and consumer.c
#define BUFFER_SIZE         128
#define BUFFER_FILENAME     "bufferfile.bin"
#define INITIAL_DEPOSIT     0
#define MAX_TRANSACTION     1000
#define NUM_CONSUMERS       2
#define NUM_PRODUCERS       1
#define PRNG_SEED           0

#define NUM_OPERATIONS      400
#define OPS_PER_CONSUMER    (NUM_OPERATIONS/NUM_CONSUMERS)
#define OPS_PER_PRODUCER    (NUM_OPERATIONS/NUM_PRODUCERS)

#if OPS_PER_CONSUMER*NUM_CONSUMERS != OPS_PER_PRODUCER*NUM_PRODUCERS
#error "Choose NUM_CONSUMERS and NUM_PRODUCERS so that we get exactly NUM_OPERATIONS operations"
#endif

#define SEMNAME_FILLED      "/mysemfilled"
#define SEMNAME_EMPTY       "/mysemempty"
#define SEMNAME_CS          "/mysemcs"


// methods defined in common.c
void initFile(int numElems, char* fileName);
void writeToBufferFile(int value, int numElems, char* fileName);
int readFromBufferFile(int numElems, char* fileName);