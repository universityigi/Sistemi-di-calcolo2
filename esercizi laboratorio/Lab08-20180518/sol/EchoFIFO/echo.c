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

static void cleanFIFOs(int echo_fifo, int client_fifo) {

    /** [TO DO] Method to close and remove the two FIFOs
     *
     * - We should run cleanup code also when the Echo module detects that the Client
     *   has closed 'client_fifo' unexpectedly. FIFOs remain in the filesystem if 
     *   we do not perform unlink() on them once all the descriptors have been closed.
     * - Close the two fifos
     * - Destroy the two fifos
     * */

    // close the descriptors
    int ret = close(echo_fifo);
    if(ret) handle_error("Cannot close Echo FIFO");
    ret = close(client_fifo);
    if(ret) handle_error("Cannot close Client FIFO");

    // destroy the two FIFOs
    ret = unlink(ECHO_FIFO_NAME);
    if(ret) handle_error("Cannot unlink Echo FIFO");
    ret = unlink(CLNT_FIFO_NAME);
    if(ret) handle_error("Cannot unlink Client FIFO");
}

/** Echo component **/
int main(int argc, char* argv[]) {
    int ret;
    int echo_fifo, client_fifo;
    char buf[1024];

    char* quit_command = QUIT_COMMAND;
    size_t quit_command_len = strlen(quit_command);

    // Create the two FIFOs
    ret = mkfifo(ECHO_FIFO_NAME, 0666);
    if(ret) handle_error("Cannot create Echo FIFO");
    ret = mkfifo(CLNT_FIFO_NAME, 0666);
    if(ret) handle_error("Cannot create Client FIFO");

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
    echo_fifo = open(ECHO_FIFO_NAME, O_WRONLY);
    if(echo_fifo == -1) handle_error("Cannot open Echo FIFO for writing");
    client_fifo = open(CLNT_FIFO_NAME, O_RDONLY);
    if(client_fifo) handle_error("Cannot open Client FIFO for reading");

    // send welcome message
    sprintf(buf, "Hi! I'm an Echo process based on FIFOs. I will send you back through a FIFO whatever"
            " you send me through the other FIFO, and I will stop and exit when you send me %s.\n", quit_command);

    writeMsg(echo_fifo, buf, strlen(buf));
     
    while (1) {
        int bytes_read = readOneByOne(client_fifo, buf, '\n');

        if (DEBUG) {
            buf[bytes_read] = '\0';
            printf("Message received: %s", buf);
        }

        // check whether I have just been told to quit...
        if (bytes_read == quit_command_len && !memcmp(buf, quit_command, quit_command_len)) break;

        // ... or if I have to send the message back through the Echo FIFO
        writeMsg(echo_fifo, buf, bytes_read);
    }

    // close the descriptors and destroy the two FIFOs
    cleanFIFOs(echo_fifo, client_fifo);
    exit(EXIT_SUCCESS);
}
