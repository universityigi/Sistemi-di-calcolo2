#ifndef COMMON_H
#define COMMON_H

// macros for handling errors
#define handle_error_en(en, msg)    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg)           do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* Configuration parameters */
#define DEBUG           1   // display debug messages
#define QUIT_COMMAND    "QUIT"
#define CLNT_FIFO_NAME  "fifo_client"
#define ECHO_FIFO_NAME  "fifo_echo"


#endif
