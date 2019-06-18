#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> // mkfifo()
#include <sys/stat.h>  // mkfifo()

#include "common.h"
int readOneByOne(int fd, char* buf, char separator);
void writeMsg(int fd, char* buf, int size);

/** Client component **/
int main(int argc, char* argv[]) {
    int ret;
    int echo_fifo, client_fifo;
    char buf[1024];

    char* quit_command = QUIT_COMMAND;
    size_t quit_command_len = strlen(quit_command);

    /** [SOLUTION] OPEN THE TWO FIFOS
     *
     * Suggestions:
     * - the two FIFOs should be opened in the same order in both the
     *   Echo and the Client programs to avoid deadlocks
     * - open in 'O_WRONLY' or 'O_RDONLY' mode to fullfil the following
     *   requirement: the Echo program sends data through 'echo_fifo'
     *   and the Client program does it through 'client_fifo'
     **/

    /* It is important that the two FIFOs are opened in the same order
     * in both the Echo and the Client process. */
    echo_fifo = open(ECHO_FIFO_NAME, O_RDONLY);
    if(echo_fifo == -1) handle_error("Cannot open Echo FIFO for reading");
    client_fifo = open(CLNT_FIFO_NAME, O_WRONLY);
    if(client_fifo == -1) handle_error("Cannot open Client FIFO for writing");

     // display welcome message received from the Echo process
    /** [SOLUTION] READ THE MESSAGE THROUGH THE ECHO FIFO
     *
     * Suggestions:
     * - you can read from a FIFO as from a regular file descriptor
     * - since you don't know the length of the message, just
     *   read one byte at a time from the socket
     * - leave the cycle when '\n' is encountered
     * - repeat the read() in case we get interrupted before read()
     *   could write any byte to 'buf'
     * - store the number of bytes read in 'bytes_read'
     * - reading 0 bytes means that the other process has closed
     *   the FIFO unexpectedly: this is an error to deal with!
     **/
    int bytes_read = readOneByOne(echo_fifo, buf, '\n');

    buf[bytes_read] = '\0';
    printf("%s", buf);

    // main loop
    while (1) {
        printf("Insert your message: ");

        // read a line from stdin (including newline symbol '\n')
        if (fgets(buf, sizeof(buf), stdin) != (char*)buf){
            handle_error("Error while reading from stdin, exiting...\n");
        }

        // send message to Echo process
        int msg_len = strlen(buf);
        writeMsg(client_fifo, buf, msg_len);

        /* After a quit command we won't receive data from the server
         * anymore, thus we must exit the main loop. */
        if (msg_len - 1 == quit_command_len && // -1 to ignore trailing '\n'
            !memcmp(buf, quit_command, quit_command_len)) break;

        // read message from Echo process         
        bytes_read = readOneByOne(echo_fifo, buf, '\n');
        buf[bytes_read] = '\0';
        printf("Server response: %s", buf);
    }

    // close the descriptors
    ret = close(echo_fifo);
    if(ret) handle_error("Cannot close Echo FIFO");
    ret = close(client_fifo);
    if(ret) handle_error("Cannot close Client FIFO");

    exit(EXIT_SUCCESS);
}
