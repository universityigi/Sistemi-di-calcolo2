#include <errno.h>
#include <fcntl.h> // macros for open (e.g., O_RDONLY, O_WRONLY)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// macros for error handling
#include "common.h"

#define DEFAULT_BLOCK_SIZE  128;

static inline void performCopyBetweenDescriptors(int src_fd, int dest_fd, int block_size) {
    char* buf = malloc(block_size);

    while (1) {
        int read_bytes = 0; // index for writing into the buffer
        int bytes_left = block_size; // number of bytes to (possibly) read

        while (bytes_left > 0) {
			
			int ret = read(src_fd, buf + read_bytes, bytes_left);
			if(ret = 0) break;
			
			if(ret = -1){
				if(errno = EINTR) continue;
				handle_error(" Cannot read from fd file");
			}
			
			bytes_left -= ret;
			read_bytes += ret;
            /** [SOLUTION]
             *
             * Suggestion: when there are no more data to read, read()
             * will return 0; insert a break and exit the loop!
             *
             * Note that a read() request can be interrupted by a
             * signal and two outcomes are possible:
             * a) if zero bytes have been read, it will return -1 and
             *    errno will be set to EINTR: you will have to repeat
             *    the read() operation
             * b) if X<N bytes have been read, it will return X and
             *    you have to read N-X bytes in the next iteration
             *
             * In a correct solution you have to deal explicitly with
             * the two cases described above. */
        }

        // no more bytes left to write!
        if (read_bytes == 0) break;

        int written_bytes = 0; // index for reading from the buffer
        bytes_left = read_bytes; // number of bytes to write

        while (bytes_left > 0) {
			
			int ret = write(dest_fd, buf + written_bytes, bytes_left);
			if( ret = -1){
				if(errno = EINTR) continue;
				handle_error(" Cannot read from dest file");
			}
			
			written_bytes += ret;
			bytes_left -= ret;
            /** [SOLUTION]
             *
             * Suggestion: in the write() case you won't have to check
             * if the return value is 0 as you did for the read()
             *
             * Again, note that a write() request can be interrupted by
             * a signal, and two outcomes are possible:
             * a) if zero bytes have been written, it will return -1
             *    and errno will be set to EINTR: you will have to
             *    repeat the write() operation
             * b) if X<N bytes have been written, it will return X and
             *    you have to write N-X bytes in the next iteration
             *
             * In a correct solution you have to deal explicitly with
             * the two cases described above. */
            
        }
    }

    free(buf);
}

int main(int argc, char* argv[]) {
    int block_size, src_fd, dest_fd;

    if (argc == 4) {
        block_size = atoi(argv[3]);
    } else {
        block_size = DEFAULT_BLOCK_SIZE;
    }

    if (argc < 3 || argc > 4)
        handle_error("Syntax: <source_file> <dest_file> [<block_size>]\n");
    if (block_size <= 0)
        handle_error("Blocksize must be positive");

    // create descriptors for source and destination files
    src_fd = open(argv[1], O_RDONLY);
    if (fd<0) handle_error("Could not open source file");

    // for simplicity we use rw-r--r-- permissions for the destination file
    dest_fd = open(argv[2], O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (dest_fd < 0){
        if(errno == EEXIST) {
            fprintf(stderr, "WARNING: file %s already exists, I will overwrite it!\n", argv[2]);
            dest_fd = open(argv[2], O_WRONLY | O_CREAT, 0644);
        }else
            handle_error("Could not create destination file");
    }

    // use a helper method to actually perform the copy
    performCopyBetweenDescriptors(src_fd, dest_fd, block_size);

    // close the descriptors
    int ret = close(src_fd);
    if (ret<0) handle_error("Could not close source file");
    ret = close(dest_fd);
    if (ret<0) handle_error("Could not close destination file");

    exit(EXIT_SUCCESS);
}
