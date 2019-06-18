#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"

int shouldStop = 0;

void* receiveMessage(void* arg) {
    int socket_desc = (int)arg;

    char close_command[MSG_SIZE];
    sprintf(close_command,"%c%s", COMMAND_CHAR, QUIT_COMMAND);
    size_t close_command_len = strlen(close_command);

    /* select() uses sets of descriptors and a timeval interval. The
     * methods returns when either an event occurs on a descriptor in
     * the sets during the given interval, or when that time elapses.
     *
     * The first argument for select is the maximum descriptor among
     * those in the sets plus one. Note also that both the sets and
     * the timeval argument are modified by the call, so you should
     * reinitialize them across multiple invocations.
     *
     * On success, select() returns the number of descriptors in the
     * given sets for which data may be available, or 0 if the timeout
     * expires before any event occurs. */
    struct timeval timeout;
    fd_set read_descriptors;
    int nfds = socket_desc + 1;

    char buf[MSG_SIZE];
    char nickname[NICKNAME_SIZE];
    char delimiter[2];
    sprintf(delimiter, "%c", MSG_DELIMITER_CHAR);

    while (!shouldStop) {
        int ret;

        // check every 1.5 seconds (why not longer?)
        timeout.tv_sec  = 1;
        timeout.tv_usec = 500000;

        FD_ZERO(&read_descriptors);
        FD_SET(socket_desc, &read_descriptors);

        /** perform select() **/
        ret = select(nfds, &read_descriptors, NULL, NULL, &timeout);

        if (ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Unable to select()");

        if (ret == 0) continue; // timeout expired

        // ret is 1: read available data!
        int read_completed = 0;
        int read_bytes = 0; // index for writing into the buffer
        int bytes_left = MSG_SIZE - 1; // number of bytes to (possibly) read
        while(!read_completed) {
            ret = recv(socket_desc, buf + read_bytes, 1, 0);
            if (ret == 0) break;
            if (ret == -1 && errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella lettura da socket");
            bytes_left -= ret;
            read_bytes += ret;
            read_completed = bytes_left == 0 || buf[read_bytes - 1] == '\n';
        }

        if (ret == 0) {
            shouldStop = 1;
        } else {
            buf[read_bytes - 1] = '\0';
            int sender_length = strcspn(buf, delimiter);
            if (sender_length == strlen(buf)) {
                printf("[???] %s\n", buf);
            } else {
                snprintf(nickname, sender_length + 1, "%s", buf);
                printf("[%s] %s\n", nickname, buf + sender_length + 1);
            }
        }
    }

    pthread_exit(NULL);
}

void* sendMessage(void* arg) {
    int socket_desc = (int)arg;

    char close_command[MSG_SIZE];
    sprintf(close_command, "%c%s\n", COMMAND_CHAR, QUIT_COMMAND);
    size_t close_command_len = strlen(close_command);

    char buf[MSG_SIZE];

    while (!shouldStop) {
        int ret;

        /* Read a line from stdin: fgets() reads up to sizeof(buf)-1
         * bytes and on success returns the first argument passed. */
        if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
            fprintf(stderr, "Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }

        if (shouldStop) break; // the endpoint might have closed the connection

        size_t msg_len = strlen(buf);

        // send message
        while ( (ret = send(socket_desc, buf, msg_len, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(-1, "Cannot write to socket");
        }

        // After a BYE command we should update shouldStop
        if (msg_len == close_command_len && !memcmp(buf, close_command, close_command_len)) {
            shouldStop = 1;
        }
    }

    pthread_exit(NULL);
}

void chat_session(int socket_desc) {
    int ret;

    pthread_t chat_threads[2];

    ret = pthread_create(&chat_threads[0], NULL, receiveMessage, (void*)socket_desc);
    GENERIC_ERROR_HELPER(ret, ret, "Cannot create thread for receiving messages");

    ret = pthread_create(&chat_threads[1], NULL, sendMessage, (void*)socket_desc);
    GENERIC_ERROR_HELPER(ret, ret, "Cannot create thread for sending messages");

    // wait for termination
    ret = pthread_join(chat_threads[0], NULL);
    GENERIC_ERROR_HELPER(ret, ret, "Cannot join on thread for receiving messages");

    ret = pthread_join(chat_threads[1], NULL);
    GENERIC_ERROR_HELPER(ret, ret, "Cannot join on thread for sending messages");

    // close socket
    ret = close(socket_desc);
    ERROR_HELPER(ret, "Cannot close socket");
}

void syntax_error(char* prog_name) {
    fprintf(stderr, "Usage: %s <IP_address> <port_number> <nickname>\n", prog_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    if (argc == 4) {
        // we use network byte order
        in_addr_t ip_addr;
        unsigned short port_number_no;
        char nickname[NICKNAME_SIZE];

        // retrieve IP address
        ip_addr = inet_addr(argv[1]); // we omit error checking

        // retrieve port number
        long tmp = strtol(argv[2], NULL, 0); // safer than atoi()
        if (tmp < 1024 || tmp > 49151) {
            fprintf(stderr, "Please use a port number between 1024 and 49151.\n");
            exit(EXIT_FAILURE);
        }
        port_number_no = htons((unsigned short)tmp);

        sprintf(nickname, "%s", argv[3]);

        int ret;
        int socket_desc;
        struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0

        // create socket
        socket_desc = socket(AF_INET, SOCK_STREAM, 0);
        ERROR_HELPER(socket_desc, "Could not create socket");

        // set up parameters for the connection
        server_addr.sin_addr.s_addr = ip_addr;
        server_addr.sin_family      = AF_INET;
        server_addr.sin_port        = port_number_no;

        // initiate a connection on the socket
        ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
        ERROR_HELPER(ret, "Could not create connection");

        // invio comando di join
        char buf[MSG_SIZE];
        sprintf(buf, "%c%s %s\n", COMMAND_CHAR, JOIN_COMMAND, nickname);
        int msg_len = strlen(buf);
        while ( (ret = send(socket_desc, buf, msg_len, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(-1, "Cannot write to socket");
        }

        chat_session(socket_desc);
    } else {
        syntax_error(argv[0]);
    }

    exit(EXIT_SUCCESS);
}
