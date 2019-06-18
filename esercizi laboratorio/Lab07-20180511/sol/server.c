#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>

#include "common.h"

#ifdef SERVER_MPROC
// nothing to do here
#elif SERVER_MTHREAD
#include <pthread.h>
typedef struct handler_args_s
{
    /** SOLUTION
     *
     * Specify fields for the arguments that will be populated in the
     * main thread and then accessed in connection_handler(void* arg).
     **/
    int socket_desc;
    struct sockaddr_in* client_addr;
} handler_args_t;
#elif SERVER_SERIAL
// nothing to do here
#endif

void connection_handler(int socket_desc, struct sockaddr_in* client_addr) {
    int ret, recv_bytes;

    char buf[1024];
    size_t buf_len = sizeof(buf);
    int msg_len;

    char* quit_command = SERVER_COMMAND;
    size_t quit_command_len = strlen(quit_command);

    // parse client IP address and port
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    uint16_t client_port = ntohs(client_addr->sin_port); // port number is an unsigned short

    // send welcome message
    sprintf(buf, "Hi! I'm an echo server. You are %s talking on port %hu.\nI will send you back whatever"
            " you send me. I will stop if you send me %s :-)\n", client_ip, client_port, quit_command);
    msg_len = strlen(buf);
    while ( (ret = send(socket_desc, buf, msg_len, 0)) < 0 ) {
        if (errno == EINTR) continue;
        handle_error("Cannot write to the socket");
    }

    // echo loop
    while (1) {
        // read message from client
        while ( (recv_bytes = recv(socket_desc, buf, buf_len, 0)) < 0 ) {
            if (errno == EINTR) continue;
            handle_error("Cannot read from socket");
        }

        // check whether I have just been told to quit...
        if (recv_bytes == 0) break;
        if (recv_bytes == quit_command_len && !memcmp(buf, quit_command, quit_command_len)) break;

        // ... or if I have to send the message back
        while ( (ret = send(socket_desc, buf, recv_bytes, 0)) < 0 ) {
            if (errno == EINTR) continue;
            handle_error("Cannot write to the socket");
        }
    }

    // close socket
    ret = close(socket_desc);
    if (ret) handle_error("Cannot close socket for incoming connection");
}


#ifdef SERVER_SINGLE

void serialServer(int server_desc) {
    // we initialize client_addr to zero
    struct sockaddr_in client_addr = {0};

    // loop to handle incoming connections serially
    int sockaddr_len = sizeof(struct sockaddr_in);
    while (1) {
        int client_desc = accept(server_desc, (struct sockaddr*) &client_addr, (socklen_t*) &sockaddr_len);
        if (client_desc == -1 && errno == EINTR) continue; // check for interruption by signals
        if (client_desc < 0) handle_error("Cannot open socket for incoming connection");

        if (DEBUG) fprintf(stderr, "Incoming connection accepted...\n");

        /* We pass the socket descriptor and the address information
         * for the incoming connection to the handler. */
        connection_handler(client_desc, &client_addr);

        if (DEBUG) fprintf(stderr, "Done!\n");

        // reset fields in client_addr so it can be reused for the next accept()
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
    }
}

#elif SERVER_MPROC

void mprocServer(int server_desc) {
    int ret = 0;
    // we initialize client_addr to zero
    struct sockaddr_in client_addr = {0};

    // loop to manage incoming connections forking the server process
    int sockaddr_len = sizeof(struct sockaddr_in);
    while (1) {
        // accept incoming connection
        int client_desc = accept(server_desc, (struct sockaddr *) &client_addr, (socklen_t *)&sockaddr_len);
        // check for interruption by signals
        if (client_desc == -1 && errno == EINTR) continue; 
        if (client_desc < 0) handle_error("Cannot open socket for incoming connection");

        if (DEBUG) fprintf(stderr, "Incoming connection accepted...\n");

        /** SOLUTION
         *
         * Suggestions:
         * - use fork() to create a child process to handle the request
         * - close descriptors that are not used in the parent and the
         *   child process: note that a connection is closed only when
         *   all the descriptors associated with it have been closed!
         * - connection_handler(client_desc, client_addr) should be
         *   executed by the child process to handle the request
         * - memset(client_addr, 0, sizeof(struct sockaddr_in)) should
         *   be performed in order to be able to accept a new request
         * - free memory where needed
         * - add a debug message in the parent process to inform the
         *   user that a child process has been created
         * - add a debug message in the child process to inform the
         *   user that the request has been handled
         **/

        pid_t pid = fork();
        if (pid < 0) handle_error("Cannot fork server process to handle the request");
        else if (pid == 0) {
            // child: close the listening socket and process the request
            ret = close(server_desc);
            if (ret) handle_error("Cannot close listening socket in child process");
            connection_handler(client_desc, &client_addr);
            if (DEBUG) fprintf(stderr, "Process created to handle the request has completed.\n");
            _exit(EXIT_SUCCESS);
        } 
        else {
            // server: close the incoming socket and continue
            ret = close(client_desc);
            if (ret) handle_error("Cannot close incoming socket in main process");
            if (DEBUG) fprintf(stderr, "Child process created to handle the request!\n");
            // reset fields in client_addr so it can be reused for the next accept()
            memset(&client_addr, 0, sizeof(struct sockaddr_in));
        }
    }
}

#elif SERVER_MTHREAD

// Wrapper function that take as input handler_args_t struct and then call 
// connection_handler.
void *thread_connection_handler(void *arg) {
    handler_args_t *args = (handler_args_t *)arg;
    int socket_desc = args->socket_desc;
    struct sockaddr_in *client_addr = args->client_addr;
    connection_handler(socket_desc,client_addr);
    // do not forget to free client_addr! by design it belongs to the
    // thread spawned for handling the incoming connection
    free(args->client_addr);
    free(args);
    pthread_exit(NULL);
}

void mthreadServer(int server_desc) {
    int ret = 0;

    // loop to manage incoming connections spawning handler threads
    int sockaddr_len = sizeof(struct sockaddr_in);
    while (1) {
        // we dynamically allocate a fresh client_addr object at each
        // loop iteration and we initialize it to zero
        struct sockaddr_in* client_addr = calloc(1, sizeof(struct sockaddr_in));
        
        // accept incoming connection
        int client_desc = accept(server_desc, (struct sockaddr *) client_addr, (socklen_t *)&sockaddr_len);
        
        if (client_desc == -1 && errno == EINTR) continue; // check for interruption by signals
        if (client_desc < 0) handle_error("Cannot open socket for incoming connection");
        
        if (DEBUG) fprintf(stderr, "Incoming connection accepted...\n");

        /** SOLUTION
         *
         * Suggestions:
         * - connection_handler(void* arg) is the method to execute in
         *   the thread we are going to spawn
         * - allocate a handler_args_t data structure and populate it
         *   with the arguments that will be passed to the thread
         * - print a debug message to inform the user that a new thread
         *   has been created to handle the request
         * - since we don't need to pthread_join(), release libpthread's
         *   internal resources after pthread_create()
         * - to accept a new connection while a thread is executing, be
         *   careful: resetting fields in client_addr won't work!
         **/

        pthread_t thread;

        // prepare arguments for the new thread
        handler_args_t *thread_args = malloc(sizeof(handler_args_t));
        thread_args->socket_desc = client_desc;
        thread_args->client_addr = client_addr;
        ret = pthread_create(&thread, NULL, thread_connection_handler, (void *)thread_args);
        if (ret) handle_error_en(ret, "Could not create a new thread");
        
        if (DEBUG) fprintf(stderr, "New thread created to handle the request!\n");
        
        ret = pthread_detach(thread); // I won't phtread_join() on this thread
        if (ret) handle_error_en(ret, "Could not detach the thread");
    }
}

#endif

int main(int argc, char* argv[]) {
    int ret;

    int socket_desc;

    // some fields are required to be filled with 0
    struct sockaddr_in server_addr = {0};

     // we will reuse it for accept()

    // initialize socket for listening
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc < 0) handle_error("Could not create socket");

    server_addr.sin_addr.s_addr = INADDR_ANY; // we want to accept connections from any interface
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    /* We enable SO_REUSEADDR to quickly restart our server after a crash:
     * for more details, read about the TIME_WAIT state in the TCP protocol */
    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    if (ret) handle_error("Cannot set SO_REUSEADDR option");

    // bind address to socket
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if (ret) handle_error("Cannot bind address to socket");

    // start listening
    ret = listen(socket_desc, MAX_CONN_QUEUE);
    if (ret) handle_error("Cannot listen on socket");

#ifdef SERVER_MPROC
    mprocServer(socket_desc);
#elif SERVER_MTHREAD
    mthreadServer(socket_desc);
#elif SERVER_SINGLE
    serialServer(socket_desc);
#endif

    exit(EXIT_SUCCESS); // this will never be executed
}
