// intercepts CTRL+C and runs a cleanup function before quitting
void setQuitHandler(void(*f)());

// macros for handling errors
#define handle_error_en(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)